#include "frame_context.hpp"

#include "core/log/log.hpp"
#include "core/math/transform3d.hpp"
#include "core/overloaded/overloaded.hpp"
#include "graphics/fontsystem/font_manager.hpp"
#include "graphics/vulkan/vk_commands_barriers.hpp"
#include "graphics/vulkan/vk_queue.hpp"

#include <format>
#include <random>

namespace nekomata2::graphics {

FrameContext::FrameContext(std::nullptr_t) {  }
FrameContext::FrameContext() {
    m_frameRenderingResources = FrameRenderingResources(2048);
}

inline vk::Offset3D toOffset3D(const vk::Extent3D& extent) {
    return vk::Offset3D{
        static_cast<int32_t>(extent.width),
        static_cast<int32_t>(extent.height),
        static_cast<int32_t>(extent.depth)
    };
}

auto FrameContext::execute(TransientRenderingResources& transientRenderingResources, SharedRenderingResources& sharedRenderingResources, VulkanSwapchain& swapchain, MRThreadsSharedDataLeaf& renderingData, float timeSinceStart) -> FrameResult {
    m_frameRenderingResources.frameDoneFence().waitForSignal(UINT64_MAX);

    auto imageAcquire = swapchain.acquireNextImage(UINT64_MAX, m_frameRenderingResources.imageAcquiredSemaphore());

    if (!imageAcquire.first.has_value() || imageAcquire.second) {
        return { .shouldRecreateSwapchain = true, .stepPerFrameResources = false };
    }

    m_frameRenderingResources.frameDoneFence().reset();
    sharedRenderingResources.refitHysteresisStates(renderingData.m_renderables.m_sparseToStorage.size());

    // ---------------------------------------------------------------- Render Pass starts here ----------------------------------------------------------------

    ecs::components::Camera firstCamera;
    ecs::components::Transform firstCameraTransform;
    bool firstCameraFound = false;

    for (auto [i, camera] : renderingData.m_cameras.m_storage | std::ranges::views::enumerate) {
        firstCamera = camera;

        auto cameraEntitySparseIndex = renderingData.m_cameras.m_storageToEntity[i];;

        if (renderingData.m_transforms.containsEntity(cameraEntitySparseIndex)) {
            firstCameraTransform = renderingData.m_transforms.get(cameraEntitySparseIndex);
        } else {
            firstCameraTransform = ecs::components::Transform{};
            firstCameraTransform.m_transform3d = Transform3D::identity();
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
        firstCameraTransform.m_transform3d = Transform3D::identity();
        firstCameraTransform.m_transform3d.m_position = Vector3f(0.0f, 0.0f, 1.2f);
    }

    Vector2f renderingArea = Vector2f(static_cast<f32>(transientRenderingResources.finalDrawBuffer().extent().width), static_cast<f32>(transientRenderingResources.finalDrawBuffer().extent().height));

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

    // std::random_device rd;
    // std::mt19937 gen(rd());
    // std::uniform_int_distribution<int> scriptDist(8, 30);
    // u32 pixelSize = scriptDist(gen);
    u32 pixelSize = 18;

        sharedRenderingResources.m_textToDisplay = std::format(":3\n\n  Project Nekomata\n\n  Device Name: {}\n  Device VRAM: {} MiB\n\n  FPS: {:.2f}\n  Frame Time: {:.5f} ms",
            VulkanContext::get().vkPhysicalDeviceProps().m_deviceName,
            VulkanContext::get().vkPhysicalDeviceProps().m_vramSize / (1024 * 1024),
            1000.0f / sharedRenderingResources.displayMs,
            sharedRenderingResources.displayMs,
            renderingArea.x(),
            renderingArea.y()
        );

    VulkanBuffer stagingBuffer = nullptr;

    // see if there are new glyphs to rasterize..
    auto batch = fonts::FontManager::get().findAndBatchMissingGlyphs(sharedRenderingResources.m_fontFace, sharedRenderingResources.m_fontAtlas, sharedRenderingResources.m_textToDisplay, pixelSize);
    if (batch.has_value()) {
        auto batches = std::vector<fonts::FontRasterBatch>();
        batches.emplace_back(std::move(*batch));

        std::vector<u8> resultBuffer;
        std::vector<u32> newImageIndices;
        std::unordered_map<u32, std::vector<vk::BufferImageCopy2>> bufferImageCopyRegions;
        fonts::FontRasterInfo rasterInfo = { batches, sharedRenderingResources.m_fontAtlas, bufferImageCopyRegions, resultBuffer, newImageIndices  };

        fonts::FontManager::get().rasterizeGlyphs(rasterInfo);

        // There can be empty glyphs we don't care about in which case the size of the buffer will be 0
        if (resultBuffer.size() != 0) {

        stagingBuffer = VulkanBuffer::create(resultBuffer.size(), vk::BufferUsageFlagBits::eTransferSrc, VulkanBufferMemoryMapping::MapForSequentialWrite, vma::MemoryUsage::eAutoPreferDevice, {}, VulkanContext::get().vkPhysicalDeviceProps().m_queueFamilies[QueueFamily::Graphics]);
        memcpy(stagingBuffer.memoryHostPtr(), resultBuffer.data(), resultBuffer.size());

        // Prepare for copy
        auto barriers = VulkanPipelineBarriers::builder();
        for (const auto& atlasImageIndex : bufferImageCopyRegions | std::views::keys) {
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
        for (const auto& [atlasImageIndex, regions] : bufferImageCopyRegions) {
            auto copyInfo = vk::CopyBufferToImageInfo2{}
                .setDstImage(sharedRenderingResources.m_fontAtlas.m_atlasTextures[atlasImageIndex].image.vkImage())
                .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
                .setSrcBuffer(stagingBuffer.vkBuffer())
                .setRegions(regions);

            cb.copyBufferToImage2(copyInfo);
        }

        // Prepare for usage
        auto barriers2 = VulkanPipelineBarriers::builder();
        for (const auto& atlasImageIndex : bufferImageCopyRegions | std::views::keys) {
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
    for (auto [i, renderable] : renderingData.m_renderables.m_storage | std::ranges::views::enumerate) {
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

        cb.pushConstants<unsigned char>(sharedRenderingResources.simpleLayout().vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstantData);
        cb.drawIndexed(lod.meshSuballocation.indexBuffer.size / sizeof(u32), 1, 0, 0, 0);
        verticesThisDraw += lod.meshSuballocation.indexBuffer.size / sizeof(u32);

        shouldInvertColors = shouldInvertColors == 0 ? 1 : 0;
    }

    // Draw the text (finally...)

    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, sharedRenderingResources.m_bitmapFontRendererPipeline.vkPipeline());
    texturesystem::TextureManager::get().shaderResourceTable().bindToCommandBuffer(m_frameRenderingResources.commandBuffer(), sharedRenderingResources.m_bitmapFontRendererLayout, vk::PipelineBindPoint::eGraphics);

    auto shapedText = fonts::FontManager::get().shapeText(sharedRenderingResources.m_fontFace, sharedRenderingResources.m_fontAtlas, sharedRenderingResources.m_textToDisplay, pixelSize, Vector2f(5.0, 20.0), renderingArea);
    memcpy(m_frameRenderingResources.glyphInstanceBuffer().memoryHostPtr(), shapedText.data(), shapedText.size() * sizeof(fonts::GlyphInstance));

    auto fontPushConstantData = std::array<unsigned char, 12>{};

    u32 sampler = texturesystem::TextureManager::get().samplerCache().acquireSampler(
        texturesystem::SamplerParams::defaultValues().setMinFilter(vk::Filter::eNearest).setMagFilter(vk::Filter::eNearest).setMipmapMode(vk::SamplerMipmapMode::eNearest).setMaxLod(0.0f)
    );
    auto instanceDevicePtr = m_frameRenderingResources.glyphInstanceBuffer().memoryDevicePtr();
    memcpy(fontPushConstantData.data(), &instanceDevicePtr, 8);
    memcpy(fontPushConstantData.data() + 8, &sampler, 4);

    cb.pushConstants<unsigned char>(sharedRenderingResources.m_bitmapFontRendererLayout.vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, fontPushConstantData);
    cb.draw(4, shapedText.size(), 0, 0);

    // Draw UI

    for (const auto& uiDrawCmd : renderingData.m_uiDrawCmds) {
        std::visit(overloaded{
            [&](const ui::UiRectDrawCmd& drawCmd) {
                cb.bindPipeline(vk::PipelineBindPoint::eGraphics, sharedRenderingResources.m_uiRectRendererPipeline.vkPipeline());
                cb.pushConstants<ui::UiRectDrawCmd>(sharedRenderingResources.m_uiRectRendererLayout.vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, drawCmd);
                cb.draw(4, 1, 0, 0);
            },
            [&](const ui::UiTextureDrawCmd& drawCmd) {
                struct PushConstants {
                    Vector2f ndcBegin, ndcEnd, texcoordBegin, texcoordEnd;
                    uint32_t textureIndex;
                    uint32_t samplerIndex;
                };

                uint32_t textureIndex = renderingData.m_textureToImageShaderIndexSnapshot[drawCmd.texture.index];
                uint32_t samplerIndex = renderingData.m_textureToSamplerShaderIndexSnapshot[drawCmd.texture.index];

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
            [&](auto&) {
                log::warn("Unsupported UI draw command in UI draw command list!");
            }
        }, uiDrawCmd);
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

    auto presentResult = VulkanContext::get().vkQueuePresent().submitPresent(swapchain, swapchainImage.vkSemaphoreImagePresent(), *imageAcquire.first);

    if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR) {
        return { .shouldRecreateSwapchain = true, .stepPerFrameResources = true };
    }

    return { .shouldRecreateSwapchain = false, .stepPerFrameResources = true };
}

} // namespace nekomata2::graphics