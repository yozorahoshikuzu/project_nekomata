module nekomata2.graphics.vulkan.vk_pipeline_layout;

namespace nekomata2 {

VulkanPipelineLayout::VulkanPipelineLayout(std::nullptr_t) : m_vkPipelineLayout(nullptr) {}
VulkanPipelineLayout::VulkanPipelineLayout(vk::raii::PipelineLayout&& vkPipelineLayout) : m_vkPipelineLayout(std::move(vkPipelineLayout)) {}

auto VulkanPipelineLayout::builder() -> VulkanPipelineLayoutBuilder {
    return {};
}

}