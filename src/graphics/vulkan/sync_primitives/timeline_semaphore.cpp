module nekomata2.graphics.vulkan.sync_primitives.timeline_semaphore;
import nekomata2.graphics.vulkan.context;

namespace nekomata2 {

VulkanTimelineSemaphore::VulkanTimelineSemaphore(std::nullptr_t) {}
VulkanTimelineSemaphore::VulkanTimelineSemaphore(vk::raii::Semaphore&& vkSemaphore) : m_vkSemaphore(std::move(vkSemaphore)) {}

auto VulkanTimelineSemaphore::create(u64 initialValue) -> VulkanTimelineSemaphore {
    auto semaphoreTypeInfo = vk::SemaphoreTypeCreateInfo{}
        .setSemaphoreType(vk::SemaphoreType::eTimeline)
        .setInitialValue(initialValue);
    
    auto chain = vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo>{
        vk::SemaphoreCreateInfo{},
        semaphoreTypeInfo
    };

    auto handle = VulkanContext::get().vkDevice().createSemaphore(chain.get<vk::SemaphoreCreateInfo>());
    return VulkanTimelineSemaphore(std::move(handle));
}

}