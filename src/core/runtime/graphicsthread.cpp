module nekomata2;
import :core.log;
import :graphics.cmd_alloc;
import :graphics.vulkan.context;
import :graphics.meshsystem.mesh_asset_storage;
import :core.runtime.graphicsthread;

namespace nekomata2 {

RenderThread::RenderThread(const std::shared_ptr<MRThreadsSharedData>& mrSharedData)
    : m_mrSharedData(mrSharedData) {
        m_currentWindowExtent = mrSharedData->m_leafs.getSecondary().m_currentWindowExtent;
    }

auto RenderThread::runMainLoop() -> void {
    cmdalloc::VulkanCommandPoolsList::initThreadLocalCommandPools();

    m_vkSwapchain = VulkanSwapchain::create(m_currentWindowExtent, std::nullopt, false);
    // TODO : remove the abuse
    std::construct_at(&m_sharedRenderingResources);
    m_transientRenderingResources = graphics::TransientRenderingResources(m_currentWindowExtent);
    for (usize i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_frames[i] = graphics::FrameContext();
    }

    m_timeAtStart = std::chrono::high_resolution_clock::now();


    m_lastFrameTime = std::chrono::high_resolution_clock::now();
    while (true) {
        m_mrSharedData->m_syncpointBarrier.arrive_and_wait();

        if (m_mrSharedData->m_shouldQuit.load(std::memory_order_acquire)) {
            break;
        }
        // NOTE: The main thread is responsible for swapping the data buffer leafs.

        m_mrSharedData->m_syncpointBarrier.arrive_and_wait();

        loop();
    }

    log::info("Render Thread exiting...");

    cmdalloc::VulkanCommandPoolsList::destroyThreadLocalCommandPools();
}


auto RenderThread::loop() -> void {
    if (!m_mrSharedData->m_leafs.getSecondary().m_hasValidFrame) return;

    auto currentTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration<float>(currentTime - m_lastFrameTime).count();
    m_lastFrameTime = currentTime;

    fpsSmoothedDt = fpsDtSmoothAlpha * deltaTime + (1.0f - fpsDtSmoothAlpha) * fpsSmoothedDt;

    auto timeSinceLastFpsReport = std::chrono::duration<float>(currentTime - m_lastFpsReportTime).count();

    bool shouldReportFps = timeSinceLastFpsReport > 0.4f;
    if (shouldReportFps) {
        m_lastFpsReportTime = currentTime;
        m_sharedRenderingResources.displayMs = deltaTime * 1000.0f;
    }

    meshsystem::MeshAssetStorage::get().tickGC(m_currentFrameNumber);

    auto maybeNewWindowExtent = m_mrSharedData->m_leafs.getSecondary().m_currentWindowExtent;
    if (m_mustRecreateSwapchainNextFrame || m_currentWindowExtent != maybeNewWindowExtent) {
        m_currentWindowExtent = maybeNewWindowExtent;
        m_mustRecreateSwapchainNextFrame = false;

        // TODO: This is to work around present queues not being friendly to synchronize
        VulkanContext::get().vkDevice().waitIdle();
        m_vkSwapchain = VulkanSwapchain::create(m_currentWindowExtent, std::move(m_vkSwapchain), false);
        m_transientRenderingResources.handleWindowSizeChange(m_currentWindowExtent);
    }
    auto timeSinceStart = std::chrono::duration<float>(currentTime - m_timeAtStart).count();

    auto result = m_frames[m_currentFrameContextIndex].execute(m_transientRenderingResources, m_sharedRenderingResources, m_vkSwapchain,
                                                               m_mrSharedData->m_leafs.getSecondary(), timeSinceStart);
    if (result.stepPerFrameResources)
        m_currentFrameContextIndex = (m_currentFrameContextIndex + 1) % MAX_FRAMES_IN_FLIGHT;

    if (result.shouldRecreateSwapchain)
        m_mustRecreateSwapchainNextFrame = true;

    m_currentFrameNumber++;
}

}