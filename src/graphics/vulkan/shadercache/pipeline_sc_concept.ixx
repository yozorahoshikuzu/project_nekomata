export module projnekomata:graphics.vulkan.shadercache.pipeline_sc_concept;
import vulkan;

export namespace projnekomata {

template <typename... StructChain> concept PipelineBinaryCacheableGraphicsPipelineCreateStructChain
    = (std::same_as<StructChain, vk::GraphicsPipelineCreateInfo> || ...)
    && (std::same_as<StructChain, vk::PipelineCreateFlags2CreateInfo> || ...)
    && (std::same_as<StructChain, vk::PipelineBinaryInfoKHR> || ...);

}