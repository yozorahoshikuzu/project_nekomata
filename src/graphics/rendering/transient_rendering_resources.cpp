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

TransientRenderingResources::TransientRenderingResources(vk::Extent2D renderImageExtent, SharedRenderingResources& sharedResources) {
    auto& srt = texturesystem::TextureManager::get().shaderResourceTable();
    m_depthBufferIndex = srt.allocateSampledImageIndex();
    m_albedoAndRoughnessBufferIndex = srt.allocateSampledImageIndex();
    m_normalBufferIndex = srt.allocateSampledImageIndex();
    m_metallicAndAoBufferIndex = srt.allocateSampledImageIndex();
    m_velocityBufferIndex = srt.allocateSampledImageIndex();
    m_smaaEdgesImageIndex = srt.allocateSampledImageIndex();
    m_smaaWeightsImageIndex = srt.allocateSampledImageIndex();
    m_colorBufferIndex = srt.allocateSampledImageIndex();
    m_colorBufferUnormViewIndex = srt.allocateSampledImageIndex();

    m_smaaColorResolvedBuffer0UnormViewIndex = srt.allocateSampledImageIndex();
    m_smaaColorResolvedBuffer1UnormViewIndex = srt.allocateSampledImageIndex();

    m_postSmaaImageIndex = srt.allocateSampledImageIndex();
    m_postSmaaImageUnormViewIndex = srt.allocateSampledImageIndex();

    m_overdrawCountersImageIndex = srt.allocateStorageImageIndex();

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
    m_normalBuffer = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { renderImageExtent, 1 }, 1, 1, false, vk::Format::eR16G16B16A16Snorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);
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

    vk::Extent2D halfExtentCeil = vk::Extent2D {
        (renderImageExtent.width + 1) / 2,
        (renderImageExtent.height + 1) / 2,
    };
    m_overdrawCountersImage = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { halfExtentCeil, 1 }, 1, 1, false, vk::Format::eR32Uint, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, affectedQueues, vk::ImageLayout::eUndefined);

    srt.bindSampledImage(m_depthBuffer, m_depthBufferIndex);
    srt.bindSampledImage(m_albedoAndRoughnessBuffer, m_albedoAndRoughnessBufferIndex);
    srt.bindSampledImage(m_normalBuffer, m_normalBufferIndex);
    srt.bindSampledImage(m_metallicAndAoBuffer, m_metallicAndAoBufferIndex);
    srt.bindSampledImage(m_velocityBuffer, m_velocityBufferIndex);
    srt.bindSampledImage(m_smaaEdgesImage, m_smaaEdgesImageIndex);
    srt.bindSampledImage(m_smaaWeightsImage, m_smaaWeightsImageIndex);
    srt.bindSampledImage(m_colorBuffer, m_colorBufferIndex);
    srt.bindSampledImageView(m_colorBufferUnormView, m_colorBufferUnormViewIndex);
    srt.bindSampledImageView(m_smaaColorResolvedBuffer0UnormView, m_smaaColorResolvedBuffer0UnormViewIndex);
    srt.bindSampledImageView(m_smaaColorResolvedBuffer1UnormView, m_smaaColorResolvedBuffer1UnormViewIndex);
    srt.bindSampledImage(m_postSmaaImage, m_postSmaaImageIndex);
    srt.bindSampledImageView(m_postSmaaImageUnormView, m_postSmaaImageUnormViewIndex);

    srt.bindStorageImage(m_overdrawCountersImage, m_overdrawCountersImageIndex);

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
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite
        )
        .insertImageMemoryBarrier(m_smaaColorResolvedBuffer1,
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite
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