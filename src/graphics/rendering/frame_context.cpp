module;
#include <string.h>
module nekomata2;
import vulkan;
import vk_mem_alloc;
import :core.log;
import :core.platform.int_def;
import :graphics.rendering.frame_rendering_resources;
import :core.ecs.world.camera;
import :core.ecs.world.transform;
import :graphics.fontsystem.font_manager;
import :graphics.vulkan.vk_buffer;
import :graphics.meshsystem.mesh_asset_storage;
import :core.ui.ui_drawcmds;
import :graphics.vulkan.context;
import :graphics.texturesystem.texture_manager;
import :graphics.vulkan.vk_commands_barriers;
import :core.ecs.entity;
import :core.overloaded;
import :graphics.rendering.frame_context;

namespace nekomata2::graphics {

using namespace nekomata2::math;

FrameContext::FrameContext(std::nullptr_t) {  }
FrameContext::FrameContext() {
    m_frameRenderingResources = FrameRenderingResources(2048);
}

inline vk::Offset3D toOffset3D(const vk::Extent3D& extent) {
    return vk::Offset3D{
        static_cast<i32>(extent.width),
        static_cast<i32>(extent.height),
        static_cast<i32>(extent.depth)
    };
}

inline bool isObjectVisible(
    Vector3f objectPos, float objectBoundingSphereRadius,
    Vector3f cameraPos, Quaternion cameraRotation,
    float perspectiveFov, float perspectiveAspectRatio, float perspectiveNear, float perspectiveFar
) {
    auto objectToCamera = cameraPos - objectPos;
    auto cameraRotationConj = cameraRotation.conjugate();
    auto camspaceObjectPos = cameraRotationConj.rotateVector3f(objectToCamera);

    // Near/far planes test
    if (camspaceObjectPos.z() + objectBoundingSphereRadius < perspectiveNear) return false;
    if (camspaceObjectPos.z() - objectBoundingSphereRadius > perspectiveFar) return false;

    // Left/right/top/bottom planes test
    float hy = perspectiveFov * 0.5f;
    float hx = std::atanf(std::tanf(hy) * perspectiveAspectRatio);
    float sx = std::sin(hx), sy = std::sin(hy);
    float cx = std::cos(hx), cy = std::cos(hy);

    if ( camspaceObjectPos.x() * cx + camspaceObjectPos.z() * sx < -objectBoundingSphereRadius) return false; // left
    if (-camspaceObjectPos.x() * cx + camspaceObjectPos.z() * sx < -objectBoundingSphereRadius) return false; // right
    if ( camspaceObjectPos.y() * cy + camspaceObjectPos.z() * sy < -objectBoundingSphereRadius) return false; // top
    if (-camspaceObjectPos.y() * cy + camspaceObjectPos.z() * sy < -objectBoundingSphereRadius) return false; // bottom

    return true;
}

auto FrameContext::execute(TransientRenderingResources& transientRenderingResources, SharedRenderingResources& sharedRenderingResources, VulkanSwapchain& swapchain, MRThreadsSharedDataLeaf& renderingData, float timeSinceStart) -> FrameResult {
    m_frameRenderingResources.frameDoneFence().waitForSignal(std::numeric_limits<u64>::max());

    auto imageAcquire = swapchain.acquireNextImage(std::numeric_limits<u64>::max(), m_frameRenderingResources.imageAcquiredSemaphore());

    if (!imageAcquire.first.has_value() || imageAcquire.second) {
        return { .shouldRecreateSwapchain = true, .stepPerFrameResources = false };
    }

    m_frameRenderingResources.frameDoneFence().reset();
    sharedRenderingResources.refitHysteresisStates(renderingData.m_renderables.m_sparseToStorage.size());

    // ---------------------------------------------------------------- Render Pass starts here ----------------------------------------------------------------

    ecs::components::Camera firstCamera;
    ecs::components::Transform firstCameraTransform;
    bool firstCameraFound = false;

    for (auto [i, camera] : renderingData.m_cameras.m_storage.iter().enumerate()) {
        firstCamera = camera;

        auto cameraEntitySparseIndex = renderingData.m_cameras.m_storageToEntity[i];;

        if (renderingData.m_transforms.containsEntity(cameraEntitySparseIndex)) {
            firstCameraTransform = renderingData.m_transforms.get(cameraEntitySparseIndex);
        } else {
            firstCameraTransform = ecs::components::Transform{};
            firstCameraTransform.m_transform3d = math::Transform3D::identity();
            log::warn("Camera #{} has no transform!", i);
        }
        firstCameraFound = true;
        break;
    }

    if (!firstCameraFound) {
        // log::warn("No camera found! Will use a default one");
        firstCamera = ecs::components::Camera{};
        firstCamera.nearPlane = 0.1f;
        firstCamera.farPlane = 1000.0f;
        firstCamera.fov = 75.0f;
        firstCamera.renderingEnable = true;
        firstCameraTransform = ecs::components::Transform{};
        firstCameraTransform.m_transform3d = math::Transform3D::identity();
        firstCameraTransform.m_transform3d.m_position = math::Vector3f(0.0f, 0.0f, 1.2f);
    }

    math::Vector2f renderingArea = Vector2f(static_cast<f32>(transientRenderingResources.finalDrawBuffer().extent().width), static_cast<f32>(transientRenderingResources.finalDrawBuffer().extent().height));

    float aspectRatio = static_cast<float>(transientRenderingResources.finalDrawBuffer().extent().width) / static_cast<float>(transientRenderingResources.finalDrawBuffer().extent().height);
    float perspFocalLength = renderingArea.y() / (2.0f * std::tan(0.5f * degreesToRadians(firstCamera.fov)));

    m_frameRenderingResources.prepareTransformsBuffer(renderingData, firstCamera, firstCameraTransform, aspectRatio);
    auto& swapchainImage = swapchain.imageAtIndex(*imageAcquire.first);

    m_frameRenderingResources.commandPool().reset();

    auto& cb = m_frameRenderingResources.commandBuffer().vkCommandBuffer();

    auto beginInfo = vk::CommandBufferBeginInfo{}
    .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cb.begin(beginInfo);

    // Temporary:: prepare font

    VulkanBuffer stagingBuffer = nullptr;

    // see if there are new glyphs to rasterize in the system text..
    auto all_texts_iter = renderingData.m_uiDrawCmds.iter()
        .filterMap([&](const auto& x) {
            if (!std::holds_alternative<ui::UiTextDrawCmd>(x)) return Option<fonts::FontRasterBatch>::none();
            auto cmd = std::get<ui::UiTextDrawCmd>(x);
            auto batch = fonts::FontManager::get().findAndBatchMissingGlyphs(cmd.face, sharedRenderingResources.m_fontAtlas, cmd.text, cmd.size);
            return batch;
        })
        .collect<Vec>();

    if (!all_texts_iter.isEmpty()) {
        auto pixelBuffer = Vec<u8>::create();
        auto newImageIndices = Vec<u32>::create();
        auto bufferImageCopyRegions = HashMap<u32, Vec<vk::BufferImageCopy2>>::create();
        fonts::FontRasterInfo rasterInfo = { all_texts_iter, sharedRenderingResources.m_fontAtlas, bufferImageCopyRegions, pixelBuffer, newImageIndices };
        fonts::FontManager::get().rasterizeGlyphs(rasterInfo);

        // there can be glyphs that don't rasterize to anything but appeared in the batch, so the buffer might be zero-sized
        if (pixelBuffer.len() != 0) {
            stagingBuffer = VulkanBuffer::create(pixelBuffer.len(), vk::BufferUsageFlagBits::eTransferSrc, VulkanBufferMemoryMapping::MapForSequentialWrite, vma::MemoryUsage::eAutoPreferDevice, {}, VulkanContext::get().vkPhysicalDeviceProps().m_queueFamilies[QueueFamily::Graphics]);
            memcpy(stagingBuffer.memoryHostPtr(), pixelBuffer.data(), pixelBuffer.len());

            // Prepare for copy
            auto barriers = VulkanPipelineBarriers::builder();
            for (const auto& atlasImageIndex : bufferImageCopyRegions.keys()) {
                // For images that are newly created, transition from eUndefined instead
                if (std::ranges::find(newImageIndices, atlasImageIndex) != newImageIndices.end()) {
                    barriers.insertImageMemoryBarrier(sharedRenderingResources.m_fontAtlas.m_atlasTextures[atlasImageIndex].image,
                        vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone,
                        vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferWrite
                    );
                    continue;
                }

                // Transition from eShaderReadOnlyOptimal for all others
                barriers.insertImageMemoryBarrier(sharedRenderingResources.m_fontAtlas.m_atlasTextures[atlasImageIndex].image,
                    vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead,
                    vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferWrite
                );
            }
            barriers.flush(m_frameRenderingResources.commandBuffer());

            // Run copies
            for (const auto& [atlasImageIndex, regions] : bufferImageCopyRegions.iter()) {
                auto copyInfo = vk::CopyBufferToImageInfo2{}
                    .setDstImage(sharedRenderingResources.m_fontAtlas.m_atlasTextures[atlasImageIndex].image.vkImage())
                    .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
                    .setSrcBuffer(stagingBuffer.vkBuffer())
                    .setRegions(regions);

                cb.copyBufferToImage2(copyInfo);
            }

            // Prepare for usage
            auto barriers2 = VulkanPipelineBarriers::builder();
            for (const auto& atlasImageIndex : bufferImageCopyRegions.keys()) {
                barriers2.insertImageMemoryBarrier(sharedRenderingResources.m_fontAtlas.m_atlasTextures[atlasImageIndex].image,
                    vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferWrite,
                    vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead
                );
            }
            barriers2.flush(m_frameRenderingResources.commandBuffer());
        }
    }

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(transientRenderingResources.finalDrawBuffer(),
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eBlit, {},
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite
        )
        .insertImageMemoryBarrier(transientRenderingResources.depthBuffer(),
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::ImageLayout::eDepthAttachmentOptimal, vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite
        )
        .flush(m_frameRenderingResources.commandBuffer());

    auto drawImageAttachmentInfo = vk::RenderingAttachmentInfo{}
        .setImageView(transientRenderingResources.finalDrawBuffer().vkImageViewWholeSize())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearColorValue{}.setFloat32({0.0, 0.0, 0.0, 1.0}));
    auto depthImageAttachmentInfo = vk::RenderingAttachmentInfo{}
        .setImageView(transientRenderingResources.depthBuffer().vkImageViewWholeSize())
        .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setClearValue(vk::ClearDepthStencilValue{}.setDepth(0.0f));

    auto vkRenderingArea = vk::Extent2D{transientRenderingResources.finalDrawBuffer().extent().width, transientRenderingResources.finalDrawBuffer().extent().height};

    auto renderingInfo = vk::RenderingInfo{}
        .setColorAttachments(drawImageAttachmentInfo)
        .setPDepthAttachment(&depthImageAttachmentInfo)
        .setLayerCount(1)
        .setRenderArea(vk::Rect2D{}.setExtent(vkRenderingArea));

    auto viewport = vk::Viewport{}
        .setWidth(static_cast<f32>(vkRenderingArea.width))
        .setHeight(static_cast<f32>(vkRenderingArea.height))
        .setMinDepth(0.0)
        .setMaxDepth(1.0);
    auto scissor = vk::Rect2D{}.setExtent(vkRenderingArea);

    cb.beginRendering(renderingInfo);
    cb.setViewport(0, viewport);
    cb.setScissor(0, scissor);

    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, sharedRenderingResources.simplePipeline().vkPipeline());
    texturesystem::TextureManager::get().shaderResourceTable().bindToCommandBuffer(m_frameRenderingResources.commandBuffer(), sharedRenderingResources.simpleLayout(), vk::PipelineBindPoint::eGraphics);
    auto elapsed = std::chrono::steady_clock::now() - sharedRenderingResources.m_tmStart;
    float seconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / 1000.0;
    auto uboDeviceAddr = m_frameRenderingResources.transformsBuffer().memoryDevicePtr();
    auto pushConstantData = std::array<unsigned char, 32>{};

    memcpy((void*)(pushConstantData.data() + 16), &seconds, 4);
    u32 verticesThisDraw = 0;

    u32 shouldInvertColors = 0;
    for (auto [i, renderable] : renderingData.m_renderables.m_storage.iter().enumerate()) {
        // Get the LOD list for the renderable and skip it if no LODs are available
        auto& lodList = meshsystem::MeshAssetStorage::get().getLodList(renderable.meshAsset);
        auto bestAvailableLod = lodList.bestLodIndex.load(std::memory_order_acquire);
        if (bestAvailableLod == ~0u) continue;

        // Copy its transforms addr to push constants
        vk::DeviceAddress uboFinalAddr = uboDeviceAddr + i * sizeof(Matrix4x4f);
        memcpy((void*)pushConstantData.data(), &uboFinalAddr, 8);

        auto textureImageId = renderingData.m_textureToImageShaderIndexSnapshot[renderable.texture.index];

        // test for fonts
        // auto textureImageId = sharedRenderingResources.m_fontAtlas.m_atlasTextures[0].imageShaderIndex;

        auto textureSamplerId = renderingData.m_textureToSamplerShaderIndexSnapshot[renderable.texture.index];
        memcpy((void*)(pushConstantData.data() + 20), &textureImageId, 4);
        memcpy((void*)(pushConstantData.data() + 24), &textureSamplerId, 4);
        memcpy((pushConstantData.data() + 28), &shouldInvertColors, 4);


        // Pick an LOD
        Vector3f objectPos = Vector3f(0.0f);
        float objectUniformScale = 1.0f;
        ecs::Entity ent = renderingData.m_renderables.m_sparseToStorage[i];
        if (renderingData.m_transforms.containsEntity(ent)) {
            objectPos = renderingData.m_transforms.get(ent).m_transform3d.m_position;
            auto transformMatrix = renderingData.m_transforms.get(ent).m_transform3d.computeModelMatrix();
            float sx = Vector3f(transformMatrix[0, 0], transformMatrix[1, 0], transformMatrix[2, 0]).length();
            float sy = Vector3f(transformMatrix[0, 1], transformMatrix[1, 1], transformMatrix[2, 1]).length();
            float sz = Vector3f(transformMatrix[0, 2], transformMatrix[1, 2], transformMatrix[2, 2]).length();
            objectUniformScale = std::max({sx, sy, sz});
        }

        // See if the object is visible
        if (!isObjectVisible(objectPos, objectUniformScale * lodList.boundingSphereRadius, firstCameraTransform.m_transform3d.m_position, firstCameraTransform.m_transform3d.m_rotation, degreesToRadians(firstCamera.fov), aspectRatio, firstCamera.nearPlane, firstCamera.farPlane)) {
            continue;
        }

        float screenPixels = lodList.computeScreenSpaceError(objectPos, firstCameraTransform.m_transform3d.m_position, perspFocalLength, objectUniformScale);

        MeshHysteresisState& hysteresisState = sharedRenderingResources.getHysteresisState(ent.index());
        // Hysteresis states can be reused between entity creations/destructions. This prevents possibly using an unavailable LOD.
        hysteresisState.currentLod = std::clamp(hysteresisState.currentLod, bestAvailableLod, lodList.maxLodIndex);

        while (hysteresisState.currentLod > bestAvailableLod) {
            // must test next level LOD to see if satisfies new threshold
            float upgradeScreenPixelsThreshold = lodList.lods[hysteresisState.currentLod - 1].screenSizeThreshold * lodList.lodHysteresisFactor;
            if (screenPixels >= upgradeScreenPixelsThreshold) {
                hysteresisState.currentLod--;
                // log::info("Object {} upgraded to LOD level {}", ent.index(), hysteresisState.currentLod);
            } else {
                break;
            }
        }

        while (hysteresisState.currentLod < lodList.maxLodIndex) {
            // tests current level LOD to see if no longer satisfies current threshold
            float downgradeScreenPixelsThreshold = lodList.lods[hysteresisState.currentLod].screenSizeThreshold / lodList.lodHysteresisFactor;
            if (screenPixels < downgradeScreenPixelsThreshold) {
                hysteresisState.currentLod++;
                // log::info("Object {} downgraded to LOD level {}", ent.index(), hysteresisState.currentLod);
            } else {
                break;
            }
        }

        auto& lod = lodList.lods[hysteresisState.currentLod];

        cb.bindIndexBuffer(lod.meshSuballocation.indexBuffer.buffer, lod.meshSuballocation.indexBuffer.offset, vk::IndexType::eUint32);

        auto vboDeviceAddr = lod.meshSuballocation.vertexBuffer.deviceAddress;
        memcpy((void*)(pushConstantData.data() + 8), &vboDeviceAddr, 8);

        cb.pushConstants<unsigned char>(sharedRenderingResources.simpleLayout().vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eTessellationEvaluation, 0, pushConstantData);
        cb.drawIndexed(lod.meshSuballocation.indexBuffer.size / sizeof(u32), 1, 0, 0, 0);
        verticesThisDraw += lod.meshSuballocation.indexBuffer.size / sizeof(u32);

        shouldInvertColors = shouldInvertColors == 0 ? 1 : 0;
    }

    // Draw UI

    auto textInstanceBuffers = Vec<VulkanBuffer>::create();

    auto queuesForBuffer = VulkanContext::get().vkPhysicalDeviceProps().m_queueFamilies[QueueFamily::Graphics];
    for (const auto& uiDrawCmd : renderingData.m_uiDrawCmds) {
        match(uiDrawCmd,
            [&](const ui::UiRectDrawCmd& drawCmd) {
                cb.bindPipeline(vk::PipelineBindPoint::eGraphics, sharedRenderingResources.m_uiRectRendererPipeline.vkPipeline());
                cb.pushConstants<ui::UiRectDrawCmd>(sharedRenderingResources.m_uiRectRendererLayout.vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, drawCmd);
                cb.draw(4, 1, 0, 0);
            },
            [&](const ui::UiTextureDrawCmd& drawCmd) {
                struct PushConstants {
                    Vector2f ndcBegin, ndcEnd, texcoordBegin, texcoordEnd;
                    u32 textureIndex;
                    u32 samplerIndex;
                };

                u32 textureIndex = renderingData.m_textureToImageShaderIndexSnapshot[drawCmd.texture.index];
                u32 samplerIndex = renderingData.m_textureToSamplerShaderIndexSnapshot[drawCmd.texture.index];

                PushConstants pushConstants = {
                    .ndcBegin = drawCmd.ndcBegin,
                    .ndcEnd = drawCmd.ndcEnd,
                    .texcoordBegin = drawCmd.texcoordBegin,
                    .texcoordEnd = drawCmd.texcoordEnd,
                    .textureIndex = textureIndex,
                    .samplerIndex = samplerIndex
                };
                cb.bindPipeline(vk::PipelineBindPoint::eGraphics, sharedRenderingResources.m_uiTextureRendererPipeline.vkPipeline());
                texturesystem::TextureManager::get().shaderResourceTable().bindToCommandBuffer(m_frameRenderingResources.commandBuffer(), sharedRenderingResources.m_uiTextureRendererLayout, vk::PipelineBindPoint::eGraphics);
                cb.pushConstants<PushConstants>(sharedRenderingResources.m_uiTextureRendererLayout.vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstants);
                cb.draw(4, 1, 0, 0);
            },
            [&](const ui::UiTextDrawCmd& drawCmd) {
                auto shapedText = fonts::FontManager::get().shapeText(drawCmd.face, sharedRenderingResources.m_fontAtlas, drawCmd.text, drawCmd.size, drawCmd.baselinePos, renderingArea);
                auto buffer = VulkanBuffer::create(shapedText.size() * sizeof(fonts::GlyphInstance), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VulkanBufferMemoryMapping::MapForSequentialWrite, vma::MemoryUsage::eAutoPreferDevice, {}, queuesForBuffer);
                memcpy(buffer.memoryHostPtr(), shapedText.data(), shapedText.size() * sizeof(fonts::GlyphInstance));

                auto fontPushConstantData2 = std::array<unsigned char, 12>{};

                u32 sampler2 = texturesystem::TextureManager::get().samplerCache().acquireSampler(
                    texturesystem::SamplerParams::defaultValues().setMinFilter(vk::Filter::eNearest).setMagFilter(vk::Filter::eNearest).setMipmapMode(vk::SamplerMipmapMode::eNearest).setMaxLod(0.0f)
                );
                auto instanceDevicePtr2 = buffer.memoryDevicePtr();
                memcpy(fontPushConstantData2.data(), &instanceDevicePtr2, 8);
                memcpy(fontPushConstantData2.data() + 8, &sampler2, 4);
                textInstanceBuffers.emplace(std::move(buffer));

                cb.bindPipeline(vk::PipelineBindPoint::eGraphics, sharedRenderingResources.m_bitmapFontRendererPipeline.vkPipeline());
                texturesystem::TextureManager::get().shaderResourceTable().bindToCommandBuffer(m_frameRenderingResources.commandBuffer(), sharedRenderingResources.m_bitmapFontRendererLayout, vk::PipelineBindPoint::eGraphics);

                cb.pushConstants<unsigned char>(sharedRenderingResources.m_bitmapFontRendererLayout.vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, fontPushConstantData2);
                cb.draw(4, shapedText.size(), 0, 0);
            }
        );
    }

    cb.endRendering();

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(transientRenderingResources.finalDrawBuffer(),
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits2::eBlit, vk::AccessFlagBits2::eTransferRead
        )
        .insertImageMemoryBarrier(swapchainImage,
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eAllCommands, vk::AccessFlagBits2::eNone,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eBlit, vk::AccessFlagBits2::eTransferWrite
        )
        .flush(m_frameRenderingResources.commandBuffer());

    auto blitSubres = vk::ImageSubresourceLayers{}
        .setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setMipLevel(0);

    auto srcOffsets = std::array<vk::Offset3D, 2>{{{0, 0, 0}, {toOffset3D(transientRenderingResources.finalDrawBuffer().extent())}}};

    auto blit = vk::ImageBlit2{}
        .setSrcSubresource(blitSubres)
        .setDstSubresource(blitSubres)
        .setSrcOffsets(srcOffsets)
        .setDstOffsets(srcOffsets);

    auto blitInfo = vk::BlitImageInfo2{}
        .setSrcImage(transientRenderingResources.finalDrawBuffer().vkImage())
        .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setDstImage(swapchainImage.vkImage())
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setFilter(vk::Filter::eLinear)
        .setRegions(blit);

    cb.blitImage2(blitInfo);

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(swapchainImage,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eBlit, vk::AccessFlagBits2::eTransferWrite,
            vk::ImageLayout::ePresentSrcKHR, vk::PipelineStageFlagBits2::eAllCommands, vk::AccessFlagBits2::eNone
        )
        .flush(m_frameRenderingResources.commandBuffer());

    cb.end();

    VulkanContext::get().vkQueueGraphics().submitOneCommandBufferWithBinarySemaphores(
        cb,
        {}, {},
        m_frameRenderingResources.imageAcquiredSemaphore(), swapchainImage.vkSemaphoreImagePresent(),
        vk::PipelineStageFlagBits2::eBlit, vk::PipelineStageFlagBits2::eBlit,
        m_frameRenderingResources.frameDoneFence()
    );

    VulkanContext::get().antiLagPacePresent(renderingData.m_frameIndex, 0);
    auto presentResult = VulkanContext::get().vkQueuePresent().submitPresent(swapchain, swapchainImage.vkSemaphoreImagePresent(), *imageAcquire.first);

    if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR) {
        return { .shouldRecreateSwapchain = true, .stepPerFrameResources = true };
    }

    return { .shouldRecreateSwapchain = false, .stepPerFrameResources = true };
}

} // namespace nekomata2::graphics