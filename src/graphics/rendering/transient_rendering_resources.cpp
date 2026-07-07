module projnekomata;
import vulkan;
import vk_mem_alloc;
import :core.platform.int_def;
import :graphics.vulkan.context;
import :graphics.rendering.transient_rendering_resources;

namespace projnekomata::graphics {

TransientRenderingResources::TransientRenderingResources(std::nullptr_t) {  }

TransientRenderingResources::TransientRenderingResources(vk::Extent2D renderImageExtent) {
    setupRenderingAttachments(renderImageExtent);
}

auto TransientRenderingResources::handleWindowSizeChange(vk::Extent2D newWindowSize) -> void {
    setupRenderingAttachments(newWindowSize);
}

auto TransientRenderingResources::setupRenderingAttachments(vk::Extent2D renderImageExtent) -> void {
    auto affectedQueues = std::span<const u32>(&VulkanContext::get().vkPhysicalDeviceProps().m_graphicsQueueIndex, 1);
    m_depthBuffer = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eD32Sfloat, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);
    m_finalDrawBuffer = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eA2R10G10B10UnormPack32, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);
}

} // namespace projnekomata::graphics