export module nekomata2.core.runtime.graphicsthread;
import std;
import vulkan;
import nekomata2.core.platform.int_def;
import nekomata2.core.runtime.shared_data;
import nekomata2.graphics.vulkan.vk_swapchain;
import nekomata2.graphics.rendering.transient_rendering_resources;
import nekomata2.graphics.rendering.shared_rendering_resources;
import nekomata2.graphics.rendering.frame_context;

export namespace nekomata2 {

constexpr usize MAX_FRAMES_IN_FLIGHT = 2;

class RenderThread {
public:
    RenderThread(const std::shared_ptr<MRThreadsSharedData>& mrSharedData);

    auto runMainLoop() -> void;

private:
    auto loop() -> void;

    VulkanSwapchain m_vkSwapchain = nullptr;
    graphics::SharedRenderingResources m_sharedRenderingResources = nullptr;
    graphics::TransientRenderingResources m_transientRenderingResources = nullptr;
    std::array<graphics::FrameContext, MAX_FRAMES_IN_FLIGHT> m_frames = {nullptr};
    usize m_currentFrameContextIndex = 0;
    usize m_currentFrameNumber = 0;

    // with MainThread
    std::shared_ptr<MRThreadsSharedData> m_mrSharedData = nullptr;
    bool m_mustRecreateSwapchainNextFrame = false;
    vk::Extent2D m_currentWindowExtent;

    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastFrameTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastFpsReportTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_timeAtStart;

    float fpsSmoothedDt = 0.0f;
    float fpsDtSmoothAlpha = 0.05f;

};

}