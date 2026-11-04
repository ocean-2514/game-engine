#include "Assets/Font/Font.h"
#include "Assets/Font/FontFace.h"
#include "Renderer/RenderDevice.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/Texture.h"
#include <iostream>

namespace eng {

Font::Font(std::shared_ptr<FontFace> face,
    RenderDevice& device,
    std::shared_ptr<ShaderProgram> glyphShader,
    const GlyphAtlasDesc& atlasDesc) 
    : m_face(std::move(face)), m_device(device), 
    m_shader(std::move(glyphShader)), m_atlas(atlasDesc) {}

std::optional<FontMetrics> Font::GetFontMetrics(
    uint32_t rasterPixelSize) {
    auto it = m_metricsBySize.find(rasterPixelSize);
    if (it != m_metricsBySize.end()) {
        return it->second;
    }
    if (!m_face || !m_face->SetRasterPixelSize(rasterPixelSize)) return std::nullopt;
    auto metrics = m_face->GetMetrics();
    if (!metrics) return std::nullopt;
    m_metricsBySize.insert_or_assign(rasterPixelSize, 
        *metrics);
    return metrics;
}

GlyphId Font::ResolveGlyph(char32_t codePoint) {
    auto id = m_face ? m_face->GetGlyphIndex(codePoint) : InvalidIndex;
    if (id == 0 || id == InvalidIndex) {
        LogMissingOnce(codePoint);
        return GlyphId{0, true}; // missing glyph
    }
    return GlyphId{id, false};
}

std::optional<GlyphMetrics> Font::GetGlyphMetrics(
    uint32_t glyphIndex, uint32_t rasterPixelSize) {
    uint64_t key = Key(glyphIndex, rasterPixelSize);
    if (!EnsureGlyph(glyphIndex, rasterPixelSize)) {
        return std::nullopt;
    }
    return m_glyphs[key].bitmap.metrics;
}

float Font::GetKerning(uint32_t leftGlyph, uint32_t rightGlyph,
    uint32_t rasterPixelSize) {
    return m_face ? m_face->GetKerning(leftGlyph, rightGlyph, rasterPixelSize) : 0.0f;
}

std::optional<GlyphRenderData> Font::GetGlyphRenderData(
    uint32_t glyphIndex, uint32_t rasterPixelSize) {
    if (!EnsureGlyph(glyphIndex, rasterPixelSize)) {
        return std::nullopt;
    }

    uint64_t key = Key(glyphIndex, rasterPixelSize);
    auto& entry = m_glyphs[key];
    if (entry.rasterizationFailed || entry.renderFailed) {
        return std::nullopt;
    }
    if (!entry.render) {
        // Upload to atlas and create render data
        auto& bitmap = entry.bitmap;
        if (!bitmap.metrics.hasBitmap) {
            entry.render = std::nullopt;
            return std::nullopt;
        }
        auto allocOpt = m_atlas.Allocate(bitmap.metrics.width, bitmap.metrics.height);
        if (!allocOpt) {
            m_diagnostics.atlasOverflows++;
            entry.renderFailed = true;
            return std::nullopt;
        }
        if (!m_atlas.Write(*allocOpt, bitmap)) {
            entry.renderFailed = true;
            return std::nullopt;
        }
        GlyphRenderData renderData;
        renderData.page = allocOpt->page;
        renderData.lowerLeftUV.x = static_cast<float>(allocOpt->x) / m_atlas.GetPageWidth();
        renderData.lowerLeftUV.y = static_cast<float>(allocOpt->y) / m_atlas.GetPageHeight();
        renderData.upperRightUV.x = static_cast<float>(allocOpt->x + allocOpt->width) / m_atlas.GetPageWidth();
        renderData.upperRightUV.y = static_cast<float>(allocOpt->y + allocOpt->height) / m_atlas.GetPageHeight();
        renderData.width = bitmap.metrics.width;
        renderData.height = bitmap.metrics.height;
        entry.render = std::move(renderData);
        m_diagnostics.atlasPages = static_cast<uint32_t>(m_atlas.GetPageCount());
        m_pendingUploads.resize(m_atlas.GetPageCount(), 0);
        ++m_pendingUploads[allocOpt->page];

        std::vector<uint8_t>{}.swap(bitmap.pixels);
    }
    return entry.renderFailed ? std::nullopt : entry.render;
}

std::shared_ptr<Texture> Font::GetGlyphTexture(int page) {
    return EnsurePageTexture(page) ? m_pageTextures[static_cast<std::size_t>(page)] : nullptr;
}

std::shared_ptr<ShaderProgram> Font::GetGlyphShader() const {
    return m_shader;
}

const FontDiagnostics& Font::GetDiagnostics() const {
    return m_diagnostics;
}

uint64_t Font::Key(uint32_t glyphIndex, uint32_t rasterPixelSize) {
    return (static_cast<uint64_t>(glyphIndex) << 32) | rasterPixelSize;
}

bool Font::EnsureGlyph(uint32_t glyphIndex, uint32_t rasterPixelSize) {
    uint64_t key = Key(glyphIndex, rasterPixelSize);
    auto it = m_glyphs.find(key);
    if (it != m_glyphs.end()) {
        return !it->second.rasterizationFailed;
    }

    auto bitmapOpt = m_face ? m_face->LoadGlyph(glyphIndex, rasterPixelSize) : std::nullopt;
    if (!bitmapOpt) {
        m_diagnostics.rasterizationFailures++;
        GlyphEntry entry;
        entry.rasterizationFailed = true;
        m_glyphs.insert_or_assign(key, std::move(entry));
        return false;
    }
    m_glyphs.insert_or_assign(key, GlyphEntry{
        std::move(bitmapOpt.value()), 
        std::nullopt, false, false});
    m_diagnostics.rasterizedGlyphs++;
    return true;
}

bool Font::EnsurePageTexture(int page) {
    if (page < 0 || page >= m_atlas.GetPageCount()) return false;
    const auto index = static_cast<std::size_t>(page);
    if (index < m_pageTextures.size() && m_pageTextures[index]) {
        if (!m_atlas.IsPageDirty(page)) {
            return true;
        }
        // Update existing texture
        const auto& pixels = m_atlas.GetPagePixels(page);
        if (pixels.empty()) return false;

        auto& texture = m_pageTextures[page];
        if (!texture || !texture->IsValid()) return false;

        if (!texture->Update(0, 0, m_atlas.GetPageWidth(), m_atlas.GetPageHeight(),
            static_cast<const void*>(pixels.data()), pixels.size())) {
            ++m_diagnostics.uploadFailures;
            return false;
        }
        m_atlas.SetPageDirty(page, false);
        m_diagnostics.uploadedGlyphs += m_pendingUploads[index];
        m_pendingUploads[index] = 0;
        return true;
    }

    if (m_pageTextures.size() <= index) m_pageTextures.resize(index + 1);

    const auto& pixels = m_atlas.GetPagePixels(page);
    if (pixels.empty()) return false;
    
    auto texture = m_device.CreateTexture(
        {
            .width = m_atlas.GetPageWidth(),
            .height = m_atlas.GetPageHeight(),
            .format = TextureFormat::R8,
            .generateMipmaps = false
        },
        {
            .addressU = TextureAddressMode::ClampToEdge,
            .addressV = TextureAddressMode::ClampToEdge,
        },
        static_cast<const void*>(pixels.data()), 
        pixels.size()
    );
    if (!texture || !texture->IsValid()) {
        m_diagnostics.uploadFailures++;
        return false;
    }
    m_pageTextures[page] = std::move(texture);
    m_atlas.SetPageDirty(page, false);
    m_diagnostics.uploadedGlyphs += m_pendingUploads[index];
    m_pendingUploads[index] = 0;

    return true;
}

void Font::LogMissingOnce(char32_t codePoint) {
    if (m_loggedMissing.insert(codePoint).second) {
        ++m_diagnostics.missingCodePoints;
        std::cout << "Font: missing Unicode code point " << static_cast<uint32_t>(codePoint) << '\n';
    }
}


}
