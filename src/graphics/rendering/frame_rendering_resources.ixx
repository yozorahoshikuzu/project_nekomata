export module projnekomata:graphics.rendering.frame_rendering_resources;
import std;
import :core.platform.int_def;
import :graphics.vulkan.vk_commands;
import :graphics.vulkan.vk_buffer;
import :graphics.vulkan.sync_primitives.fence;
import :graphics.vulkan.sync_primitives.binary_semaphore;
import :core.runtime.shared_data;
import :core.ecs.world.camera;
import :core.ecs.world.transform;
import :graphics.vulkan.vk_query_pool;

export namespace projnekomata::graphics {

struct Transforms {
    Matrix4x4f model;
    Matrix3x3f normalMatrix;
};

struct RenderingGlobalData {
    Matrix4x4f projview;
    Matrix4x4f projviewInverse;
    Vector3f cameraPos;
};

struct PointlightData {
    Vector3f position;
    Vector3f lightRadiance;
};

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
    auto globalDataBuffer() -> VulkanBuffer& { return m_globalDataBuffer; }
    auto pointlightsBuffer() -> VulkanBuffer& { return m_pointlightsBuffer; }

    auto frameDoneFence() -> VulkanFence& { return m_frameDoneFence; }
    auto imageAcquiredSemaphore() -> VulkanBinarySemaphore& { return m_imageAcquiredSemaphore; }

    auto prepareBuffers(MRThreadsSharedDataLeaf& renderingData, ecs::components::Camera camera, const ecs::components::Transform& cameraTransform, float renderAspectRatio) -> void;

private:
    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Commands
    VulkanCommandPool m_commandPool = nullptr;
    VulkanCommandBuffer m_commandBuffer = nullptr;

    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Buffers
    VulkanBuffer m_globalDataBuffer = nullptr;
    VulkanBuffer m_transformsBuffer = nullptr;
    VulkanBuffer m_pointlightsBuffer = nullptr;

    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Synchronization

    /// Signaled after the frame has completed rendering
    VulkanFence m_frameDoneFence = nullptr;

    /// Signaled after a successful swapchain image acquire
    VulkanBinarySemaphore m_imageAcquiredSemaphore = nullptr;
};

}