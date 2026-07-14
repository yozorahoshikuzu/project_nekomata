export module projnekomata:graphics.texturesystem.texture_manager;
import std;
import vulkan;
import :core.platform.int_def;
import :graphics.vulkan.vk_image;
import :graphics.srt.shader_resource_table;
import :core.containers.freelist_pool;
import :graphics.texturesystem.sampler_cache;
import :graphics.texturesystem.texture_to_shader_index_table;

export namespace projnekomata::graphics::texturesystem {

inline class TextureManager* g_textureManager = nullptr;

struct Texture {
    u32 index = 0;
};

struct TextureResources {
public:
    TextureResources(std::nullptr_t);
    TextureResources(VulkanImage&& image);

    [[nodiscard]] auto image() -> VulkanImage& { return m_image; }
    auto setImage(VulkanImage&& image) -> void { m_image = std::move(image); }

private:
    VulkanImage m_image = nullptr;

};

class TextureManager {
public:
    static auto get() -> TextureManager& { return *g_textureManager; }
    TextureManager(std::nullptr_t);
    TextureManager(std::unique_ptr<srt::IShaderResourceTable>&& srt);

    static auto create() -> std::unique_ptr<TextureManager>;

    // auto loadTextureFromMemory(u32 width, u32 height, u32 depth, u32 arrayLayers, u32 mipLevels, vk::Format format, Slice<const u8>& data) -> Texture;

    auto createTexture(u32 width, u32 height, u32 depth, u32 layers, u32 mipLevels, bool isCube, vk::Format format, vk::ImageUsageFlags usage, const SamplerParams& samplerParams) -> Texture;

    /// Asynchronously loads a KTX2 texture from system storage.
    ///
    /// The texture can immediately be used, but it will be substituted for a dummy texture while the load is pending.
    auto loadKtx2TextureAsync(const std::filesystem::path& path, const SamplerParams& samplerParams) -> Texture;
    auto loadKtx2TextureBlocking(const std::filesystem::path& path, const SamplerParams& samplerParams) -> Texture;
    auto freeTexture(Texture texture) -> void;

    [[nodiscard]] constexpr auto shaderResourceTable() -> srt::IShaderResourceTable& { return *m_srt; }
    [[nodiscard]] constexpr auto samplerCache() -> SamplerCache& { return m_samplerCache; }
    [[nodiscard]] constexpr auto getTextureResources(Texture texture) -> TextureResources& { return m_loadedTextures[texture.index]; }
    [[nodiscard]] constexpr auto textureToShaderIndexTable() -> TextureToShaderIndexTable& { return m_textureToShaderIndexTable; }

private:

    auto allocateTexture(VulkanImage&& img) -> Texture;

    auto loadTextureFromMemoryInternal(u32 width, u32 height, u32 depth, u32 arrayLayers, u32 mipLevels, vk::Format format, Slice<const u8> data, const SamplerParams& samplerParams) -> Texture;

    static auto temporary_uploadTheImage(Texture texture, const std::filesystem::path& path, const SamplerParams& params) -> void;

    Texture m_defaultTexture;

    FreelistPool<TextureResources, 10, 4096> m_loadedTextures;
    SamplerCache m_samplerCache;

    TextureToShaderIndexTable m_textureToShaderIndexTable = nullptr;

    std::unique_ptr<srt::IShaderResourceTable> m_srt = nullptr;
};

}