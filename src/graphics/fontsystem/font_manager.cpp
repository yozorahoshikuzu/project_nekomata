module;
#include <freetype/freetype.h>
#include <utf8cpp/utf8.h>
module projnekomata;
import :core.log;
import :core.platform.assert;
import :graphics.fontsystem.dynamic_font_atlas;
import :graphics.fontsystem.font_manager;

namespace projnekomata::graphics::fonts {

FontManager::FontManager(std::nullptr_t) {  }
FontManager::FontManager() {
    if (FT_Init_FreeType(&m_ftLibrary)) {
        panic("failed to load freetype library");
    }
}

FontManager::~FontManager() {
    FT_Done_FreeType(m_ftLibrary);
}

auto FontManager::create() -> std::unique_ptr<FontManager> {
    debug_assert(g_fontManager == nullptr, "only one FontManager may live at any given time");
    auto fontManager = std::make_unique<FontManager>();
    g_fontManager = fontManager.get();
    return fontManager;
}

auto FontManager::loadFont(const std::filesystem::path& path) -> FontFace {
    std::unique_lock lock(m_registryMutex);

    u32 slot = getFreeFontIndex();
    auto& fontEntry = *m_fontEntries[slot];

    {
        std::scoped_lock ftLock(m_ftLibraryMutex);
        if (FT_New_Face(m_ftLibrary, path.string().c_str(), 0, &fontEntry.face)) {
            panic("failed to load font face");
        }
    }

    fontEntry.unitsPerEm = fontEntry.face->units_per_EM;
    fontEntry.ascender = fontEntry.face->ascender;
    fontEntry.descender = fontEntry.face->descender;
    fontEntry.isLoaded = true;

    return FontFace(slot);
}

auto FontManager::freeFont(FontFace font) -> void {
    std::unique_lock lock(m_registryMutex);

    auto& fontEntry = *m_fontEntries[font.handleIndex];
    if (!fontEntry.isLoaded) return;

    std::unique_lock faceLock(fontEntry.rasterMutex);

    {
        std::scoped_lock ftLock(m_ftLibraryMutex);
        FT_Done_Face(fontEntry.face);
    }
    fontEntry.isLoaded = false;
    fontEntry.face = nullptr;
}
auto FontManager::rasterizeGlyphs(FontRasterInfo rasterInfo) -> void {
    std::shared_lock lock(m_registryMutex);

    usize bufferCursor = 0;
    for (auto& batch : rasterInfo.batches) {
        auto& fontEntry = *m_fontEntries[batch.fontFace.handleIndex];
        std::scoped_lock faceLock(fontEntry.rasterMutex);

        FT_Set_Pixel_Sizes(fontEntry.face, 0, batch.pixelSize);

        for (auto& glyph : batch.glyphIndices) {
            if (FT_Load_Char(fontEntry.face, glyph, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT)) {
                log::error("Font Rasterization Error: Failed to rasterize font {} glyph {} at pixel size {}", batch.fontFace.handleIndex, glyph, batch.pixelSize);
                continue;
            }

            FT_GlyphSlot slot = fontEntry.face->glyph;
            FT_Bitmap& bitmap = slot->bitmap;

            u32 imageWidth = bitmap.width;
            u32 imageHeight = bitmap.rows;
            u32 imageSize = imageWidth * imageHeight;

            if (imageSize + bufferCursor > rasterInfo.resultBuffer.size()) {
                // Add some overprovisioning to avoid hot realloc
                rasterInfo.resultBuffer.resize((bufferCursor + imageSize) * 2);
            }

            if (imageSize == 0) {
                // Empty glyph, but we have to encode parameters
                u32 glyphIndex = FT_Get_Char_Index(fontEntry.face, glyph);
                rasterInfo.atlas.insertGlyphParam(batch.fontFace, batch.pixelSize, glyphIndex, math::Vector2f(0.0f, 0.0f), math::Vector2f(0.0f, 0.0f), 0,
    { (float)slot->bitmap_left,  (float)slot->bitmap_top  }, { 0, 0 }, slot->advance.x / 64.0f);
                continue;
            }

            // Find a rect to pack into
            math::Vector2i imageOffset;
            math::Vector2i dstImageSize;
            vk::Image image = nullptr;
            u32 imageIndex = 0;

            for (auto& atlasImage : rasterInfo.atlas.m_atlasTextures) {
                if (auto offset = atlasImage.imagePacker.pack(imageWidth, imageHeight)) {
                    imageOffset = *offset;
                    dstImageSize = { static_cast<i32>(atlasImage.image.extent().width), static_cast<i32>(atlasImage.image.extent().height) };
                    image = *atlasImage.image.vkImage();
                    break;
                }
                imageIndex++;
            }

            if (!image) {
                // Must add a new image to the atlas
                rasterInfo.atlas.pushNewImage(4096, 4096);
                imageIndex = rasterInfo.atlas.m_atlasTextures.size() - 1;
                rasterInfo.newImageIndices.emplace(imageIndex);
                if (auto offset = rasterInfo.atlas.m_atlasTextures.last().imagePacker.pack(imageWidth, imageHeight)) {
                    imageOffset = *offset;
                    dstImageSize = { static_cast<i32>(rasterInfo.atlas.m_atlasTextures.last().image.extent().width), static_cast<i32>(rasterInfo.atlas.m_atlasTextures.last().image.extent().height) };
                    image = *rasterInfo.atlas.m_atlasTextures.last().image.vkImage();
                } else {
                    log::error("Failed to pack glyph into atlas");
                    continue;
                }
            }

            auto bufferImageCopy = vk::BufferImageCopy2{}
                .setBufferImageHeight(0)
                .setBufferOffset(bufferCursor)
                .setBufferRowLength(0)
                .setImageExtent({imageWidth, imageHeight, 1})
                .setImageOffset({imageOffset.x(), imageOffset.y(), 0})
                .setImageSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});

            if (!rasterInfo.copyRegions.contains(imageIndex)) rasterInfo.copyRegions.insert(imageIndex, Vec<vk::BufferImageCopy2>::create());
            rasterInfo.copyRegions[imageIndex].emplace(bufferImageCopy);
            for (u32 row = 0; row < imageHeight; row++) {
                memcpy(
                    rasterInfo.resultBuffer.data() + bufferCursor + row * imageWidth,
                    bitmap.buffer + row * abs(bitmap.pitch),
                    imageWidth);
            }
            bufferCursor += imageHeight * abs(bitmap.pitch);
            bufferCursor = (bufferCursor + 3) & ~3;


            // Fill texture params to the atlas for use in rendering
            math::Vector2f texcoordStart = { static_cast<float>(imageOffset.x()) / static_cast<float>(dstImageSize.x()), static_cast<float>(imageOffset.y()) / static_cast<float>(dstImageSize.y()) };
            math::Vector2f texcoordEnd = { static_cast<float>(imageOffset.x() + imageWidth) / static_cast<float>(dstImageSize.x()), static_cast<float>(imageOffset.y() + imageHeight) / static_cast<float>(dstImageSize.y()) };
            u32 glyphIndex = FT_Get_Char_Index(fontEntry.face, glyph);
            rasterInfo.atlas.insertGlyphParam(batch.fontFace, batch.pixelSize, glyphIndex, texcoordStart, texcoordEnd, rasterInfo.atlas.m_atlasTextures[imageIndex].imageShaderIndex,
                { (float)slot->bitmap_left,  (float)slot->bitmap_top }, { (float)imageWidth, (float)imageHeight }, slot->advance.x / 64.0f);
        }
    }
    rasterInfo.resultBuffer.resize(bufferCursor);
}

math::Vector2f pixelsToNDC(math::Vector2f px, math::Vector2f screenSize) {
    return math::Vector2f(
         (px.x() / screenSize.x()) * 2.0f - 1.0f,
        ((px.y() / screenSize.y()) * 2.0f - 1.0f)
    );
}

auto FontManager::shapeText(FontFace font, rendering::DynamicBitmapFontAtlas& atlas, std::string_view text, u32 pixelSize, math::Vector2f baselineStartPos, math::Vector2f screenSize)
    -> Vec<GlyphInstance> {
    std::shared_lock lock(m_registryMutex);
    auto& fontEntry = *m_fontEntries[font.handleIndex];
    if (!fontEntry.isLoaded) return {};
    std::scoped_lock faceLock(fontEntry.rasterMutex);

    FT_Set_Pixel_Sizes(fontEntry.face, 0, pixelSize);
    bool hasKerning = FT_HAS_KERNING(fontEntry.face);

    auto glyphInstances = Vec<GlyphInstance>::withCapacity(text.size());

    math::Vector2f cursor = baselineStartPos;
    u32 previousGlyphIndex = 0;

    for (auto it = utf8::iterator(text.begin(), text.begin(), text.end()); it != utf8::iterator(text.end(), text.begin(), text.end()); it++) {
        u32 c = *it;

        if (c == '\n') {
            cursor.x() = baselineStartPos.x();
            cursor.y() += fontEntry.face->size->metrics.height / 64.0f;
            previousGlyphIndex = 0;
            continue;
        }
        u32 glyphId = FT_Get_Char_Index(fontEntry.face, c);

        if (hasKerning && previousGlyphIndex && glyphId) {
            FT_Vector kerning;
            FT_Get_Kerning(fontEntry.face, previousGlyphIndex, glyphId, FT_KERNING_DEFAULT, &kerning);
            cursor.x() += kerning.x / 64.0f;
        }

        if (!atlas.hasGlyphParam(font, pixelSize, glyphId)) {
            log::warn("Glyph {} (code {}) not found in atlas! font={} pixelSize={}", glyphId, (char)c, (u32)font.handleIndex, pixelSize);
            continue;
        }

        auto& param = atlas.getGlyphParams(font, pixelSize, glyphId);

        if (param.size.x() == 0 || param.size.y() == 0) {
            // Empty char, skip it, but advance the cursor
            cursor.x() += param.advance;
            previousGlyphIndex = glyphId;
            continue;
        }

        auto ascender = fontEntry.ascender / 64.0f;

        float gx = cursor.x() + param.bearing.x();
        float gy = cursor.y() - param.bearing.y();

        GlyphInstance inst;
        inst.positionStart = pixelsToNDC({ gx,                     gy                      }, screenSize);
        inst.positionEnd   = pixelsToNDC({ gx + param.size.x(),    gy + param.size.y()     }, screenSize);
        inst.texcoordStart = param.texcoordStart;
        inst.texcoordEnd   = param.texcoordEnd;
        inst.imageShaderIndex  = param.imageShaderIndex;
        glyphInstances.emplace(inst);

        cursor.x() += param.advance;
        previousGlyphIndex = glyphId;
    }

    return glyphInstances;
}
auto FontManager::findAndBatchMissingGlyphs(FontFace font, rendering::DynamicBitmapFontAtlas& atlas, std::string_view text, u32 pixelSize)
    -> Option<FontRasterBatch> {
    std::shared_lock lock(m_registryMutex);

    // TODO: a thread could possibly be rasterizing glyphs and updating the atlas, so reading concurrently here is not safe, with a single thread right now
    // TODO: that's not a concern, but could become later when we dispatch threads to do raster work

    auto& fontEntry = *m_fontEntries[font.handleIndex];
    if (!fontEntry.isLoaded) return Option<FontRasterBatch>::none();
    std::scoped_lock faceLock(fontEntry.rasterMutex);

    auto glyphIndices = Vec<u32>::create();
    std::unordered_set<u32> glyphsAlreadyQueued;

    for (auto it = utf8::iterator(text.begin(), text.begin(), text.end()); it != utf8::iterator(text.end(), text.begin(), text.end()); it++) {
        u32 c = *it;

        if (c == '\n') continue;

        auto glyphId = FT_Get_Char_Index(fontEntry.face, c);

        if (!atlas.hasGlyphParam(font, pixelSize, glyphId) && !glyphsAlreadyQueued.contains(glyphId)) {
            glyphIndices.emplace(c);
            glyphsAlreadyQueued.insert(glyphId);
        }
    }

    if (glyphIndices.isEmpty()) return Option<FontRasterBatch>::none();
    return Option<FontRasterBatch>::some(FontRasterBatch { font, pixelSize, std::move(glyphIndices) });
}

u32 FontManager::getFreeFontIndex() {
    // The caller must hold the registry unique lock for this to be safe.
    for (u32 i = 0; i < m_fontEntries.size(); i++) {
        if (!m_fontEntries[i] || !m_fontEntries[i]->isLoaded) return i;
    }
    m_fontEntries.emplace(std::make_unique<FontEntry>());
    return m_fontEntries.size() - 1;
}

} // namespace projnekomata::graphics::fonts