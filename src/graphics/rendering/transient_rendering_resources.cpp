module projnekomata;
import vulkan;
import vk_mem_alloc;
import :core.platform.int_def;
import :graphics.vulkan.context;
import :graphics.rendering.transient_rendering_resources;

namespace projnekomata::graphics {

TransientRenderingResources::TransientRenderingResources(std::nullptr_t) {  }

TransientRenderingResources::TransientRenderingResources(vk::Extent2D renderImageExtent) {
    auto& srt = texturesystem::TextureManager::get().shaderResourceTable();
    m_depthBufferIndex = srt.allocateImageIndex();
    m_albedoAndRoughnessBufferIndex = srt.allocateImageIndex();
    m_normalBufferIndex = srt.allocateImageIndex();
    m_metallicAndAoBufferIndex = srt.allocateImageIndex();

    setupRenderingAttachments(renderImageExtent);
}

auto TransientRenderingResources::handleWindowSizeChange(vk::Extent2D newWindowSize) -> void {
    setupRenderingAttachments(newWindowSize);
}

auto TransientRenderingResources::setupRenderingAttachments(vk::Extent2D renderImageExtent) -> void {
    auto affectedQueues = std::span<const u32>(&VulkanContext::get().vkPhysicalDeviceProps().m_graphicsQueueIndex, 1);
    auto& srt = texturesystem::TextureManager::get().shaderResourceTable();

    m_depthBuffer = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eD32Sfloat, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);
    m_albedoAndRoughnessBuffer = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);
    m_normalBuffer = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR16G16Snorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);
    m_metallicAndAoBuffer = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);

    m_finalDrawBuffer = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);

    srt.bindImage(m_depthBuffer, m_depthBufferIndex);
    srt.bindImage(m_albedoAndRoughnessBuffer, m_albedoAndRoughnessBufferIndex);
    srt.bindImage(m_normalBuffer, m_normalBufferIndex);
    srt.bindImage(m_metallicAndAoBuffer, m_metallicAndAoBufferIndex);
}

} // namespace projnekomata::graphics