export module nekomata2:graphics.vulkan.vk_image;
import std;
import vulkan;
import vk_mem_alloc;
import :core.platform.int_def;
import :graphics.vulkan.vk_image_view;
import :graphics.vulkan.vk_gpu_obrm;
import :graphics.vulkan.vk_image_trait;

export namespace nekomata2 {

struct ImageFormatMd {
    vk::ImageAspectFlags aspectFlags;
    /// for plain uncompressed formats; set to 0 if compressed
    u64 bpp = 0;
    /// for compressed formats; set to 0 if uncompressed
    u64 blockByteSize = 0;
    /// for compressed formats; set to 1 if uncompressed
    u64 blockWidth = 0;
    /// for compressed formats; set to 1 if uncompressed
    u64 blockHeight = 0;
};

class VulkanImage {
public:
    using isCVulkanImage = std::true_type;

    VulkanImage(std::nullptr_t);
    VulkanImage(vk::raii::Image&& image, vma::raii::Allocation&& allocation, vk::raii::ImageView imageViewWholeSize, vk::ImageSubresourceRange imageSubresourceRangeFull, vk::Extent3D extents, u32 arrayLayerCount, u32 mipLevelCount, vk::Format format, vk::ImageType type);

    VulkanImage(const VulkanImage&) = delete;
    VulkanImage(VulkanImage&&) = default;
    VulkanImage& operator=(const VulkanImage&) = delete;
    VulkanImage& operator=(VulkanImage&&) = default;

    static auto create(vk::ImageType type, vk::Extent3D extent, u32 arrayLayerCount, u32 mipLevelCount, vk::Format format, vk::ImageUsageFlags usage, vk::ImageTiling tiling, vma::MemoryUsage memoryUsage, vk::MemoryPropertyFlags memoryRequiredFlags, const std::span<const u32>& queueFamilyIndices, vk::ImageLayout initialLayout) -> VulkanImage;

    // clang-format off
    static std::unordered_map<vk::Format, ImageFormatMd> s_formatMetadata;
    // clang-format on

    [[nodiscard]] auto vkImage() const -> const vk::raii::Image& { return m_vkImage.vkHandle(); }
    [[nodiscard]] auto vkImageViewWholeSize() const -> const vk::raii::ImageView& { return m_vkImageViewWholeSize.vkHandle(); }
    [[nodiscard]] auto extent() const -> vk::Extent3D { return m_vkImageExtents; }
    [[nodiscard]] auto format() const -> vk::Format { return m_vkImageFormat; }
    [[nodiscard]] auto subresourceRangeFull() const -> vk::ImageSubresourceRange { return m_vkImageSubresourceRangeFull; }

    auto createImageView(u32 baseMipLevel, u32 mipLevelCount, u32 baseArrayLayer, u32 arrayLayerCount) -> VulkanImageView;
    auto createImageViewWithMinLod(u32 baseMipLevel, u32 mipLevelCount, u32 baseArrayLayer, u32 arrayLayerCount, float minLod) -> VulkanImageView;

private:
    VulkanAsyncRaiiWrapper<vk::raii::Image> m_vkImage = nullptr;
    VulkanAsyncRaiiWrapper<vk::raii::ImageView> m_vkImageViewWholeSize = nullptr;
    VulkanAsyncRaiiWrapper<vma::raii::Allocation> m_vmaAllocation = nullptr;

    vk::ImageSubresourceRange m_vkImageSubresourceRangeFull;
    vk::Extent3D m_vkImageExtents;
    u32 m_arrayLayerCount;
    u32 m_mipLevelCount;
    vk::Format m_vkImageFormat;
    vk::ImageType m_vkImageType;

    static auto selectImageViewType(vk::ImageType type, u32 arrayLayerCount) -> vk::ImageViewType;
};
static_assert(CVulkanImage<VulkanImage> && "VulkanImage must satisfy CVulkanImage");


}