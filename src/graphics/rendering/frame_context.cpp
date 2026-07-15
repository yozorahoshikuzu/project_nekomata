module;
#include <string.h>
module projnekomata;
import projnekomata.cs;
import vulkan;
import vk_mem_alloc;
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

namespace projnekomata::graphics {

using namespace projnekomata::math;

FrameContext::FrameContext(std::nullptr_t) {  }
FrameContext::FrameContext() {
    m_frameRenderingResources = FrameRenderingResources(2048);

    m_timestampsQueryPool = VulkanQueryPool::create(vk::QueryType::eTimestamp, 6, {});
    m_pipelineStatisticsQueryPool = VulkanQueryPool::create(vk::QueryType::ePipelineStatistics, 1,
        vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations
            | vk::QueryPipelineStatisticFlagBits::eTessellationControlShaderPatches
            | vk::QueryPipelineStatisticFlagBits::eTessellationEvaluationShaderInvocations
            | vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations
    );

}
auto FrameContext::waitForLastFrame() -> void {
    m_frameRenderingResources.frameDoneFence().waitForSignal(std::numeric_limits<u64>::max());
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

auto FrameContext::execute(TransientRenderingResources& transientRenderingResources, SharedRenderingResources& sharedRenderingResources, VulkanSwapchain& swapchain,
    MRThreadsSharedDataLeaf& renderingData, bool recordStatistics) -> FrameResult {
    auto imageAcquire = swapchain.acquireNextImage(std::numeric_limits<u64>::max(), m_frameRenderingResources.imageAcquiredSemaphore());

    if (imageAcquire.first.isNone() || imageAcquire.second) {
        return { .shouldRecreateSwapchain = true, .stepPerFrameResources = false };
    }

    m_frameRenderingResources.frameDoneFence().reset();
    sharedRenderingResources.refitHysteresisStates(renderingData.m_renderables.m_sparseToStorage.size());
    m_numDrawcalls = 0;
    m_queryPoolsHaveResultsOnFinish = recordStatistics;

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
        log::warn("No camera found! Will use a default one");
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

    m_frameRenderingResources.prepareBuffers(renderingData, firstCamera, firstCameraTransform, aspectRatio, renderingData.m_frameIndex);
    auto& swapchainImage = swapchain.imageAtIndex(imageAcquire.first.unwrap());

    m_frameRenderingResources.commandPool().reset();

    auto& cb = m_frameRenderingResources.commandBuffer().vkCommandBuffer();

    auto beginInfo = vk::CommandBufferBeginInfo{}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    vkCheckResult(cb.begin(beginInfo));

    if (recordStatistics) {
        cb.resetQueryPool(m_timestampsQueryPool.vkQueryPool(), 0, m_timestampsQueryPool.queryCount());
        cb.resetQueryPool(m_pipelineStatisticsQueryPool.vkQueryPool(), 0, m_pipelineStatisticsQueryPool.queryCount());
    }

    // ---- Font Rasterization ---------------------------------------------------------------------------------------------------------------------------------

    VulkanBuffer stagingBuffer = nullptr;

    // see if there are new glyphs to rasterize in the system text..
    auto all_texts_iter = renderingData.m_uiDrawCmds.iter()
        .filterMap([&](const auto& x) -> Option<fonts::FontRasterBatch> {
            if (!matches<ui::UiTextDrawCmd>(x)) return None;
            auto cmd = acquireInto<ui::UiTextDrawCmd>(x);
            auto batch = fonts::FontManager::get().findAndBatchMissingGlyphs(cmd.face, sharedRenderingResources.m_fontAtlas, cmd.text, cmd.size);
            return batch;
        })
        .collect<Vec>();

    if (!all_texts_iter.isEmpty()) {
        auto pixelBuffer = Vec<u8>::create();
        auto newImageIndices = Vec<u32>::create();
        auto bufferImageCopyRegions = HashMap<u32, Vec<vk::BufferImageCopy2>>::create();
        fonts::FontRasterInfo rasterInfo = { all_texts_iter.asSlice(), sharedRenderingResources.m_fontAtlas, bufferImageCopyRegions, pixelBuffer, newImageIndices };
        fonts::FontManager::get().rasterizeGlyphs(rasterInfo);

        // there can be glyphs that don't rasterize to anything but appeared in the batch, so the buffer might be zero-sized
        if (pixelBuffer.len() != 0) {
            stagingBuffer = VulkanBuffer::create(pixelBuffer.len(), vk::BufferUsageFlagBits::eTransferSrc, VulkanBufferMemoryMapping::MapForSequentialWrite, vma::MemoryUsage::eAutoPreferDevice, {}, VulkanContext::get().vkPhysicalDeviceProps().m_queueFamilies[QueueFamily::Graphics]);
            memcpy(stagingBuffer.memoryHostPtr(), pixelBuffer.data(), pixelBuffer.len());

            // Prepare for copy
            auto barriers = VulkanPipelineBarriers::builder();
            for (const auto& atlasImageIndex : bufferImageCopyRegions.keys()) {
                // For images that are newly created, transition from eUndefined instead
                if (newImageIndices.contains(atlasImageIndex)) {
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

    auto vkRenderingArea = vk::Extent2D{transientRenderingResources.finalDrawBuffer().extent().width, transientRenderingResources.finalDrawBuffer().extent().height};
    auto viewport = vk::Viewport{}
        .setWidth(static_cast<f32>(vkRenderingArea.width))
        .setHeight(static_cast<f32>(vkRenderingArea.height))
        .setMinDepth(0.0)
        .setMaxDepth(1.0);
    auto scissor = vk::Rect2D{}.setExtent(vkRenderingArea);

    // ---- Deferred Geometry Stage ----------------------------------------------------------------------------------------------------------------------------

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(transientRenderingResources.albedoAndRoughnessBuffer(),
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eFragmentShader, {},
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite
        )
        .insertImageMemoryBarrier(transientRenderingResources.normalBuffer(),
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eFragmentShader, {},
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite
        )
        .insertImageMemoryBarrier(transientRenderingResources.metallicAndAoBuffer(),
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eFragmentShader, {},
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite
        )
        .insertImageMemoryBarrier(transientRenderingResources.depthBuffer(),
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eFragmentShader, {},
            vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::AccessFlagBits2::eDepthStencilAttachmentWrite
        )
        .flush(m_frameRenderingResources.commandBuffer());

    if (recordStatistics) {
        cb.writeTimestamp2(vk::PipelineStageFlagBits2::eTopOfPipe, m_timestampsQueryPool.vkQueryPool(), 0);
        cb.beginQuery(m_pipelineStatisticsQueryPool.vkQueryPool(), 0, {});
    }

    auto albedoAndRoughnessAttachmentInfo = vk::RenderingAttachmentInfo{}
        .setImageView(transientRenderingResources.albedoAndRoughnessBuffer().vkImageViewWholeSize())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStoreOp(vk::AttachmentStoreOp::eStore);
    auto normalAttachmentInfo = vk::RenderingAttachmentInfo{}
        .setImageView(transientRenderingResources.normalBuffer().vkImageViewWholeSize())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStoreOp(vk::AttachmentStoreOp::eStore);
    auto metallicAndAoAttachmentInfo = vk::RenderingAttachmentInfo{}
        .setImageView(transientRenderingResources.metallicAndAoBuffer().vkImageViewWholeSize())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStoreOp(vk::AttachmentStoreOp::eStore);
    auto depthAttachmentInfo = vk::RenderingAttachmentInfo{}
        .setImageView(transientRenderingResources.depthBuffer().vkImageViewWholeSize())
        .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearDepthStencilValue{}.setDepth(0.0f));

    auto colorAttachments = std::array<vk::RenderingAttachmentInfo, 3>{albedoAndRoughnessAttachmentInfo, normalAttachmentInfo, metallicAndAoAttachmentInfo};
    auto deferredGeomRenderingInfo = vk::RenderingInfo{}
        .setColorAttachments(colorAttachments)
        .setPDepthAttachment(&depthAttachmentInfo)
        .setLayerCount(1)
        .setRenderArea(vk::Rect2D{}.setExtent(vkRenderingArea));

    cb.beginRendering(deferredGeomRenderingInfo);
    cb.setViewport(0, viewport);
    cb.setScissor(0, scissor);

    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, sharedRenderingResources.m_mainGeometryRenderPipeline.vkPipeline());
    texturesystem::TextureManager::get().shaderResourceTable().bindToCommandBuffer(m_frameRenderingResources.commandBuffer(), sharedRenderingResources.m_mainGeometryRenderLayout, vk::PipelineBindPoint::eGraphics);
    auto uboDeviceAddr = m_frameRenderingResources.transformsBuffer().memoryDevicePtr();
    auto globaldataAddr = m_frameRenderingResources.globalDataBuffer().memoryDevicePtr();

    struct RenderPushConstantData {
        vk::DeviceAddress objectUniformAddr;
        vk::DeviceAddress vertexbufferAddr;
        vk::DeviceAddress globaldataAddr;
        u32 textureId;
        u32 samplerId;
        float roughness;
        float metallic;
    };

    for (auto [i, renderable] : renderingData.m_renderables.m_storage.iter().enumerate()) {
        // Get the LOD list for the renderable and skip it if no LODs are available
        auto& lodList = meshsystem::MeshAssetStorage::get().getLodList(renderable.meshAsset);
        auto bestAvailableLod = lodList.bestLodIndex.load(std::memory_order_acquire);
        if (bestAvailableLod == ~0u) continue;

        // Copy its transforms addr to push constants
        vk::DeviceAddress uboFinalAddr = uboDeviceAddr + i * sizeof(Transforms);

        auto textureImageId = renderingData.m_textureToImageShaderIndexSnapshot[renderable.texture.index];
        auto textureSamplerId = renderingData.m_textureToSamplerShaderIndexSnapshot[renderable.texture.index];

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

        auto vboDeviceAddr = lod.meshSuballocation.vertexBuffer.deviceAddress;

        float roughness = static_cast<float>(i % 11) / 10.0f;
        float metallic = static_cast<float>(i / 11) / 10.0f;

        auto pushconstData = RenderPushConstantData {
            .objectUniformAddr = uboFinalAddr,
            .vertexbufferAddr = vboDeviceAddr,
            .globaldataAddr = globaldataAddr,
            .textureId = textureImageId,
            .samplerId = textureSamplerId,
            .roughness = roughness,
            .metallic = metallic,
        };

        cb.bindIndexBuffer(lod.meshSuballocation.indexBuffer.buffer, lod.meshSuballocation.indexBuffer.offset, vk::IndexType::eUint32);

        cb.pushConstants<RenderPushConstantData>(sharedRenderingResources.m_mainGeometryRenderLayout.vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eTessellationEvaluation, 0, pushconstData);
        cb.drawIndexed(lod.meshSuballocation.indexBuffer.size / sizeof(u32), 1, 0, 0, 0);
        m_numDrawcalls++;
    }

    cb.endRendering();

    if (recordStatistics) {
        cb.writeTimestamp2(vk::PipelineStageFlagBits2::eBottomOfPipe, m_timestampsQueryPool.vkQueryPool(), 1);
        cb.endQuery(m_pipelineStatisticsQueryPool.vkQueryPool(), 0);
    }

    // ---- Deferred Lighting Stage ----------------------------------------------------------------------------------------------------------------------------

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(transientRenderingResources.albedoAndRoughnessBuffer(),
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead
        )
        .insertImageMemoryBarrier(transientRenderingResources.normalBuffer(),
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead
        )
        .insertImageMemoryBarrier(transientRenderingResources.metallicAndAoBuffer(),
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead
        )
        .insertImageMemoryBarrier(transientRenderingResources.depthBuffer(),
            vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead
        )
        .insertImageMemoryBarrier(transientRenderingResources.colorBuffer(),
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eFragmentShader, {},
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite
        )
        .flush(m_frameRenderingResources.commandBuffer());

    if (recordStatistics) {
        cb.writeTimestamp2(vk::PipelineStageFlagBits2::eTopOfPipe, m_timestampsQueryPool.vkQueryPool(), 2);
    }

    u32 skyboxTextureId = renderingData.m_textureToImageShaderIndexSnapshot[sharedRenderingResources.m_skyCubemap.index];
    u32 irradianceTextureId = renderingData.m_textureToImageShaderIndexSnapshot[sharedRenderingResources.m_skyIrradianceCubemap.index];
    u32 prefilterTextureId = renderingData.m_textureToImageShaderIndexSnapshot[sharedRenderingResources.m_skyPrefilterCubemap.index];
    u32 iblLutTextureId = renderingData.m_textureToImageShaderIndexSnapshot[sharedRenderingResources.m_brdfLUT.index];
    u32 linearSamplerId = renderingData.m_textureToSamplerShaderIndexSnapshot[sharedRenderingResources.m_skyCubemap.index];
    u32 nearestSamplerId = texturesystem::TextureManager::get().samplerCache().acquireSampler(
        texturesystem::SamplerParams::defaultValues().setMinFilter(vk::Filter::eNearest).setMagFilter(vk::Filter::eNearest).setMipmapMode(vk::SamplerMipmapMode::eNearest).setMaxLod(0.0f)
    );

    auto drawImageAttachmentInfo = vk::RenderingAttachmentInfo{}
        .setImageView(transientRenderingResources.colorBuffer().vkImageViewWholeSize())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStoreOp(vk::AttachmentStoreOp::eStore);

    auto lightingstageRenderingInfo = vk::RenderingInfo{}
        .setColorAttachments(drawImageAttachmentInfo)
        .setLayerCount(1)
        .setRenderArea(vk::Rect2D{}.setExtent(vkRenderingArea));

    cb.beginRendering(lightingstageRenderingInfo);
    cb.setViewport(0, viewport);
    cb.setScissor(0, scissor);

    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, sharedRenderingResources.m_mainLightingPassPipeline.vkPipeline());
    texturesystem::TextureManager::get().shaderResourceTable().bindToCommandBuffer(m_frameRenderingResources.commandBuffer(), sharedRenderingResources.m_mainLightingPassLayout, vk::PipelineBindPoint::eGraphics);

    struct LightingStagePushConstantData {
        vk::DeviceAddress globaldataAddr;
        vk::DeviceAddress pointlightsAddr;
        u32 pointlightCount;
        u32 depthTextureIndex;
        u32 albedoAndRoughnessTextureIndex;
        u32 normalTextureIndex;
        u32 metallicAoTextureIndex;
        u32 skyboxTextureId;
        u32 irradianceTextureId;
        u32 prefiltTextureId;
        u32 iblLutTextureId;
        u32 linearSamplerIndex;
        u32 nearestSamplerIndex;
    };

    auto pushconstData = LightingStagePushConstantData {
        .globaldataAddr = globaldataAddr,
        .pointlightsAddr = m_frameRenderingResources.pointlightsBuffer().memoryDevicePtr(),
        .pointlightCount = static_cast<u32>(renderingData.m_pointlights.m_storage.len()),
        .depthTextureIndex = transientRenderingResources.depthBufferIndex().imageIndex,
        .albedoAndRoughnessTextureIndex = transientRenderingResources.albedoAndRoughnessBufferIndex().imageIndex,
        .normalTextureIndex = transientRenderingResources.normalBufferIndex().imageIndex,
        .metallicAoTextureIndex = transientRenderingResources.metallicAndAoBufferIndex().imageIndex,
        .skyboxTextureId = skyboxTextureId,
        .irradianceTextureId = irradianceTextureId,
        .prefiltTextureId = prefilterTextureId,
        .iblLutTextureId = iblLutTextureId,
        .linearSamplerIndex = linearSamplerId,
        .nearestSamplerIndex = nearestSamplerId
    };

    cb.pushConstants<LightingStagePushConstantData>(sharedRenderingResources.m_mainLightingPassLayout.vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushconstData);
    cb.draw(3, 1, 0, 0);

    if (recordStatistics) {
        cb.writeTimestamp2(vk::PipelineStageFlagBits2::eBottomOfPipe, m_timestampsQueryPool.vkQueryPool(), 3);
    }

    cb.endRendering();

    // ---- SMAA -----------------------------------------------------------------------------------------------------------------------------------------------

    if (recordStatistics) {
        cb.writeTimestamp2(vk::PipelineStageFlagBits2::eTopOfPipe, m_timestampsQueryPool.vkQueryPool(), 4);
    }

    auto smaaRtMetrics = Vector4f(1.0f / renderingArea.x(), 1.0f / renderingArea.y(), renderingArea.x(), renderingArea.y());

    auto smaaLinearSamplerSrtID = texturesystem::TextureManager::get().samplerCache().acquireSampler(
        texturesystem::SamplerParams::defaultValues()
            .setMinFilter(vk::Filter::eLinear)
            .setMagFilter(vk::Filter::eLinear)
            .setMipmapMode(vk::SamplerMipmapMode::eNearest)
            .setMaxLod(0.0f)
    );

    auto smaaNearestSamplerSrtID = texturesystem::TextureManager::get().samplerCache().acquireSampler(
        texturesystem::SamplerParams::defaultValues()
            .setMinFilter(vk::Filter::eNearest)
            .setMagFilter(vk::Filter::eNearest)
            .setMipmapMode(vk::SamplerMipmapMode::eNearest)
            .setMaxLod(0.0f)
    );

    u32 smaaAreaTextureSrtID = renderingData.m_textureToImageShaderIndexSnapshot[sharedRenderingResources.m_smaaAreaTexture.index];
    u32 smaaSearchTextureSrtID = renderingData.m_textureToImageShaderIndexSnapshot[sharedRenderingResources.m_smaaSearchTexture.index];

    // -------- [Stage 1] Edge detection

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(transientRenderingResources.colorBuffer(),
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead
        )
        .insertImageMemoryBarrier(transientRenderingResources.smaaEdgesImage(),
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eFragmentShader, {},
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite
        )
        .flush(m_frameRenderingResources.commandBuffer());


    auto edgesImageAttachmentInfo = vk::RenderingAttachmentInfo{}
        .setImageView(transientRenderingResources.smaaEdgesImage().vkImageViewWholeSize())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearColorValue{0.0f, 0.0f, 0.0f, 0.0f});

    auto smaaEdgeDetectionRenderingInfo = vk::RenderingInfo{}
        .setColorAttachments(edgesImageAttachmentInfo)
        .setLayerCount(1)
        .setRenderArea(vk::Rect2D{}.setExtent(vkRenderingArea));

    cb.beginRendering(smaaEdgeDetectionRenderingInfo);
    cb.setViewport(0, viewport);
    cb.setScissor(0, scissor);

    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, sharedRenderingResources.m_smaaEdgeDetectPipeline.vkPipeline());
    texturesystem::TextureManager::get().shaderResourceTable().bindToCommandBuffer(m_frameRenderingResources.commandBuffer(), sharedRenderingResources.m_smaaEdgeDetectLayout, vk::PipelineBindPoint::eGraphics);

    struct SmaaEdgeDetectionPushConstantData {
        Vector4f rtMetrics;
        u32 colorBufferSrtID;
        u32 depthBufferSrtID;
        u32 linearSamplerSrtID;
        u32 nearestSamplerSrtID;
    };

    auto smaaEdgeDetectionPushconstData = SmaaEdgeDetectionPushConstantData {
        .rtMetrics = smaaRtMetrics,
        .colorBufferSrtID = transientRenderingResources.colorBufferUnormViewIndex().imageIndex,
        .depthBufferSrtID = transientRenderingResources.depthBufferIndex().imageIndex,
        .linearSamplerSrtID = smaaLinearSamplerSrtID,
        .nearestSamplerSrtID = smaaNearestSamplerSrtID
    };

    cb.pushConstants<SmaaEdgeDetectionPushConstantData>(sharedRenderingResources.m_smaaEdgeDetectLayout.vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, smaaEdgeDetectionPushconstData);
    cb.draw(3, 1, 0, 0);

    cb.endRendering();

    // -------- [Stage 2] Blend weights

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(transientRenderingResources.smaaEdgesImage(),
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead
        )
        .insertImageMemoryBarrier(transientRenderingResources.smaaWeightsImage(),
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eFragmentShader, {},
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite
        )
        .flush(m_frameRenderingResources.commandBuffer());

    auto weightsImageAttachmentInfo = vk::RenderingAttachmentInfo{}
        .setImageView(transientRenderingResources.smaaWeightsImage().vkImageViewWholeSize())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearColorValue{0.0f, 0.0f, 0.0f, 0.0f});

    auto smaaBlendWeightsRenderingInfo = vk::RenderingInfo{}
        .setColorAttachments(weightsImageAttachmentInfo)
        .setLayerCount(1)
        .setRenderArea(vk::Rect2D{}.setExtent(vkRenderingArea));

    cb.beginRendering(smaaBlendWeightsRenderingInfo);
    cb.setViewport(0, viewport);
    cb.setScissor(0, scissor);

    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, sharedRenderingResources.m_smaaBlendWeightPipeline.vkPipeline());
    texturesystem::TextureManager::get().shaderResourceTable().bindToCommandBuffer(m_frameRenderingResources.commandBuffer(), sharedRenderingResources.m_smaaBlendWeightLayout, vk::PipelineBindPoint::eGraphics);

    struct SmaaBlendWeightPushConstantData {
        Vector4f rtMetrics;
        u32 edgesImageSrtID;
        u32 areaTextureSrtID;
        u32 searchTextureSrtID;
        u32 linearSamplerSrtID;
        u32 nearestSamplerSrtID;
    };

    auto smaaBlendWeightPushconstData = SmaaBlendWeightPushConstantData {
        .rtMetrics = smaaRtMetrics,
        .edgesImageSrtID = transientRenderingResources.smaaEdgesImageIndex().imageIndex,
        .areaTextureSrtID = smaaAreaTextureSrtID,
        .searchTextureSrtID = smaaSearchTextureSrtID,
        .linearSamplerSrtID = smaaLinearSamplerSrtID,
        .nearestSamplerSrtID = smaaNearestSamplerSrtID
    };

    cb.pushConstants<SmaaBlendWeightPushConstantData>(sharedRenderingResources.m_smaaBlendWeightLayout.vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, smaaBlendWeightPushconstData);
    cb.draw(3, 1, 0, 0);

    cb.endRendering();

    // -------- [Stage 3] Neighborhood blend

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(transientRenderingResources.smaaWeightsImage(),
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead
        )
        .insertImageMemoryBarrier(transientRenderingResources.finalDrawBuffer(),
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eBlit, {},
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite
        )
        .flush(m_frameRenderingResources.commandBuffer());

    auto finalDrawBufferUnormAttachmentInfo = vk::RenderingAttachmentInfo{}
        .setImageView(transientRenderingResources.finalDrawBufferUnormView().vkImageView())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearColorValue{0.0f, 0.0f, 0.0f, 0.0f});

    auto smaaNeighborhoodBlendRenderingInfo = vk::RenderingInfo{}
        .setColorAttachments(finalDrawBufferUnormAttachmentInfo)
        .setLayerCount(1)
        .setRenderArea(vk::Rect2D{}.setExtent(vkRenderingArea));

    cb.beginRendering(smaaNeighborhoodBlendRenderingInfo);
    cb.setViewport(0, viewport);
    cb.setScissor(0, scissor);

    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, sharedRenderingResources.m_smaaNeighborhoodBlendPipeline.vkPipeline());
    texturesystem::TextureManager::get().shaderResourceTable().bindToCommandBuffer(m_frameRenderingResources.commandBuffer(), sharedRenderingResources.m_smaaNeighborhoodBlendLayout, vk::PipelineBindPoint::eGraphics);

    struct SmaaNeighborhoodBlendPushConstantData {
        Vector4f rtMetrics;
        u32 colorBufferSrtID;
        u32 weightsImageSrtID;
        u32 linearSamplerSrtID;
        u32 nearestSamplerSrtID;
    };

    auto smaaNeighborhoodBlendPushconstData = SmaaNeighborhoodBlendPushConstantData {
        .rtMetrics = smaaRtMetrics,
        .colorBufferSrtID = transientRenderingResources.colorBufferUnormViewIndex().imageIndex,
        .weightsImageSrtID = transientRenderingResources.smaaWeightsImageIndex().imageIndex,
        .linearSamplerSrtID = smaaLinearSamplerSrtID,
        .nearestSamplerSrtID = smaaNearestSamplerSrtID
    };

    cb.pushConstants<SmaaNeighborhoodBlendPushConstantData>(sharedRenderingResources.m_smaaNeighborhoodBlendLayout.vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, smaaNeighborhoodBlendPushconstData);
    cb.draw(3, 1, 0, 0);

    cb.endRendering();

    if (recordStatistics) {
        cb.writeTimestamp2(vk::PipelineStageFlagBits2::eBottomOfPipe, m_timestampsQueryPool.vkQueryPool(), 5);
    }

    // ---- UI -------------------------------------------------------------------------------------------------------------------------------------------------

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(transientRenderingResources.finalDrawBuffer(),
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite
        )
        .flush(m_frameRenderingResources.commandBuffer());

    auto finalDrawBufferAttachmentInfo = vk::RenderingAttachmentInfo{}
        .setImageView(transientRenderingResources.finalDrawBuffer().vkImageViewWholeSize())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eLoad)
        .setStoreOp(vk::AttachmentStoreOp::eStore);

    auto uiRenderingInfo = vk::RenderingInfo{}
        .setColorAttachments(finalDrawBufferAttachmentInfo)
        .setLayerCount(1)
        .setRenderArea(vk::Rect2D{}.setExtent(vkRenderingArea));

    cb.beginRendering(uiRenderingInfo);
    cb.setViewport(0, viewport);
    cb.setScissor(0, scissor);

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

                u32 sampler2 = texturesystem::TextureManager::get().samplerCache().acquireSampler(
                    texturesystem::SamplerParams::defaultValues().setMinFilter(vk::Filter::eNearest).setMagFilter(vk::Filter::eNearest).setMipmapMode(vk::SamplerMipmapMode::eNearest).setMaxLod(0.0f)
                );
                auto instanceDevicePtr2 = buffer.memoryDevicePtr();

                auto col = drawCmd.color.asRgba32Float();
                auto fontPushConstantData2 = std::array<unsigned char, 28>{};
                memcpy(fontPushConstantData2.data(), &instanceDevicePtr2, 8);
                memcpy(fontPushConstantData2.data() + 8, &sampler2, 4);
                memcpy(fontPushConstantData2.data() + 12, &col, 16);
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

    vkCheckResult(cb.end());

    VulkanContext::get().vkQueueGraphics().submitOneCommandBufferWithBinarySemaphores(
        cb,
        {}, {},
        m_frameRenderingResources.imageAcquiredSemaphore(), swapchainImage.vkSemaphoreImagePresent(),
        vk::PipelineStageFlagBits2::eBlit, vk::PipelineStageFlagBits2::eBlit,
        Some(std::ref(m_frameRenderingResources.frameDoneFence()))
    );

    VulkanContext::get().antiLagPacePresent(renderingData.m_frameIndex, 0);
    auto presentResult = VulkanContext::get().vkQueuePresent().submitPresent(swapchain, swapchainImage.vkSemaphoreImagePresent(), imageAcquire.first.unwrap());

    if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR) {
        return { .shouldRecreateSwapchain = true, .stepPerFrameResources = true };
    }

    return { .shouldRecreateSwapchain = false, .stepPerFrameResources = true };
}

} // namespace projnekomata::graphics