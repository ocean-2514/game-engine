#include "Assets/Font/FontFace.h"

#include <freetype/freetype.h>
#include <iostream>
#include <cstring>
#include <limits>

namespace eng
{
    
struct FontFace::Impl {
    FT_Library library = nullptr;   
    FT_Face face = nullptr;
    std::vector<char> fileData;     
    FontRasterizationOptions options;

    ~Impl() {
        if (face) {
            FT_Done_Face(face);
            face = nullptr;
        }
        if (library) {
            FT_Done_FreeType(library);
            library = nullptr;
        }
    }
};

FontFace::FontFace(std::unique_ptr<Impl> impl) 
    : m_impl(std::move(impl)) {}

std::shared_ptr<FontFace> FontFace::Load(
    std::vector<char> fileData,
    int faceIndex,
    const FontRasterizationOptions& options) {
    if (fileData.empty() || faceIndex < 0
        || fileData.size() > static_cast<std::size_t>(std::numeric_limits<FT_Long>::max()))
        return nullptr;
    auto impl = std::make_unique<Impl>();
    impl->fileData = std::move(fileData);
    impl->options = options;

    FT_Error error = FT_Init_FreeType(&impl->library);
    if (error != FT_Err_Ok) return nullptr;

    error = FT_New_Memory_Face(impl->library, 
        reinterpret_cast<const FT_Byte*>(impl->fileData.data()),
        static_cast<FT_Long>(impl->fileData.size()), 
        faceIndex, &impl->face);
    if (error != FT_Err_Ok) return nullptr;

    if (FT_Select_Charmap(impl->face, FT_ENCODING_UNICODE) != FT_Err_Ok)
        return nullptr;
    return std::shared_ptr<FontFace>(new FontFace(std::move(impl)));
}

FontFace::~FontFace() = default;

int FontFace::GetFaceCount() const {
    if (!m_impl || !m_impl->face) return 0;
    return static_cast<int>(m_impl->face->num_faces);
}

std::string FontFace::GetFamilyName() const {
    if (!m_impl || !m_impl->face) return {};
    char* name = m_impl->face->family_name;
    return name ? name : "";
}

bool FontFace::SetRasterPixelSize(uint32_t rasterPixelSize) {
    if (!m_impl || !m_impl->face) return false;
    if (rasterPixelSize == 0 || rasterPixelSize >= 65536) {
        std::cout << "FontFace::GetMetrics: invalid raster pixel size: "
            << rasterPixelSize << std::endl;
        return false;
    }
    if (rasterPixelSize == m_currentSize) return true;

    auto error = FT_Set_Pixel_Sizes(m_impl->face, 0, rasterPixelSize);
    if (error != FT_Err_Ok) return false;

    auto* face = m_impl->face;
    m_metrics.ascender = face->size->metrics.ascender / 64.0f;
    m_metrics.descender = face->size->metrics.descender / 64.0f;
    m_metrics.lineHeight = face->size->metrics.height / 64.0f;
    m_metrics.underlinePosition = FT_MulFix(face->underline_position, 
        face->size->metrics.y_scale) / 64.0f;
    m_metrics.underlineThickness = FT_MulFix(face->underline_thickness, 
        face->size->metrics.y_scale) / 64.0f;

    m_currentSize = rasterPixelSize;
    return true;
}

uint32_t FontFace::GetRasterPixelSize() const {
    return m_currentSize;
}

std::optional<FontMetrics> FontFace::GetMetrics() {
    if (!m_impl || !m_impl->face 
        || m_currentSize == InvalidSize) return std::nullopt;
    return m_metrics;
}

uint32_t FontFace::GetGlyphIndex(char32_t codePoint) const {
    if (!m_impl || !m_impl->face) return InvalidIndex;
    if (codePoint > 0x10FFFF || (codePoint >= 0xD800 && codePoint <= 0xDFFF)) return 0;
    return FT_Get_Char_Index(m_impl->face, 
        static_cast<FT_ULong>(codePoint));
}

std::optional<GlyphBitmap> FontFace::LoadGlyph(
    uint32_t glyphIndex, uint32_t rasterPixelSize) {
    if (!m_impl || !m_impl->face) return std::nullopt;
    if (rasterPixelSize == 0 || rasterPixelSize >= 65536
        || glyphIndex >= static_cast<uint32_t>(m_impl->face->num_glyphs)) return std::nullopt;
    if (rasterPixelSize != m_currentSize) {
        if(!SetRasterPixelSize(rasterPixelSize)) return std::nullopt;
    }

    FT_Int32 flags = FT_LOAD_RENDER;
    if (!m_impl->options.hinting) flags |= FT_LOAD_NO_HINTING;
    if (m_impl->options.hinting && m_impl->options.lightHinting) flags |= FT_LOAD_TARGET_LIGHT;

    auto error = FT_Load_Glyph(m_impl->face, glyphIndex, flags);
    if (error != FT_Err_Ok) return std::nullopt;

    FT_GlyphSlot slot = m_impl->face->glyph;
    GlyphBitmap out;
    out.metrics.advanceX = slot->advance.x / 64.0f;
    out.metrics.advanceY = slot->advance.y / 64.0f;
    out.metrics.bearingX = static_cast<float>(slot->bitmap_left);
    out.metrics.bearingY = static_cast<float>(slot->bitmap_top);
    out.metrics.height = slot->bitmap.rows;
    out.metrics.width = slot->bitmap.width;
    out.metrics.hasBitmap = slot->bitmap.buffer != nullptr
        && slot->bitmap.width > 0 && slot->bitmap.rows > 0;
    
    const auto& bitmap = slot->bitmap;
    out.pitch = bitmap.width;
    if (out.metrics.hasBitmap) {
        if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY && bitmap.pixel_mode != FT_PIXEL_MODE_MONO)
            return std::nullopt; // R8 coverage only; no LCD or colour glyphs yet.
        const auto stride = bitmap.pitch < 0 ? -static_cast<int64_t>(bitmap.pitch) : bitmap.pitch;
        const auto rowBytes = bitmap.pixel_mode == FT_PIXEL_MODE_MONO
            ? (static_cast<std::size_t>(bitmap.width) + 7) / 8 : bitmap.width;
        if (stride < static_cast<int64_t>(rowBytes)
            || bitmap.width > out.pixels.max_size() / bitmap.rows) return std::nullopt;
        out.pixels.resize(static_cast<std::size_t>(bitmap.width) * bitmap.rows);
        const auto* top = bitmap.buffer;
        if (bitmap.pitch < 0) top += static_cast<std::size_t>(stride) * (bitmap.rows - 1);
        for (uint32_t y = 0; y < bitmap.rows; ++y) {
            const auto* row = top + static_cast<std::ptrdiff_t>(y) * bitmap.pitch;
            auto* dst = out.pixels.data() + static_cast<std::size_t>(y) * bitmap.width;
            if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
                for (uint32_t x = 0; x < bitmap.width; ++x)
                    dst[x] = (row[x / 8] & (0x80u >> (x % 8))) ? 255 : 0;
            } else {
                if (bitmap.num_grays < 2) return std::nullopt;
                for (uint32_t x = 0; x < bitmap.width; ++x)
                    dst[x] = static_cast<uint8_t>(static_cast<unsigned>(row[x]) * 255 / (bitmap.num_grays - 1));
            }
        }
    }
    return out;
}
    
float FontFace::GetKerning(
    uint32_t leftGlyph, uint32_t rightGlyph,
    uint32_t rasterPixelSize) {
    if (!m_impl || !m_impl->face) return 0.0f;
    if (rasterPixelSize == 0 || rasterPixelSize >= 65536) return 0.0f;
    if (rasterPixelSize != m_currentSize) {
        if(!SetRasterPixelSize(rasterPixelSize)) return 0.0f;
    }

    FT_Vector kerning;
    auto error = FT_Get_Kerning(m_impl->face, leftGlyph, rightGlyph, FT_KERNING_DEFAULT, &kerning);
    if (error != FT_Err_Ok) return 0.0f;

    return kerning.x / 64.0f;
}
    
bool FontFace::IsValid() const {
    return m_impl && m_impl->face;
}

} // namespace eng
