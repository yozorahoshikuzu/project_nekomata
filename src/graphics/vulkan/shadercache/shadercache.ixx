export module nekomata2:graphics.vulkan.shadercache;
import std;
import vulkan;
import :core.platform.int_def;
import :core.overloaded;
import :graphics.vulkan.shadercache.pipeline_binary_frontend;

export namespace nekomata2 {

using ShaderCacheFrontend = std::variant<std::monostate, ShaderCachePipelineBinaryFrontend>;

class ShaderCache {
public:
    ShaderCache(bool usePipelineBinaries);

    template <typename... ScElements>
        requires (std::is_same_v<vk::GraphicsPipelineCreateInfo, ScElements> || ...)
        && (std::is_same_v<vk::PipelineCreateFlags2CreateInfo, ScElements> || ...)
        && (std::is_same_v<vk::PipelineBinaryInfoKHR, ScElements> || ...)
    auto createGraphicsPipeline(vk::StructureChain<ScElements...>& chain) -> vk::raii::Pipeline {
        return std::visit(overloaded{
            [&](ShaderCachePipelineBinaryFrontend& sc) { return sc.handleCreateGraphicsPipeline(chain); },
            [&](auto&) { return VulkanContext::get().vkDevice().createGraphicsPipeline(nullptr, chain.template get<vk::GraphicsPipelineCreateInfo>()); }
        }, m_shaderCacheFrontend);
    }
private:
    ShaderCacheFrontend m_shaderCacheFrontend;

    auto makeShaderCacheDirectoryPath() -> std::filesystem::path;
};

}