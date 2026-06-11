module;
#include <xxhash.h>
#include <string.h>
export module nekomata2:graphics.texturesystem.sampler_cache;
import std;
import vulkan;
import :core.platform.int_def;
import :graphics.vulkan.vk_sampler;

export namespace nekomata2::graphics::texturesystem {

struct SamplerParams {
    SamplerParams() = default;

    vk::Filter minFilter = vk::Filter::eLinear;
    vk::Filter magFilter = vk::Filter::eLinear;
    vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear;
    vk::SamplerAddressMode addressModeU = vk::SamplerAddressMode::eRepeat;
    vk::SamplerAddressMode addressModeV = vk::SamplerAddressMode::eRepeat;
    vk::SamplerAddressMode addressModeW = vk::SamplerAddressMode::eRepeat;
    f32 anisotropy = 1.0f;
    f32 maxLod = vk::LodClampNone;
    f32 minLod = 0.0f;
    f32 mipLodBias = 0.0f;

    auto operator==(const SamplerParams& other) const -> bool {
        return memcmp(this, &other, sizeof(*this)) == 0;
    }

    [[nodiscard]] constexpr static auto defaultValues() -> SamplerParams {
        return SamplerParams();
    }

    [[nodiscard]] constexpr auto setMinFilter(vk::Filter filter) noexcept -> SamplerParams& {
        minFilter = filter;
        return *this;
    }
    [[nodiscard]] constexpr auto setMagFilter(vk::Filter filter) noexcept -> SamplerParams& {
        magFilter = filter;
        return *this;
    }
    [[nodiscard]] constexpr auto setMipmapMode(vk::SamplerMipmapMode mipmapMode) noexcept -> SamplerParams& {
        this->mipmapMode = mipmapMode;
        return *this;
    }
    [[nodiscard]] constexpr auto setAddressModeU(vk::SamplerAddressMode addressModeU) noexcept -> SamplerParams& {
        this->addressModeU = addressModeU;
        return *this;
    }
    [[nodiscard]] constexpr auto setAddressModeV(vk::SamplerAddressMode addressModeV) noexcept -> SamplerParams& {
        this->addressModeV = addressModeV;
        return *this;
    }
    [[nodiscard]] constexpr auto setAddressModeW(vk::SamplerAddressMode addressModeW) noexcept -> SamplerParams& {
        this->addressModeW = addressModeW;
        return *this;
    }
    [[nodiscard]] constexpr auto setAnisotropy(f32 anisotropy) noexcept -> SamplerParams& {
        this->anisotropy = anisotropy;
        return *this;
    }
    [[nodiscard]] constexpr auto setMaxLod(f32 maxLod) noexcept -> SamplerParams& {
        this->maxLod = maxLod;
        return *this;
    }
    [[nodiscard]] constexpr auto setMinLod(f32 minLod) noexcept -> SamplerParams& {
        this->minLod = minLod;
        return *this;
    }
    [[nodiscard]] constexpr auto setMipLodBias(f32 mipLodBias) noexcept -> SamplerParams& {
        this->mipLodBias = mipLodBias;
        return *this;
    }
};

struct SamplerParamsHash {
    size_t operator()(const SamplerParams& k) const {
        return XXH3_64bits(&k, sizeof(k));
    }
};

class SamplerCache {
public:
    SamplerCache();

    SamplerCache(const SamplerCache&) = delete;
    SamplerCache(SamplerCache&&) = delete;
    SamplerCache& operator=(const SamplerCache&) = delete;
    SamplerCache& operator=(SamplerCache&&) = delete;

    /// Returns the shader index of the sampler. TODO: make a strong typedef
    [[nodiscard]] auto acquireSampler(const SamplerParams& params) -> u32;

private:
    struct SamplerCacheEntry {
        VulkanSampler sampler;
        u32 shaderIndex = 0;
    };

    static auto createSampler(const SamplerParams& params) -> SamplerCacheEntry;

    std::unordered_map<SamplerParams, SamplerCacheEntry, SamplerParamsHash> m_hashmap;
    std::shared_mutex m_hashmapMutex;
};

}