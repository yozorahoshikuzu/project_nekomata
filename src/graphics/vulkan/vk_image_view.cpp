module projnekomata;
import :graphics.vulkan.vk_image_view;

namespace projnekomata {

VulkanImageView::VulkanImageView(std::nullptr_t) {  }
VulkanImageView::VulkanImageView(vk::raii::ImageView&& vkImageView)
    : m_vkImageView(std::move(vkImageView)) {}

} // namespace projnekomata