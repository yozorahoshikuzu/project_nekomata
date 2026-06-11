module nekomata2;
import :graphics.vulkan.vk_pipeline_graphics;

namespace nekomata2 {

VulkanGraphicsPipeline::VulkanGraphicsPipeline(std::nullptr_t) {}
VulkanGraphicsPipeline::VulkanGraphicsPipeline(vk::raii::Pipeline&& vkPipeline) : m_vkPipeline(std::move(vkPipeline)) {}

auto VulkanGraphicsPipeline::builder() -> VulkanGraphicsPipelineBuilder {
    return {};
}

VulkanGraphicsPipelineBuilder::VulkanGraphicsPipelineBuilder() = default;
VulkanGraphicsPipelineBuilder::VulkanGraphicsPipelineBuilder(std::nullptr_t) {}

}