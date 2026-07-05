module;
#include <string.h>
module projnekomata;
import vulkan;
import vk_mem_alloc;
import :graphics.vulkan.context;
import :graphics.vulkan.vk_queue_family_swizzling;
import :graphics.rendering.frame_rendering_resources;

namespace projnekomata::graphics {

FrameRenderingResources::FrameRenderingResources(std::nullptr_t) {  }


FrameRenderingResources::FrameRenderingResources(u32 initialMaxObjects) {
    m_commandPool = VulkanCommandPool::createForGraphics(true);
    m_commandBuffer = m_commandPool.allocateCommandBuffer(vk::CommandBufferLevel::ePrimary);


    auto queuesForBuffer = VulkanContext::get().vkPhysicalDeviceProps().m_queueFamilies[QueueFamily::Graphics];
    m_transformsBuffer = VulkanBuffer::create(initialMaxObjects * 2048, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VulkanBufferMemoryMapping::MapForSequentialWrite, vma::MemoryUsage::eAutoPreferDevice, vk::MemoryPropertyFlagBits::eHostVisible, queuesForBuffer);
    m_globalDataBuffer = VulkanBuffer::create(sizeof(RenderingGlobalData), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VulkanBufferMemoryMapping::MapForSequentialWrite, vma::MemoryUsage::eAutoPreferDevice, vk::MemoryPropertyFlagBits::eHostVisible, queuesForBuffer);

    m_frameDoneFence = VulkanFence::create(true);
    m_imageAcquiredSemaphore = VulkanBinarySemaphore::create();
}

auto FrameRenderingResources::prepareTransformsBuffer(MRThreadsSharedDataLeaf& renderingData, ecs::components::Camera camera, const ecs::components::Transform& cameraTransform, float renderAspectRatio) -> void {
    auto projectionMatrix = camera.computeProjectionMatrix(renderAspectRatio);
    auto cameraModelMatrix = cameraTransform.m_transform3d.computeModelMatrix();
    auto viewMatrix = cameraModelMatrix.inverse().unwrapOr(math::Matrix4x4f::identity());

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
            .normalMatrix = normalMatrix,
        };

        memcpy(m_transformsBuffer.memoryHostPtr() + i * sizeof(Transforms), &transforms, sizeof(Transforms));
    }

    // ---- Global Data ----------------------------------------------------------------------------------------------------------------------------------------

    auto globdata = RenderingGlobalData {
        .projview = projectionMatrix * viewMatrix
    };

    memcpy(m_globalDataBuffer.memoryHostPtr(), &globdata, sizeof(RenderingGlobalData));
}

} // namespace projnekomata::graphics