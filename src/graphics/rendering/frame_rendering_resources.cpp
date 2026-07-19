module;
#include <string.h>
module projnekomata;
import vulkan;
import vk_mem_alloc;
import :graphics.vulkan.context;
import :graphics.vulkan.vk_queue_family_swizzling;
import :graphics.rendering.frame_rendering_resources;
import :graphics.shaders.globallayout;

namespace projnekomata::graphics {

FrameRenderingResources::FrameRenderingResources(std::nullptr_t) {  }


FrameRenderingResources::FrameRenderingResources(u32 initialMaxObjects) {
    m_commandPool = VulkanCommandPool::createForGraphics(true);
    m_commandBuffer = m_commandPool.allocateCommandBuffer(vk::CommandBufferLevel::ePrimary);


    auto queuesForBuffer = VulkanContext::get().vkPhysicalDeviceProps().m_queueFamilies[QueueFamily::Graphics];
    m_transformsBuffer = VulkanBuffer::create(initialMaxObjects * 2048, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VulkanBufferMemoryMapping::MapForSequentialWrite, vma::MemoryUsage::eAutoPreferDevice, vk::MemoryPropertyFlagBits::eHostVisible, queuesForBuffer);
    m_globalDataBuffer = VulkanBuffer::create(sizeof(ShGlobalData), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VulkanBufferMemoryMapping::MapForSequentialWrite, vma::MemoryUsage::eAutoPreferDevice, vk::MemoryPropertyFlagBits::eHostVisible, queuesForBuffer);
    m_pointlightsBuffer = VulkanBuffer::create(1024 * sizeof(PointlightData), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VulkanBufferMemoryMapping::MapForSequentialWrite, vma::MemoryUsage::eAutoPreferDevice, vk::MemoryPropertyFlagBits::eHostVisible, queuesForBuffer);

    m_frameDoneFence = VulkanFence::create(true);
    m_imageAcquiredSemaphore = VulkanBinarySemaphore::create();
}

auto FrameRenderingResources::prepareBuffers(MRThreadsSharedDataLeaf& renderingData, SharedRenderingResources& sharedRendResources, ecs::components::Camera camera, const ecs::components::Transform& cameraTransform, float renderAspectRatio, u64 frameIndex) -> void {
    auto projectionMatrix = camera.computeProjectionMatrix(renderAspectRatio);
    auto cameraModelMatrix = cameraTransform.m_transform3d.computeModelMatrix();
    auto viewMatrix = cameraModelMatrix.inverse().unwrapOr(math::Matrix4x4f::identity());
    auto viewportSize = Vector2f(renderingData.m_currentWindowExtent.width, renderingData.m_currentWindowExtent.height);

    // ---- Per-Object Data ------------------------------------------------------------------------------------------------------------------------------------

    for (auto [i, rend] : renderingData.m_renderables.m_storage.iter().enumerate()) {
        auto entSparseIndex = renderingData.m_renderables.m_storageToEntity[i];

        auto modelMatrix = math::Matrix4x4f::identity();
        if (renderingData.m_transforms.containsEntity(entSparseIndex)) {
            modelMatrix = renderingData.m_transforms.get(entSparseIndex).m_transform3d.computeModelMatrix();
        }

        auto normalMatrixPrec = Matrix3x3f({
            modelMatrix[0, 0], modelMatrix[0, 1], modelMatrix[0, 2],
            modelMatrix[1, 0], modelMatrix[1, 1], modelMatrix[1, 2],
            modelMatrix[2, 0], modelMatrix[2, 1], modelMatrix[2, 2],
        });
        auto normalMatrix = normalMatrixPrec.inverse().unwrapOr(Matrix3x3f::identity()).transpose();

        auto transforms = Transforms {
            .model = modelMatrix,
            .prevModel = sharedRendResources.getLastRenderableModelMatrix(entSparseIndex.index()),
            .normalMatrix = normalMatrix,
        };
        sharedRendResources.getLastRenderableModelMatrix(entSparseIndex.index()) = modelMatrix;

        memcpy(m_transformsBuffer.memoryHostPtr() + i * sizeof(Transforms), &transforms, sizeof(Transforms));
    }

    // ---- Pointlights ----------------------------------------------------------------------------------------------------------------------------------------

    for (auto [i, light] : renderingData.m_pointlights.m_storage.iter().enumerate()) {
        auto entSparseIndex = renderingData.m_pointlights.m_storageToEntity[i];

        auto position = Vector3f(0.0f);
        if (renderingData.m_transforms.containsEntity(entSparseIndex)) {
            position = renderingData.m_transforms.get(entSparseIndex).m_transform3d.m_position;
        }

        auto pointlightData = PointlightData {
            .position = position,
            .lightRadiance = light.lightRadiance
        };

        memcpy(m_pointlightsBuffer.memoryHostPtr() + i * sizeof(PointlightData), &pointlightData, sizeof(PointlightData));
    }

    // ---- Global Data ----------------------------------------------------------------------------------------------------------------------------------------

    static Vector2f smaat2xJitterPattern[2] = {
        Vector2f( 0.25f, -0.25f),
        Vector2f(-0.25f,  0.25f)
    };

    auto jitter = smaat2xJitterPattern[frameIndex % 2];
    auto jitterOffset = 2.0f * jitter.componentWiseDivide(viewportSize);

    auto jitteredProj = projectionMatrix;
    jitteredProj[0, 2] += jitterOffset.x();
    jitteredProj[1, 2] += jitterOffset.y();

    auto projview = projectionMatrix * viewMatrix;
    auto jitteredProjview = jitteredProj * viewMatrix;

    auto camModelMatrixNoTranslation = cameraModelMatrix;
    camModelMatrixNoTranslation[0, 3] = 0.0f;
    camModelMatrixNoTranslation[1, 3] = 0.0f;
    camModelMatrixNoTranslation[2, 3] = 0.0f;
    auto viewMatrixNoTranslation = camModelMatrixNoTranslation.inverse().unwrapOr(Matrix4x4f::identity());

    auto projviewNoTranslation = projectionMatrix * viewMatrixNoTranslation;
    auto projviewNoTranslationInverse = projviewNoTranslation.inverse().unwrapOr(Matrix4x4f::identity());

    auto globdata = ShGlobalData {
        .jitteredProjview = jitteredProjview,
        .projview = projview,
        .prevProjview = sharedRendResources.m_lastProjview,
        .prevProjviewNoTranslation = sharedRendResources.m_lastProjviewNoTranslation,
        .projviewInverse = projview.inverse().unwrapOr(Matrix4x4f::identity()),
        .projviewNoTranslationInverse = projviewNoTranslationInverse,
        .cameraPos = cameraTransform.m_transform3d.m_position,
        .frameIndex = static_cast<u32>(frameIndex)
    };
    sharedRendResources.m_lastProjview = projview;
    sharedRendResources.m_lastProjviewNoTranslation = projviewNoTranslation;

    memcpy(m_globalDataBuffer.memoryHostPtr(), &globdata, sizeof(ShGlobalData));
}

} // namespace projnekomata::graphics