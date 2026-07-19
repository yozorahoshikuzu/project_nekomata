module projnekomata;
import vulkan;
import projnekomata.cs;
import vk_mem_alloc;
import :graphics.vulkan.context;
import :graphics.rendering.transient_rendering_resources;
import :graphics.cmd_alloc;
import :graphics.vulkan.vk_commands_barriers;

namespace projnekomata::graphics {

TransientRenderingResources::TransientRenderingResources(std::nullptr_t) {  }

TransientRenderingResources::TransientRenderingResources(vk::Extent2D renderImageExtent) {
    auto& srt = texturesystem::TextureManager::get().shaderResourceTable();
    m_depthBufferIndex = srt.allocateImageIndex();
    m_albedoAndRoughnessBufferIndex = srt.allocateImageIndex();
    m_normalBufferIndex = srt.allocateImageIndex();
    m_metallicAndAoBufferIndex = srt.allocateImageIndex();
    m_velocityBufferIndex = srt.allocateImageIndex();
    m_smaaEdgesImageIndex = srt.allocateImageIndex();
    m_smaaWeightsImageIndex = srt.allocateImageIndex();
    m_colorBufferIndex = srt.allocateImageIndex();
    m_colorBufferUnormViewIndex = srt.allocateImageIndex();

    m_smaaColorResolvedBuffer0UnormViewIndex = srt.allocateImageIndex();
    m_smaaColorResolvedBuffer1UnormViewIndex = srt.allocateImageIndex();

    m_postSmaaImageIndex = srt.allocateImageIndex();
    m_postSmmaImageUnormViewIndex = srt.allocateImageIndex();

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
    m_velocityBuffer = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR16G16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);
    m_colorBuffer = VulkanImage::createMutableFormat(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined, colorMutableFormats);
    m_colorBufferUnormView = m_colorBuffer.createImageViewWithFormat(vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1, false);

    m_smaaColorResolvedBuffer0 = VulkanImage::createMutableFormat(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined, colorMutableFormats);
    m_smaaColorResolvedBuffer1 = VulkanImage::createMutableFormat(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined, colorMutableFormats);
    m_smaaColorResolvedBuffer0UnormView = m_smaaColorResolvedBuffer0.createImageViewWithFormat(vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1, false);
    m_smaaColorResolvedBuffer1UnormView = m_smaaColorResolvedBuffer1.createImageViewWithFormat(vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1, false);

    m_smaaEdgesImage = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);
    m_smaaWeightsImage = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);

    m_postSmaaImage = VulkanImage::createMutableFormat(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined, colorMutableFormats);
    m_postSmaaImageUnormView = m_postSmaaImage.createImageViewWithFormat(vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1, false);

    m_finalImage = VulkanImage::createMutableFormat(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined, colorMutableFormats);
    m_finalImageUnormView = m_finalImage.createImageViewWithFormat(vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1, false);

    srt.bindImage(m_depthBuffer, m_depthBufferIndex);
    srt.bindImage(m_albedoAndRoughnessBuffer, m_albedoAndRoughnessBufferIndex);
    srt.bindImage(m_normalBuffer, m_normalBufferIndex);
    srt.bindImage(m_metallicAndAoBuffer, m_metallicAndAoBufferIndex);
    srt.bindImage(m_velocityBuffer, m_velocityBufferIndex);
    srt.bindImage(m_smaaEdgesImage, m_smaaEdgesImageIndex);
    srt.bindImage(m_smaaWeightsImage, m_smaaWeightsImageIndex);
    srt.bindImage(m_colorBuffer, m_colorBufferIndex);
    srt.bindImageView(m_colorBufferUnormView, m_colorBufferUnormViewIndex);
    srt.bindImageView(m_smaaColorResolvedBuffer0UnormView, m_smaaColorResolvedBuffer0UnormViewIndex);
    srt.bindImageView(m_smaaColorResolvedBuffer1UnormView, m_smaaColorResolvedBuffer1UnormViewIndex);
    srt.bindImage(m_postSmaaImage, m_postSmaaImageIndex);
    srt.bindImageView(m_postSmaaImageUnormView, m_postSmmaImageUnormViewIndex);

    zeroinitColorBuffers();
}
auto TransientRenderingResources::zeroinitColorBuffers() -> void {
    auto cb = cmdalloc::VulkanCommandPoolsList::getAssignedGraphicsCommandPool().allocateCommandBuffer(vk::CommandBufferLevel::ePrimary);
    auto& cmd = cb.vkCommandBuffer();

    auto beginInfo = vk::CommandBufferBeginInfo{}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    vkCheckResult(cmd.begin(beginInfo));

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(m_smaaColorResolvedBuffer0,
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eNone
        )
        .insertImageMemoryBarrier(m_smaaColorResolvedBuffer1,
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eNone
        )
        .flush(cb);

    cmd.clearColorImage(m_smaaColorResolvedBuffer0.vkImage(), vk::ImageLayout::eTransferDstOptimal, vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 0.0f }, m_smaaColorResolvedBuffer0.subresourceRangeFull());
    cmd.clearColorImage(m_smaaColorResolvedBuffer1.vkImage(), vk::ImageLayout::eTransferDstOptimal, vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 0.0f }, m_smaaColorResolvedBuffer1.subresourceRangeFull());

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(m_smaaColorResolvedBuffer0,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead
        )
        .insertImageMemoryBarrier(m_smaaColorResolvedBuffer1,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead
        )
        .flush(cb);

    vkCheckResult(cmd.end());
    VulkanContext::get().vkQueueGraphics().submitOneCommandBuffer(cmd, {},{}, None)
        .await();
}

} // namespace projnekomata::graphics