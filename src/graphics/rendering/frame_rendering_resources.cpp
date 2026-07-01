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
    m_glyphInstanceBuffer = VulkanBuffer::create(160000, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VulkanBufferMemoryMapping::MapForSequentialWrite, vma::MemoryUsage::eAutoPreferDevice, {}, queuesForBuffer);

    m_frameDoneFence = VulkanFence::create(true);
    m_imageAcquiredSemaphore = VulkanBinarySemaphore::create();
}

auto FrameRenderingResources::prepareTransformsBuffer(MRThreadsSharedDataLeaf& renderingData, ecs::components::Camera camera, const ecs::components::Transform& cameraTransform, float renderAspectRatio) -> void {
    auto projectionMatrix = camera.computeProjectionMatrix(renderAspectRatio);
    auto cameraModelMatrix = cameraTransform.m_transform3d.computeModelMatrix();
    auto viewMatrix = cameraModelMatrix.inverse().unwrapOr(math::Matrix4x4f::identity());

    for (auto [i, rend] : renderingData.m_renderables.m_storage.iter().enumerate()) {
        auto entSparseIndex = renderingData.m_renderables.m_storageToEntity[i];

        auto modelMatrix = math::Matrix4x4f::identity();
        if (renderingData.m_transforms.containsEntity(entSparseIndex)) {
            modelMatrix = renderingData.m_transforms.get(entSparseIndex).m_transform3d.computeModelMatrix();
        }

        auto pvm = projectionMatrix * viewMatrix * modelMatrix;
        memcpy(m_transformsBuffer.memoryHostPtr() + i * sizeof(pvm), &pvm, sizeof(pvm));
    }
}

} // namespace projnekomata::graphics