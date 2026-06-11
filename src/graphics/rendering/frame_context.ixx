export module nekomata2.graphics.rendering.frame_context;
import std;
import nekomata2.graphics.rendering.transient_rendering_resources;
import nekomata2.graphics.rendering.shared_rendering_resources;
import nekomata2.graphics.vulkan.vk_swapchain;
import nekomata2.core.runtime.shared_data;

export namespace nekomata2::graphics {

struct FrameResult {
    bool shouldRecreateSwapchain = false;
    bool stepPerFrameResources = false;
};

class FrameContext {
public:
    FrameContext(std::nullptr_t);
    FrameContext();

    [[nodiscard]] auto execute(TransientRenderingResources& transientRenderingResources, SharedRenderingResources& sharedRenderingResources,
                               VulkanSwapchain& swapchain, MRThreadsSharedDataLeaf& renderingData, float timeSinceStart) -> FrameResult;

private:
    FrameRenderingResources m_frameRenderingResources = nullptr;

};

}