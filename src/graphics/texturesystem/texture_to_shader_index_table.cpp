module projnekomata;
import projnekomata.cs;
import :graphics.texturesystem.texture_to_shader_index_table;

namespace projnekomata::graphics::texturesystem {

TextureToShaderIndexTable::TextureToShaderIndexTable(std::nullptr_t) {}
TextureToShaderIndexTable::TextureToShaderIndexTable(usize maxTextureCount)
    : m_textureToShaderImageIndexTable(Vec<std::atomic<u32>>::withCapacity(maxTextureCount)), m_textureToShaderSamplerIndexTable(Vec<std::atomic<u32>>::withCapacity(maxTextureCount)) {
    // Preinitialize the arrays with zeroes, so any potential invalid/not-ready references resolve to index 0 (default/dummy image/samplers).
    for (usize i = 0; i < maxTextureCount; i++) {
        m_textureToShaderImageIndexTable.emplace(0);
        m_textureToShaderSamplerIndexTable.emplace(0);
    }
}

auto TextureToShaderIndexTable::setTextureShaderImageIndex(usize textureId, u32 shaderImageIndex) -> void {
    m_textureToShaderImageIndexTable[textureId].store(shaderImageIndex, std::memory_order_relaxed);
}

auto TextureToShaderIndexTable::setTextureShaderSamplerIndex(usize textureId, u32 shaderSamplerIndex) -> void {
    m_textureToShaderSamplerIndexTable[textureId].store(shaderSamplerIndex, std::memory_order_relaxed);
}
auto TextureToShaderIndexTable::textureToShaderImageIndex(usize textureId) const -> u32 {
    return m_textureToShaderImageIndexTable[textureId].load(std::memory_order_relaxed);
}
auto TextureToShaderIndexTable::textureToShaderSamplerIndex(usize textureId) const -> u32 {
    return m_textureToShaderSamplerIndexTable[textureId].load(std::memory_order_relaxed);
}

auto TextureToShaderIndexTable::snapshotTables(Vec<u32>& dstTextureToShaderImageIndexTable, Vec<u32>& dstTextureToShaderSamplerIndexTable) const
    -> void {
    debug_assert(dstTextureToShaderImageIndexTable.len() >= m_textureToShaderImageIndexTable.len(), "the texture to shader image index table must have enough capacity for a snapshot");
    debug_assert(dstTextureToShaderSamplerIndexTable.len() >= m_textureToShaderSamplerIndexTable.len(), "the texture to shader sampler index table must have enough capacity for a snapshot");

    // It is possible a thread might update one of the indices during the loops. However, we want the latest state anyway for rendering, so it's not an issue.
    for (usize i = 0; i < m_textureToShaderImageIndexTable.size(); i++) {
        dstTextureToShaderImageIndexTable[i] = m_textureToShaderImageIndexTable[i].load(std::memory_order_relaxed);
    }

    for (usize i = 0; i < m_textureToShaderSamplerIndexTable.size(); i++) {
        dstTextureToShaderSamplerIndexTable[i] = m_textureToShaderSamplerIndexTable[i].load(std::memory_order_relaxed);
    }

}
} // namespace projnekomata::graphics::texturesystem