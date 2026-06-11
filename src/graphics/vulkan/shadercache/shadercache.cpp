module nekomata2.graphics.vulkan.shadercache;

namespace nekomata2 {

ShaderCache::ShaderCache()
    : m_shaderCacheFrontend(ShaderCachePipelineBinaryFrontend(makeShaderCacheDirectoryPath(), 2, 1)) {
    std::visit(overloaded{
        [&](ShaderCachePipelineBinaryFrontend& sc) { sc.checkGlobalKeyAndInvalidateStale(); }
    }, m_shaderCacheFrontend);
}

auto ShaderCache::makeShaderCacheDirectoryPath() -> std::filesystem::path {
    // TODO: use a proper path
    return "./shadercache_placeholder_name/";
}

} // namespace nekomata2