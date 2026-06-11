module nekomata2;
import :graphics.vulkan.context;

namespace nekomata2 {

auto GpuResourceRetireTimelineValues::latestSubmitValues() -> GpuResourceRetireTimelineValues {
    auto graphicsValue = VulkanContext::get().vkQueueGraphics().lastTimelineSubmissionValue();
    auto asyncComputeValue = VulkanContext::get().vkQueueAsyncCompute().lastTimelineSubmissionValue();
    return {graphicsValue, asyncComputeValue};
}
auto GpuResourceRetireTimelineValues::queueCurrentValues() -> GpuResourceRetireTimelineValues {
    auto graphicsValue = VulkanContext::get().vkQueueGraphics().currentTimelineValue();
    auto asyncComputeValue = VulkanContext::get().vkQueueAsyncCompute().currentTimelineValue();
    return {graphicsValue, asyncComputeValue};
}

} // namespace nekomata2