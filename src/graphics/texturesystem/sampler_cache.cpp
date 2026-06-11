module nekomata2.graphics.texturesystem.sampler_cache;
import nekomata2.graphics.texturesystem.texture_manager;

namespace nekomata2::graphics::texturesystem {

SamplerCache::SamplerCache() {}

auto SamplerCache::acquireSampler(const SamplerParams& params) -> u32 {
    {
        std::shared_lock lock(m_hashmapMutex);
        auto it = m_hashmap.find(params);
        if (it != m_hashmap.end()) return it->second.shaderIndex;
    }

    std::unique_lock lock(m_hashmapMutex);
    auto it = m_hashmap.find(params);
    if (it != m_hashmap.end()) return it->second.shaderIndex;

    m_hashmap.emplace(params, createSampler(params));
    return m_hashmap.at(params).shaderIndex;
}

auto SamplerCache::createSampler(const SamplerParams& params) -> SamplerCacheEntry {
    auto sampler = VulkanSampler::create(params.minFilter, params.magFilter, params.mipmapMode, params.addressModeU, params.addressModeV, params.addressModeW, params.anisotropy > 1.0f, params.anisotropy, params.minLod, params.maxLod, params.mipLodBias);

    auto idx = TextureManager::get().shaderResourceTable().allocateSamplerIndex();
    TextureManager::get().shaderResourceTable().bindSampler(sampler, idx);

    auto entry = SamplerCacheEntry{std::move(sampler), idx.imageIndex};

    return entry;
}

} // namespace nekomata2::graphics::texturesystem