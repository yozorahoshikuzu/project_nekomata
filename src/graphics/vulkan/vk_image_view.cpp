module nekomata2;
import :graphics.vulkan.vk_image_view;

namespace nekomata2 {

VulkanImageView::VulkanImageView(std::nullptr_t) {  }
VulkanImageView::VulkanImageView(vk::raii::ImageView&& vkImageView)
    : m_vkImageView(std::move(vkImageView)) {}

} // namespace nekomata2