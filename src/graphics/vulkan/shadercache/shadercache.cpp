module nekomata2;
import :graphics.vulkan.shadercache;

namespace nekomata2 {

ShaderCache::ShaderCache(bool usePipelineBinaries) {
    if (usePipelineBinaries) {
        m_shaderCacheFrontend = ShaderCachePipelineBinaryFrontend(makeShaderCacheDirectoryPath(), 2, 1);
    } else {
        m_shaderCacheFrontend = std::monostate{};
    }

    match(m_shaderCacheFrontend,
        [&](ShaderCachePipelineBinaryFrontend& sc) { sc.checkGlobalKeyAndInvalidateStale(); },
        [&](auto&) { }
    );
}

auto ShaderCache::makeShaderCacheDirectoryPath() -> std::filesystem::path {
    // TODO: use a proper path
    return "./shadercache_placeholder_name/";
}

} // namespace nekomata2