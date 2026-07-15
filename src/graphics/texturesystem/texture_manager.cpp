module;
#include <ktx.h>
#include <vulkan/vulkan.h>
#include <ktxvulkan.h>
#include <cstring>
module projnekomata;
import fmt;
import vk_mem_alloc;
import projnekomata.cs;
import :graphics.cmd_alloc;
import :graphics.vulkan.vk_queue_family_swizzling;
import :graphics.vulkan.vk_buffer;
import :graphics.vulkan.vk_commands_barriers;
import :graphics.srt.bindless_descriptor_set_srt;
import :graphics.vulkan.context;
import :graphics.texturesystem.texture_manager;

namespace projnekomata::graphics::texturesystem {

TextureResources::TextureResources(std::nullptr_t) {}
TextureResources::TextureResources(VulkanImage&& image)
    : m_image(std::move(image)) {}

TextureManager::TextureManager(std::nullptr_t) {}
TextureManager::TextureManager(std::unique_ptr<srt::IShaderResourceTable>&& srt)
    : m_textureToShaderIndexTable(2048), m_srt(std::move(srt)) {}

auto TextureManager::create() -> std::unique_ptr<TextureManager> {
    debug_assert(g_textureManager == nullptr, "only one TextureManager may live at any given time");

    auto srt = srt::BindlessDescriptorSetShaderResourceTable::create(2048, 2048);

    auto textureManager = std::make_unique<TextureManager>(std::move(srt));
    g_textureManager = textureManager.get();
    auto pxData = Vec<u8>::create({0x39, 0x43, 0x52, 0xff});
    // loadTextureFromMemoryInternal depends on the sampler!!!
    u32 defaultSamplerIndex = g_textureManager->m_samplerCache.acquireSampler(SamplerParams::defaultValues());
    debug_assert(defaultSamplerIndex == 0, "the default sampler didn't have index 0");
    g_textureManager->m_defaultTexture = g_textureManager->loadTextureFromMemoryInternal(1, 1, 1, 1, 1, vk::Format::eR8G8B8A8Srgb, pxData.asSlice(), SamplerParams::defaultValues());
    debug_assert(g_textureManager->m_defaultTexture.index == 0, "the default texture didn't have index 0");
    return textureManager;
}
auto TextureManager::createTexture(u32 width, u32 height, u32 depth, u32 layers, u32 mipLevels, bool isCube, vk::Format format, vk::ImageUsageFlags usage,
                                   const SamplerParams& samplerParams) -> Texture {
    // TODO: fix UB with assigning 2D null image when image is a type other than 2D
    auto image = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { width, height, depth }, layers, mipLevels, isCube, format, usage, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, VulkanContext::get().vkPhysicalDeviceProps().m_queueFamilies[QueueFamily::Graphics | QueueFamily::AsyncCompute], vk::ImageLayout::eUndefined);
    auto texture = allocateTexture(std::move(image));
    auto imageShaderIndex = m_srt->allocateImageIndex();
    auto samplerShaderIndex = m_samplerCache.acquireSampler(samplerParams);
    TextureManager::get().m_srt->bindImage(getTextureResources(texture).image(), imageShaderIndex);
    m_textureToShaderIndexTable.setTextureShaderImageIndex(texture.index, imageShaderIndex.imageIndex);
    m_textureToShaderIndexTable.setTextureShaderSamplerIndex(texture.index, samplerShaderIndex);
    return texture;
}

auto TextureManager::loadTextureFromMemoryInternal(u32 width, u32 height, u32 depth, u32 arrayLayers, u32 mipLevels, vk::Format format,
                                           Slice<const u8> data, const SamplerParams& samplerParams) -> Texture {
    auto image = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { width, height, depth }, arrayLayers, mipLevels, false, format, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vma::MemoryUsage::eAutoPreferDevice, {}, VulkanContext::get().vkPhysicalDeviceProps().m_queueFamilies[QueueFamily::Graphics | QueueFamily::AsyncCompute], vk::ImageLayout::eUndefined);

    auto buffer = VulkanBuffer::create(data.size(), vk::BufferUsageFlagBits::eTransferSrc, VulkanBufferMemoryMapping::MapForSequentialWrite, vma::MemoryUsage::eAutoPreferDevice, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, VulkanContext::get().vkPhysicalDeviceProps().m_queueFamilies[QueueFamily::Graphics | QueueFamily::AsyncCompute]);
    memcpy(buffer.memoryHostPtr(), data.data(), data.size());
    auto cmd = cmdalloc::VulkanCommandPoolsList::getAssignedAsyncComputeCommandPool().allocateCommandBuffer(vk::CommandBufferLevel::ePrimary);
    auto& cb = cmd.vkCommandBuffer();
    auto cbBeginInfo = vk::CommandBufferBeginInfo{}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cb.begin(cbBeginInfo);
    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(
            image,
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eNone,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite
        )
        .flush(cmd);

    auto copyRegion = vk::BufferImageCopy2{}
        .setImageExtent(vk::Extent3D { width, height, depth })
        .setImageOffset(vk::Offset3D { 0, 0, 0 })
        .setImageSubresource(vk::ImageSubresourceLayers { vk::ImageAspectFlagBits::eColor, 0, 0, 1 })
        .setBufferOffset(0)
        .setBufferImageHeight(0)
        .setBufferRowLength(0);
    auto copyInfo = vk::CopyBufferToImageInfo2{}
        .setDstImage(image.vkImage())
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setSrcBuffer(buffer.vkBuffer())
        .setRegions(copyRegion);
    cb.copyBufferToImage2(copyInfo);
    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(
            image,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone
        )
        .flush(cmd);
    cb.end();
    auto asyncOp = VulkanContext::get().vkQueueAsyncCompute().submitOneCommandBuffer(cb, {}, {}, None);
    asyncOp.await();

    auto texture = allocateTexture(std::move(image));
    auto imageShaderIndex = m_srt->allocateImageIndex();
    auto samplerShaderIndex = m_samplerCache.acquireSampler(samplerParams);
    TextureManager::get().m_srt->bindImage(getTextureResources(texture).image(), imageShaderIndex);
    m_textureToShaderIndexTable.setTextureShaderImageIndex(texture.index, imageShaderIndex.imageIndex);
    m_textureToShaderIndexTable.setTextureShaderSamplerIndex(texture.index, samplerShaderIndex);

    return texture;
}

auto TextureManager::loadKtx2TextureAsync(const std::filesystem::path& path, const SamplerParams& samplerParams) -> Texture {
    auto texture = allocateTexture(nullptr);

    // TODO: use a threadpool
    auto thr = std::thread(temporary_uploadTheImage, texture, path, samplerParams);
    thr.detach();

    return texture;
}
auto TextureManager::loadKtx2TextureBlocking(const std::filesystem::path& path, const SamplerParams& samplerParams) -> Texture {
    auto texture = allocateTexture(nullptr);
    temporary_uploadTheImage(texture, path, samplerParams);
    return texture;
}

auto TextureManager::allocateTexture(VulkanImage&& img) -> Texture {
    auto textureIndex = m_loadedTextures.emplace(std::move(img));
    return Texture(textureIndex);
}

auto TextureManager::freeTexture(Texture texture) -> void {
    log::error("freeTexture not implemented");
}

auto TextureManager::temporary_uploadTheImage(Texture texture, const std::filesystem::path& path, const SamplerParams& samplerParams) -> void {
    cmdalloc::VulkanCommandPoolsList::initThreadLocalCommandPools();

    log::info("Loading texture: {}", path.string());
    // Load the image from disk first.
    // TODO: basist::ktx2_transcoder might give better results
    ktxTexture2* ktxData;
    KTX_error_code result = ktxTexture2_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxData);

    if (result != KTX_SUCCESS) {
        panic("failed to load texture: {}", path.string());
    }

    bool needsTranscoding = ktxTexture2_NeedsTranscoding(ktxData);
    if (needsTranscoding) {
        KTX_error_code transcodeResult = ktxTexture2_TranscodeBasis(ktxData, KTX_TTF_BC7_RGBA, 0);
        if (transcodeResult != KTX_SUCCESS) {
            panic("failed to transcode texture");
        }
    }

    auto& textureResources = TextureManager::get().getTextureResources(texture);

    // Data parameters
    auto bufferSize = ktxData->dataSize;
    auto imgData = ktxData->pData;

    // Image extents and mip chain
    auto imageWidth = ktxData->baseWidth;
    auto imageHeight = ktxData->baseHeight;
    auto imageDepth = ktxData->baseDepth;
    auto imageArrayLayers = ktxData->numLayers > 0 ? ktxData->numLayers : 1;
    auto imageMipLevels = ktxData->numLevels;
    auto imageIsCubemap = ktxData->isCubemap;

    // Image usage and format
    auto imageFormat = static_cast<vk::Format>(ktxTexture2_GetVkFormat(ktxData));
    auto imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    auto imageTiling = vk::ImageTiling::eOptimal;
    auto imageQueues = VulkanContext::get().vkPhysicalDeviceProps().m_queueFamilies[QueueFamily::Graphics | QueueFamily::AsyncCompute];

    // Vulkan Image
    auto image = VulkanImage::create(vk::ImageType::e2D, vk::Extent3D { imageWidth, imageHeight, imageDepth }, imageArrayLayers, imageMipLevels, imageIsCubemap, imageFormat, imageUsage, imageTiling, vma::MemoryUsage::eAutoPreferDevice, {}, imageQueues, vk::ImageLayout::eUndefined);


    log::info("Texture {} size: {}x{}x{}{} mips: {} layers: {} mem: {} format: {}{}",
        path.string(),
        imageWidth, imageHeight, imageDepth,
        imageIsCubemap ? " cubemap" : "",
        imageMipLevels, imageArrayLayers,
        bufferSize >= 1048576 ? fmt::format("{} MiB", bufferSize / 1048576) : fmt::format("{} kiB", bufferSize / 1024),
        vk::to_string(imageFormat), needsTranscoding ? " decompr" : ""
    );

    // Staging buffer
    auto buffer = VulkanBuffer::create(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, VulkanBufferMemoryMapping::MapForSequentialWrite, vma::MemoryUsage::eAutoPreferDevice, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, imageQueues);
    memcpy(buffer.memoryHostPtr(), imgData, bufferSize);

    // Command to upload
    auto cmd = cmdalloc::VulkanCommandPoolsList::getAssignedAsyncComputeCommandPool().allocateCommandBuffer(vk::CommandBufferLevel::ePrimary);
    auto& cb = cmd.vkCommandBuffer();

    auto cbBeginInfo = vk::CommandBufferBeginInfo{}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    vkCheckResult(cb.begin(cbBeginInfo));
    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(
            image,
            vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite
        )
        .flush(cmd);

    Vec<vk::BufferImageCopy2> copyRegions;

    u32 faceCount = imageIsCubemap ? 6 : 1;

    for (u32 mip = 0; mip < imageMipLevels; mip++) {
        for (u32 layer = 0; layer < imageArrayLayers; layer++) {
            for (u32 face = 0; face < faceCount; face++) {
                u32 arrayLayer = imageIsCubemap ? (layer * 6) + face : layer;

                auto subresLayers = vk::ImageSubresourceLayers{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(arrayLayer)
                    .setLayerCount(1)
                    .setMipLevel(mip);

                ktx_size_t offset;
                ktxTexture2_GetImageOffset(ktxData, mip, layer, face, &offset);

                auto extent = vk::Extent3D{
                    std::max(1u,  imageWidth >> mip),
                    std::max(1u, imageHeight >> mip),
                    std::max(1u,  imageDepth >> mip)
                };

                auto copyRegion = vk::BufferImageCopy2{}
                    .setImageExtent(extent)
                    .setImageOffset(vk::Offset3D{0, 0, 0})
                    .setImageSubresource(subresLayers)
                    .setBufferOffset(offset)
                    .setBufferImageHeight(0)
                    .setBufferRowLength(0);
                copyRegions.emplace(copyRegion);
            }
        }
    }

    auto copyInfo = vk::CopyBufferToImageInfo2{}
        .setDstImage(image.vkImage())
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setSrcBuffer(buffer.vkBuffer())
        .setRegions(copyRegions);

    cb.copyBufferToImage2(copyInfo);

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(
            image,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone
        )
        .flush(cmd);
    vkCheckResult(cb.end());

    auto future = VulkanContext::get().vkQueueAsyncCompute().submitOneCommandBuffer(cb, {}, {}, None);
    ktxTexture2_Destroy(ktxData);

    // TODO: Move somewhere else. We don't want it blocking an entire job when a threadpool gets in place.
    textureResources.setImage(std::move(image));
    // Wait for the upload to complete and then let the texture be used for rendering
    future.await();
    auto imageShaderIndex = TextureManager::get().m_srt->allocateImageIndex();
    TextureManager::get().m_srt->bindImage(textureResources.image(), imageShaderIndex);
    TextureManager::get().m_textureToShaderIndexTable.setTextureShaderImageIndex(texture.index, imageShaderIndex.imageIndex);
    TextureManager::get().m_textureToShaderIndexTable.setTextureShaderSamplerIndex(texture.index, TextureManager::get().m_samplerCache.acquireSampler(samplerParams));
}

} // namespace projnekomata::texturesystem