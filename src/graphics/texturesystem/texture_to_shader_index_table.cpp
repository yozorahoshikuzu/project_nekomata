module nekomata2.graphics.texturesystem.texture_to_sader_index_table;
import nekomata2.core.platform.assert;

namespace nekomata2::graphics::texturesystem {

TextureToShaderIndexTable::TextureToShaderIndexTable(std::nullptr_t) {}
TextureToShaderIndexTable::TextureToShaderIndexTable(usize maxTextureCount) : m_textureToShaderImageIndexTable(maxTextureCount), m_textureToShaderSamplerIndexTable(maxTextureCount) {
    // Preinitialize the arrays with zeroes, so any potential invalid/not-ready references resolve to index 0 (default/dummy image/samplers).
    for (usize i = 0; i < maxTextureCount; i++) {
        m_textureToShaderImageIndexTable[i].store(0, std::memory_order_relaxed);
        m_textureToShaderSamplerIndexTable[i].store(0, std::memory_order_relaxed);
    }
}

auto TextureToShaderIndexTable::setTextureShaderImageIndex(usize textureId, u32 shaderImageIndex) -> void {
    m_textureToShaderImageIndexTable[textureId].store(shaderImageIndex, std::memory_order_relaxed);
}

auto TextureToShaderIndexTable::setTextureShaderSamplerIndex(usize textureId, u32 shaderSamplerIndex) -> void {
    m_textureToShaderSamplerIndexTable[textureId].store(shaderSamplerIndex, std::memory_order_relaxed);
}

auto TextureToShaderIndexTable::snapshotTables(std::vector<u32>& dstTextureToShaderImageIndexTable, std::vector<u32>& dstTextureToShaderSamplerIndexTable) const
    -> void {
    debug_assert(dstTextureToShaderImageIndexTable.size() >= m_textureToShaderImageIndexTable.size(), "the texture to shader image index table must have enough capacity for a snapshot");
    debug_assert(dstTextureToShaderSamplerIndexTable.size() >= m_textureToShaderSamplerIndexTable.size(), "the texture to shader sampler index table must have enough capacity for a snapshot");

    // It is possible a thread might update one of the indices during the loops. However, we want the latest state anyway for rendering, so it's not an issue.
    for (usize i = 0; i < m_textureToShaderImageIndexTable.size(); i++) {
        dstTextureToShaderImageIndexTable[i] = m_textureToShaderImageIndexTable[i].load(std::memory_order_relaxed);
    }

    for (usize i = 0; i < m_textureToShaderSamplerIndexTable.size(); i++) {
        dstTextureToShaderSamplerIndexTable[i] = m_textureToShaderSamplerIndexTable[i].load(std::memory_order_relaxed);
    }

}
} // namespace nekomata2::graphics::texturesystem