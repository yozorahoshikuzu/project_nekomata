export module nekomata2.graphics.rendering.frame_rendering_resources;
import std;
import nekomata2.core.platform.int_def;
import nekomata2.graphics.vulkan.vk_commands;
import nekomata2.graphics.vulkan.vk_buffer;
import nekomata2.graphics.vulkan.sync_primitives.fence;
import nekomata2.graphics.vulkan.sync_primitives.binary_semaphore;
import nekomata2.core.runtime.shared_data;
import nekomata2.core.ecs.world.camera;
import nekomata2.core.ecs.world.transform;

export namespace nekomata2::graphics {

/// Per-frame rendering resources used exclusively by each frame and only by that frame.
///
/// # Access Contingency
/// | CPU Read/Write | GPU Access Scope across Queue | GPU Visibility Scope across Queue |
/// |----------------|-------------------------------|-----------------------------------|
/// | Yes            | Exclusive                     | Exclusive                         |
///
class FrameRenderingResources {
public:
    FrameRenderingResources(std::nullptr_t);
    FrameRenderingResources(u32 initialMaxObjects);


    auto commandPool() -> VulkanCommandPool& { return m_commandPool; }
    auto commandBuffer() -> VulkanCommandBuffer& { return m_commandBuffer; }

    auto transformsBuffer() -> VulkanBuffer& { return m_transformsBuffer; }
    auto glyphInstanceBuffer() -> VulkanBuffer& { return m_glyphInstanceBuffer; }

    auto frameDoneFence() -> VulkanFence& { return m_frameDoneFence; }
    auto imageAcquiredSemaphore() -> VulkanBinarySemaphore& { return m_imageAcquiredSemaphore; }

    auto prepareTransformsBuffer(MRThreadsSharedDataLeaf& renderingData, ecs::components::Camera camera, const ecs::components::Transform& cameraTransform, float renderAspectRatio) -> void;

private:
    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Commands
    VulkanCommandPool m_commandPool = nullptr;
    VulkanCommandBuffer m_commandBuffer = nullptr;

    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Buffers
    VulkanBuffer m_transformsBuffer = nullptr;

    // temporary, for font rendering demo
    VulkanBuffer m_glyphInstanceBuffer = nullptr;

    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Synchronization

    /// Signaled after the frame has completed rendering
    VulkanFence m_frameDoneFence = nullptr;

    /// Signaled after a successful swapchain image acquire
    VulkanBinarySemaphore m_imageAcquiredSemaphore = nullptr;
};

}