module nekomata2.graphics.vulkan.vk_queue;
import std;
import vulkan;
import nekomata2.graphics.vulkan.context;

namespace nekomata2 {

auto GPUFuture::await(u64 timeout) const -> void {
    auto semWaitInfo = vk::SemaphoreWaitInfo{}
        .setSemaphores(*m_timelineSemaphore.get())
        .setValues(m_timelineSemaphoreTargetValue);

    VulkanContext::get().vkDevice().waitSemaphores(semWaitInfo, timeout);
}

VulkanQueue::VulkanQueue(std::nullptr_t) {}

VulkanQueue::VulkanQueue(vk::raii::Queue vkQueue, vk::raii::Semaphore vkTimelineSemaphore, u64 lastTimelineSubmissionValue)
    : m_vkQueue(std::move(std::move(vkQueue))), m_timelineSemaphore(std::move(vkTimelineSemaphore)), m_lastTimelineSubmissionValue(lastTimelineSubmissionValue) {}

VulkanQueue::~VulkanQueue() {
    m_vkQueue.waitIdle();
}

// TODO: Timeline semaphore support
auto VulkanQueue::submitOneCommandBuffer(const vk::raii::CommandBuffer& buf, const std::span<GPUFuture>& asyncWaits,
                                            const std::span<vk::PipelineStageFlags2>& asyncWaitStages,
                                            const std::optional<std::reference_wrapper<VulkanFence>>& fence) -> GPUFuture {
    std::scoped_lock lock(m_queueMutex);
    auto signalValue = m_lastTimelineSubmissionValue.fetch_add(1, std::memory_order_relaxed) + 1;

    std::vector<vk::SemaphoreSubmitInfo> semaphoreWaitInfos;
    semaphoreWaitInfos.reserve(asyncWaits.size());

    for (auto [op, flags] : std::views::zip(asyncWaits, asyncWaitStages)) {
        auto semaphoreSubmit =
            vk::SemaphoreSubmitInfo{}.setSemaphore(op.timelineSemaphore()).setValue(op.timelineSemaphoreTargetValue()).setStageMask(flags);

        semaphoreWaitInfos.emplace_back(semaphoreSubmit);
    }

    auto semaphoreSubmitInfo = vk::SemaphoreSubmitInfo{}
                                     .setSemaphore(m_timelineSemaphore)
                                     .setValue(signalValue)
                                     .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

    auto commandBuffer = vk::CommandBufferSubmitInfo{}.setCommandBuffer(buf);

    auto submitInfo =
        vk::SubmitInfo2{}.setCommandBufferInfos(commandBuffer).setSignalSemaphoreInfos(semaphoreSubmitInfo).setWaitSemaphoreInfos(semaphoreWaitInfos);

    if (fence.has_value()) {
        m_vkQueue.submit2(submitInfo, fence->get().vkFence());
    } else {
        m_vkQueue.submit2(submitInfo);
    }

    auto asyncOp = GPUFuture(m_timelineSemaphore, signalValue);
    return asyncOp;
}

auto VulkanQueue::submitOneCommandBufferWithBinarySemaphores(const vk::raii::CommandBuffer& buf, const std::span<GPUFuture>& asyncWaits,
                                                       const std::span<vk::PipelineStageFlags2>& asyncWaitStages, const VulkanBinarySemaphore& binaryWait,
                                                       const VulkanBinarySemaphore& binarySignal, const vk::PipelineStageFlags2& binaryWaitStage,
                                                       const vk::PipelineStageFlags2& binarySignalStage,
                                                       const std::optional<std::reference_wrapper<VulkanFence>>& fence) -> GPUFuture {
    std::scoped_lock lock(m_queueMutex);
    auto signalValue = m_lastTimelineSubmissionValue.fetch_add(1, std::memory_order_relaxed) + 1;

    std::vector<vk::SemaphoreSubmitInfo> semaphoreWaitInfos;
    semaphoreWaitInfos.reserve(asyncWaits.size() + 1);

    for (auto [op, flags] : std::views::zip(asyncWaits, asyncWaitStages)) {
        auto semaphoreSubmit =
            vk::SemaphoreSubmitInfo{}.setSemaphore(op.timelineSemaphore()).setValue(op.timelineSemaphoreTargetValue()).setStageMask(flags);

        semaphoreWaitInfos.emplace_back(semaphoreSubmit);
    }

    auto binarySemaphoreWaitInfo = vk::SemaphoreSubmitInfo{}
        .setSemaphore(binaryWait.vkSemaphore())
        .setStageMask(binaryWaitStage);
    semaphoreWaitInfos.emplace_back(binarySemaphoreWaitInfo);
    

    std::vector<vk::SemaphoreSubmitInfo> semaphoreSignalInfos;
    semaphoreSignalInfos.reserve(2);
    auto semaphoreSubmitInfo = vk::SemaphoreSubmitInfo{}
                                     .setSemaphore(m_timelineSemaphore)
                                     .setValue(signalValue)
                                     .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
    
    semaphoreSignalInfos.emplace_back(semaphoreSubmitInfo);

    auto binarySemaphoreSignalInfo = vk::SemaphoreSubmitInfo{}
        .setSemaphore(binarySignal.vkSemaphore())
        .setStageMask(binarySignalStage);
    semaphoreSignalInfos.emplace_back(binarySemaphoreSignalInfo);

    auto commandBuffer = vk::CommandBufferSubmitInfo{}.setCommandBuffer(buf);

    auto submitInfo =
        vk::SubmitInfo2{}.setCommandBufferInfos(commandBuffer).setSignalSemaphoreInfos(semaphoreSignalInfos).setWaitSemaphoreInfos(semaphoreWaitInfos);

    if (fence.has_value()) {
        m_vkQueue.submit2(submitInfo, fence->get().vkFence());
    } else {
        m_vkQueue.submit2(submitInfo);
    }

    auto asyncOp = GPUFuture(m_timelineSemaphore, signalValue);
    return asyncOp;
}

auto VulkanQueue::submitPresent(const VulkanSwapchain& swapchain, const VulkanBinarySemaphore& waitSemaphore, u32 imageIndex) -> vk::Result {
    std::scoped_lock lock(m_queueMutex);

    auto presentInfo = vk::PresentInfoKHR{}
        .setWaitSemaphores(*waitSemaphore.vkSemaphore())
        .setImageIndices(imageIndex)
        .setSwapchains(*swapchain.vkSwapchain());
    
    return m_vkQueue.presentKHR(presentInfo);
}

auto VulkanQueue::waitIdle() -> void {
    m_vkQueue.waitIdle();
}

} // namespace nekomata2