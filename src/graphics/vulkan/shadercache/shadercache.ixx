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
        requires PipelineBinaryCacheableGraphicsPipelineCreateStructChain<ScElements...>
    auto createGraphicsPipeline(vk::StructureChain<ScElements...>& chain) -> vk::raii::Pipeline {
        return match(m_shaderCacheFrontend,
            [&](ShaderCachePipelineBinaryFrontend& sc) { return sc.handleCreateGraphicsPipeline(chain); },
            [&](const std::monostate&) { return vkCheckResult(VulkanContext::get().vkDevice().createGraphicsPipeline(nullptr, chain.template get<vk::GraphicsPipelineCreateInfo>())); }
        );
    }

    auto usesPipelineBinaries() const -> bool {
        return match(m_shaderCacheFrontend,
            [&](const ShaderCachePipelineBinaryFrontend&) { return true; },
            [&](const std::monostate&) { return false; }
        );
    }
private:
    ShaderCacheFrontend m_shaderCacheFrontend;

    auto makeShaderCacheDirectoryPath() -> std::filesystem::path;
};

}