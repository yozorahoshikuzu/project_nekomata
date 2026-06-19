export module nekomata2:graphics.vulkan.shadercache.pipeline_sc_concept;
import vulkan;

export namespace nekomata2 {

template <typename... StructChain> concept PipelineBinaryCacheableGraphicsPipelineCreateStructChain
    = (std::same_as<StructChain, vk::GraphicsPipelineCreateInfo> || ...)
    && (std::same_as<StructChain, vk::PipelineCreateFlags2CreateInfo> || ...)
    && (std::same_as<StructChain, vk::PipelineBinaryInfoKHR> || ...);

}