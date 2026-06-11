module;
#include <freetype/freetype.h>
export module nekomata2.graphics.fontsystem.font_manager;
import std;
import vulkan;
import nekomata2.core.platform.int_def;
import nekomata2.core.math;
import nekomata2.graphics.fontsystem.font_face;
import nekomata2.graphics.fontsystem.dynamic_font_atlas;

export namespace nekomata2::graphics::fonts {
class FontManager;

inline FontManager* g_fontManager = nullptr;

struct FontEntry {
    FT_Face face  = nullptr;

    bool isLoaded = false;

    mutable std::mutex rasterMutex;

    float unitsPerEm = 0.0f;
    float ascender = 0.0f;
    float descender = 0.0f;
};

struct FontRasterBatch {
    FontFace fontFace;
    u32 pixelSize;
    std::vector<u32> glyphIndices;
};

struct FontRasterInfo {
    std::span<const FontRasterBatch> batches;
    rendering::DynamicBitmapFontAtlas& atlas;
    // the u32 is the index in the font atlas image array
    std::unordered_map<u32, std::vector<vk::BufferImageCopy2>>& copyRegions;
    std::vector<u8>& resultBuffer;
    std::vector<u32>& newImageIndices;
};

struct GlyphInstance {
    math::Vector2f positionStart;
    math::Vector2f positionEnd;
    math::Vector2f texcoordStart;
    math::Vector2f texcoordEnd;
    u32 imageShaderIndex;
};

class FontManager {
public:
    static auto get() -> FontManager& {
        return *g_fontManager;
    }

    FontManager(std::nullptr_t);
    FontManager();
    ~FontManager();

    static auto create() -> std::unique_ptr<FontManager>;

    auto loadFont(const std::filesystem::path& path) -> FontFace;
    auto freeFont(FontFace font) -> void;

    auto rasterizeGlyphs(FontRasterInfo rasterInfo) -> void;
    auto shapeText(FontFace font, rendering::DynamicBitmapFontAtlas& atlas, std::string_view text, u32 pixelSize, math::Vector2f baselineStartPos, math::Vector2f screenSize) -> std::vector<GlyphInstance>;
    auto findAndBatchMissingGlyphs(FontFace font, rendering::DynamicBitmapFontAtlas& atlas, std::string_view text, u32 pixelSize) -> std::optional<FontRasterBatch>;

private:
    FT_Library m_ftLibrary;
    std::mutex m_ftLibraryMutex;

    std::vector<std::unique_ptr<FontEntry>> m_fontEntries;
    std::shared_mutex m_registryMutex;

    u32 getFreeFontIndex();
};

}