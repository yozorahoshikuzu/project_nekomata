export module nekomata2.graphics.vulkan.vk_image_view;
import vulkan;
import nekomata2.graphics.vulkan.vk_gpu_obrm;

export namespace nekomata2 {

class VulkanImageView {
public:
    VulkanImageView(std::nullptr_t);
    VulkanImageView(vk::raii::ImageView&& vkImageView);

    VulkanImageView(const VulkanImageView&) = delete;
    VulkanImageView(VulkanImageView&&) = default;
    VulkanImageView& operator=(const VulkanImageView&) = delete;
    VulkanImageView& operator=(VulkanImageView&&) = default;

    [[nodiscard]] auto vkImageView() const -> const vk::raii::ImageView& { return m_vkImageView.vkHandle(); }

private:
    VulkanAsyncRaiiWrapper<vk::raii::ImageView> m_vkImageView = nullptr;
};

}