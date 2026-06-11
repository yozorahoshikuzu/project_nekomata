module nekomata2;
import :graphics.vulkan.vk_descriptor_set;

namespace nekomata2 {

VulkanDescriptorSet::VulkanDescriptorSet(std::nullptr_t) {  }
VulkanDescriptorSet::VulkanDescriptorSet(vk::raii::DescriptorSet&& vkDescriptorSet)
    : m_vkDescriptorSet(std::move(vkDescriptorSet)) {}

VulkanDescriptorSet::~VulkanDescriptorSet() {
    if (!m_isFromFreeDescriptorSetPool) {
        m_vkDescriptorSet = nullptr;
    }
}

} // namespace nekomata2