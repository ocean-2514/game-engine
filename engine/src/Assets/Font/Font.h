#ifndef O_FONT
#define O_FONT

#include "Assets/Font/FontDesc.h"
#include "Assets/Font/GlyphAtlas.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace eng {

class FontFace;
class RenderDevice;
class Texture;
class ShaderProgram;

class Font final : public GlyphRenderSource {
public:
    // Main/render-thread use only. The device and graphics context must outlive
    // this font and any shared page textures still held by render commands.
    Font(std::shared_ptr<FontFace> face,
        RenderDevice& device,
        std::shared_ptr<ShaderProgram> glyphShader,
        const GlyphAtlasDesc& atlasDesc);
    ~Font() = default;

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    std::optional<FontMetrics> GetFontMetrics(
        uint32_t rasterPixelSize) override;
    GlyphId ResolveGlyph(char32_t codePoint) override;
    std::optional<GlyphMetrics> GetGlyphMetrics(
        uint32_t glyphIndex, uint32_t rasterPixelSize) override;
    float GetKerning(uint32_t leftGlyph, uint32_t rightGlyph,
        uint32_t rasterPixelSize) override;

    std::optional<GlyphRenderData> GetGlyphRenderData(
        uint32_t glyphIndex, uint32_t rasterPixelSize) override;
    std::shared_ptr<Texture> GetGlyphTexture(int page) override;
    std::shared_ptr<ShaderProgram> GetGlyphShader() const override;

    const FontDiagnostics& GetDiagnostics() const;

private:
    struct GlyphEntry {
        GlyphBitmap bitmap;            // metrics + top-down pixels；写入 CPU 图集后释放 pixels
        std::optional<GlyphRenderData> render;
        bool rasterizationFailed = false;
        bool renderFailed = false;
    };

    static uint64_t Key(uint32_t glyphIndex, uint32_t rasterPixelSize);
    bool EnsureGlyph(uint32_t glyphIndex, uint32_t rasterPixelSize);
    bool EnsurePageTexture(int page);
    void LogMissingOnce(char32_t codePoint);

    std::shared_ptr<FontFace> m_face;
    RenderDevice& m_device;
    std::shared_ptr<ShaderProgram> m_shader;
    GlyphAtlas m_atlas;
    std::unordered_map<uint32_t, FontMetrics> m_metricsBySize;
    std::unordered_map<uint64_t, GlyphEntry> m_glyphs;
    std::vector<std::shared_ptr<Texture>> m_pageTextures;
    std::vector<uint32_t> m_pendingUploads;
    FontDiagnostics m_diagnostics;
    std::unordered_set<char32_t> m_loggedMissing;
};

} // namespace eng

#endif // O_FONT
