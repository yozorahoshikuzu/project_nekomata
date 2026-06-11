module nekomata2.graphics.vulkan.sync_primitives.binary_semaphore;
import nekomata2.graphics.vulkan.context;

namespace nekomata2 {

VulkanBinarySemaphore::VulkanBinarySemaphore(std::nullptr_t) {}
VulkanBinarySemaphore::VulkanBinarySemaphore(vk::raii::Semaphore&& vkSemaphore) : m_vkSemaphore(std::move(vkSemaphore)) {}

auto VulkanBinarySemaphore::create() -> VulkanBinarySemaphore {
    auto semaphoreCreateInfo = vk::SemaphoreCreateInfo{};

    auto handle = VulkanContext::get().vkDevice().createSemaphore(semaphoreCreateInfo);
    return VulkanBinarySemaphore(std::move(handle));
}

}