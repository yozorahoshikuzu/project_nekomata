module projnekomata;
import vulkan;
import projnekomata.cs;
import vk_mem_alloc;
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
    m_smaaEdgesImageIndex = srt.allocateImageIndex();
    m_smaaWeightsImageIndex = srt.allocateImageIndex();
    m_colorBufferIndex = srt.allocateImageIndex();
    m_colorBufferUnormViewIndex = srt.allocateImageIndex();

    setupRenderingAttachments(renderImageExtent);
}

auto TransientRenderingResources::handleWindowSizeChange(vk::Extent2D newWindowSize) -> void {
    setupRenderingAttachments(newWindowSize);
}

auto TransientRenderingResources::setupRenderingAttachments(vk::Extent2D renderImageExtent) -> void {
    auto affectedQueues = VulkanContext::get().vkPhysicalDeviceProps().m_queueFamilies[QueueFamily::Graphics];
    auto& srt = texturesystem::TextureManager::get().shaderResourceTable();


    auto colorMutableFormats = StaticSlice<const vk::Format>::inst<vk::Format::eR8G8B8A8Srgb, vk::Format::eR8G8B8A8Unorm>();

    m_depthBuffer = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eD32Sfloat, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);
    m_albedoAndRoughnessBuffer = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);
    m_normalBuffer = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR16G16Snorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);
    m_metallicAndAoBuffer = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);
    m_colorBuffer = VulkanImage::createMutableFormat(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined, colorMutableFormats);
    m_colorBufferUnormView = m_colorBuffer.createImageViewWithFormat(vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1, false);

    m_smaaEdgesImage = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);
    m_smaaWeightsImage = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);

    m_finalDrawBuffer = VulkanImage::createMutableFormat(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined, colorMutableFormats);
    m_finalDrawBufferUnormView = m_finalDrawBuffer.createImageViewWithFormat(vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1, false);

    srt.bindImage(m_depthBuffer, m_depthBufferIndex);
    srt.bindImage(m_albedoAndRoughnessBuffer, m_albedoAndRoughnessBufferIndex);
    srt.bindImage(m_normalBuffer, m_normalBufferIndex);
    srt.bindImage(m_metallicAndAoBuffer, m_metallicAndAoBufferIndex);
    srt.bindImage(m_smaaEdgesImage, m_smaaEdgesImageIndex);
    srt.bindImage(m_smaaWeightsImage, m_smaaWeightsImageIndex);
    srt.bindImage(m_colorBuffer, m_colorBufferIndex);
    srt.bindImageView(m_colorBufferUnormView, m_colorBufferUnormViewIndex);
}

} // namespace projnekomata::graphics