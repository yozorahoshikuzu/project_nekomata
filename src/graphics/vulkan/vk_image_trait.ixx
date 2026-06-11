export module nekomata2:graphics.vulkan.vk_image_trait;
import std;
import vulkan;

export namespace nekomata2 {

template <typename T> concept CVulkanImage = requires { typename T::isCVulkanImage; } && requires (const T& t)
{
    { t.vkImage() } -> std::convertible_to<vk::Image>;
    { t.extent() } -> std::convertible_to<vk::Extent3D>;
    { t.format() } -> std::convertible_to<vk::Format>;
    { t.subresourceRangeFull() } -> std::convertible_to<vk::ImageSubresourceRange>;
};

}