module projnekomata;
import :graphics.texturesystem.texture_manager;

namespace projnekomata::graphics::texturesystem {

SamplerCache::SamplerCache() {}

auto SamplerCache::acquireSampler(const SamplerParams& params) -> u32 {
    {
        std::shared_lock lock(m_hashmapMutex);
        if (auto entry = m_hashmap.get(params)) return entry->get().shaderIndex;
    }

    std::unique_lock lock(m_hashmapMutex);
    if (auto entry = m_hashmap.get(params)) return entry->get().shaderIndex;

    m_hashmap.insert(params, createSampler(params));
    return m_hashmap[params].shaderIndex;
}

auto SamplerCache::createSampler(const SamplerParams& params) -> SamplerCacheEntry {
    auto sampler = VulkanSampler::create(params.minFilter, params.magFilter, params.mipmapMode, params.addressModeU, params.addressModeV, params.addressModeW, params.anisotropy > 1.0f, params.anisotropy, params.minLod, params.maxLod, params.mipLodBias);

    auto idx = TextureManager::get().shaderResourceTable().allocateSamplerIndex();
    TextureManager::get().shaderResourceTable().bindSampler(sampler, idx);

    auto entry = SamplerCacheEntry{std::move(sampler), idx.imageIndex};

    return entry;
}

} // namespace projnekomata::graphics::texturesystem