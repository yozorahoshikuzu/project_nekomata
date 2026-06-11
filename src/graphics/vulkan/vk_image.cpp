module nekomata2;
import :graphics.vulkan.context;
import :graphics.vulkan.vk_image;

namespace nekomata2 {

VulkanImage::VulkanImage(std::nullptr_t) {}
VulkanImage::VulkanImage(vk::raii::Image&& image, vma::raii::Allocation&& allocation, vk::raii::ImageView imageViewWholeSize, vk::ImageSubresourceRange imageSubresourceRangeFull, vk::Extent3D extents, u32 arrayLayerCount, u32 mipLevelCount, vk::Format format, vk::ImageType type)
    : m_vkImage(std::move(image)), m_vkImageViewWholeSize(std::move(imageViewWholeSize)), m_vmaAllocation(std::move(allocation)), m_vkImageSubresourceRangeFull(imageSubresourceRangeFull), m_vkImageExtents(extents), m_arrayLayerCount(arrayLayerCount), m_mipLevelCount(mipLevelCount), m_vkImageFormat(format), m_vkImageType(type) {}

// clang-format off
std::unordered_map<vk::Format, ImageFormatMd> VulkanImage::s_formatMetadata = {
    // Plain Color Formats
    {vk::Format::eR8Unorm,                                    ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .bpp = 1 }},
    {vk::Format::eR8G8Unorm,                                  ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .bpp = 2 }},
    {vk::Format::eR8G8B8A8Unorm,                              ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .bpp = 4 }},

    {vk::Format::eR8G8B8A8Srgb,                               ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .bpp = 4 }},
    {vk::Format::eB8G8R8A8Srgb,                               ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .bpp = 4 }},
    {vk::Format::eR16G16B16A16Sfloat,                         ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .bpp = 8 }},
    {vk::Format::eR16G16Sfloat,                               ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .bpp = 4 }},

    // Compressed Color Formats
    // BC1 - 4x4 blocks, 8 bytes per block
    { vk::Format::eBc1RgbUnormBlock,                          ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 8, .blockWidth = 4, .blockHeight = 4 } },
    { vk::Format::eBc1RgbSrgbBlock,                           ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 8, .blockWidth = 4, .blockHeight = 4 } },
    { vk::Format::eBc1RgbaUnormBlock,                         ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 8, .blockWidth = 4, .blockHeight = 4 } },
    { vk::Format::eBc1RgbaSrgbBlock,                          ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 8, .blockWidth = 4, .blockHeight = 4 } },

    // BC2 - 4x4 blocks, 16 bytes per block
    { vk::Format::eBc2UnormBlock,                             ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 16, .blockWidth = 4, .blockHeight = 4 } },
    { vk::Format::eBc2SrgbBlock,                              ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 16, .blockWidth = 4, .blockHeight = 4 } },

    // BC3 - 4x4 blocks, 16 bytes per block
    { vk::Format::eBc3UnormBlock,                             ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 16, .blockWidth = 4, .blockHeight = 4 } },
    { vk::Format::eBc3SrgbBlock,                              ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 16, .blockWidth = 4, .blockHeight = 4 } },

    // BC4 - 4x4 blocks, 8 bytes per block
    { vk::Format::eBc4UnormBlock,                             ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 8, .blockWidth = 4, .blockHeight = 4 } },
    { vk::Format::eBc4SnormBlock,                             ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 8, .blockWidth = 4, .blockHeight = 4 } },

    // BC5 - 4x4 blocks, 16 bytes per block
    { vk::Format::eBc5UnormBlock,                             ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 16, .blockWidth = 4, .blockHeight = 4 } },
    { vk::Format::eBc5SnormBlock,                             ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 16, .blockWidth = 4, .blockHeight = 4 } },

    // BC6H - 4x4 blocks, 16 bytes per block
    { vk::Format::eBc6HUfloatBlock,                           ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 16, .blockWidth = 4, .blockHeight = 4 } },
    { vk::Format::eBc6HSfloatBlock,                           ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 16, .blockWidth = 4, .blockHeight = 4 } },

    // BC7 - 4x4 blocks, 16 bytes per block
    { vk::Format::eBc7UnormBlock,                             ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 16, .blockWidth = 4, .blockHeight = 4 } },
    { vk::Format::eBc7SrgbBlock,                              ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eColor, .blockByteSize = 16, .blockWidth = 4, .blockHeight = 4 } },

    // Depth Formats
    {vk::Format::eD32Sfloat,                                  ImageFormatMd { .aspectFlags = vk::ImageAspectFlagBits::eDepth, .bpp = 4 }},
};
// clang-format on

auto VulkanImage::create(vk::ImageType type, vk::Extent3D extent, u32 arrayLayerCount, u32 mipLevelCount, vk::Format format, vk::ImageUsageFlags usage, vk::ImageTiling tiling, vma::MemoryUsage memoryUsage, vk::MemoryPropertyFlags memoryRequiredFlags, const std::span<const u32>& queueFamilyIndices, vk::ImageLayout initialLayout) -> VulkanImage {
    auto imageCreateInfo = vk::ImageCreateInfo{}
        .setImageType(type)
        .setExtent(extent)
        .setFormat(format)
        .setUsage(usage)
        .setTiling(tiling)
        .setArrayLayers(arrayLayerCount)
        .setMipLevels(mipLevelCount)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setInitialLayout(initialLayout)
        .setQueueFamilyIndices(queueFamilyIndices)
        .setSharingMode(queueFamilyIndices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent);
    
    auto allocationCreateInfo = vma::AllocationCreateInfo{}
        .setUsage(memoryUsage)
        .setRequiredFlags(memoryRequiredFlags);

    auto [allocation, image] = VulkanContext::get().vmaAllocator().createImage(imageCreateInfo, allocationCreateInfo).split();

    auto imageViewType = selectImageViewType(type, arrayLayerCount);
    auto imageViewSubresRange = vk::ImageSubresourceRange{}
        .setBaseMipLevel(0)
        .setLevelCount(mipLevelCount)
        .setBaseArrayLayer(0)
        .setLayerCount(arrayLayerCount)
        .setAspectMask(s_formatMetadata[format].aspectFlags);

    auto imageViewCreateInfo = vk::ImageViewCreateInfo{}
        .setImage(image)
        .setViewType(imageViewType)
        .setFormat(format)
        .setSubresourceRange(imageViewSubresRange);
    
    auto imageView = VulkanContext::get().vkDevice().createImageView(imageViewCreateInfo);
    return VulkanImage(std::move(image), std::move(allocation), std::move(imageView), imageViewSubresRange, extent, arrayLayerCount, mipLevelCount, format, type);
}

auto VulkanImage::createImageView(u32 baseMipLevel, u32 mipLevelCount, u32 baseArrayLayer, u32 arrayLayerCount) -> VulkanImageView {
    auto subresourceRange = vk::ImageSubresourceRange{}
        .setBaseMipLevel(baseMipLevel)
        .setLevelCount(mipLevelCount)
        .setBaseArrayLayer(baseArrayLayer)
        .setLayerCount(arrayLayerCount)
        .setAspectMask(s_formatMetadata[m_vkImageFormat].aspectFlags);

    auto imageViewType = selectImageViewType(m_vkImageType, arrayLayerCount);
    auto imageViewCreateInfo = vk::ImageViewCreateInfo{}
        .setImage(m_vkImage.vkHandle())
        .setViewType(imageViewType)
        .setFormat(m_vkImageFormat)
        .setSubresourceRange(subresourceRange);

    return VulkanImageView(VulkanContext::get().vkDevice().createImageView(imageViewCreateInfo));
}
auto VulkanImage::createImageViewWithMinLod(u32 baseMipLevel, u32 mipLevelCount, u32 baseArrayLayer, u32 arrayLayerCount, float minLod) -> VulkanImageView {
    auto subresourceRange = vk::ImageSubresourceRange{}
        .setBaseMipLevel(baseMipLevel)
        .setLevelCount(mipLevelCount)
        .setBaseArrayLayer(baseArrayLayer)
        .setLayerCount(arrayLayerCount)
        .setAspectMask(s_formatMetadata[m_vkImageFormat].aspectFlags);

    auto imageViewType = selectImageViewType(m_vkImageType, arrayLayerCount);
    auto imageViewCreateInfo = vk::ImageViewCreateInfo{}
        .setImage(m_vkImage.vkHandle())
        .setViewType(imageViewType)
        .setFormat(m_vkImageFormat)
        .setSubresourceRange(subresourceRange);

    auto minLodInfo = vk::ImageViewMinLodCreateInfoEXT{}
        .setMinLod(minLod);
    auto sc = vk::StructureChain{imageViewCreateInfo, minLodInfo};
    return VulkanImageView(VulkanContext::get().vkDevice().createImageView(sc.get<vk::ImageViewCreateInfo>()));
}

auto VulkanImage::selectImageViewType(vk::ImageType type, u32 arrayLayerCount) -> vk::ImageViewType {
    vk::ImageViewType imageViewType;
    switch (type) {
        case vk::ImageType::e1D: imageViewType = vk::ImageViewType::e1D; break;
        case vk::ImageType::e2D: imageViewType = vk::ImageViewType::e2D; break;
        case vk::ImageType::e3D: imageViewType = vk::ImageViewType::e3D; break;
    }
    if (arrayLayerCount > 1) {
        if (imageViewType == vk::ImageViewType::e1D) imageViewType = vk::ImageViewType::e1DArray;
        else if (imageViewType == vk::ImageViewType::e2D) imageViewType = vk::ImageViewType::e2DArray;
    }
    return imageViewType;
}

} // namespace nekomata2