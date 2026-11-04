#ifndef O_FONT_DESC
#define O_FONT_DESC

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace eng
{
    
class Texture;
class ShaderProgram;

struct FontMetrics {
    float ascender = 0.0f;         // 物理像素，基线以上为正
    float descender = 0.0f;        // 物理像素，基线以下为负
    float lineHeight = 0.0f;       // 物理像素，建议行距
    float underlinePosition = 0.0f;
    float underlineThickness = 0.0f;
};

struct GlyphId {
    uint32_t index = 0;            // FreeType glyph index；0 是 .notdef
    bool missing = false;          // 原 codepoint 未映射，使用了 fallback
};

struct GlyphMetrics {
    float advanceX = 0.0f;         // 物理像素
    float advanceY = 0.0f;         // 横向排版时为 0
    float bearingX = 0.0f;         // 物理像素，相对 pen
    float bearingY = 0.0f;         // 物理像素，相对基线向上
    uint32_t width = 0;            // bitmap 宽，物理像素（整数）
    uint32_t height = 0;           // bitmap 高，物理像素（整数）
    bool hasBitmap = false;        // 空格等为 false
};

struct GlyphBitmap {
    GlyphMetrics metrics;
    std::vector<uint8_t> pixels;   // 紧排、top-down、每行 width 字节
    uint32_t pitch = 0;            // FontFace 输出等于 width；GlyphAtlas::Write 也接受行尾 padding
};

struct GlyphRenderData {
    int page = -1;                 // Font 的图集页索引
    glm::vec2 lowerLeftUV{0.0f};   // 屏幕下方采样点
    glm::vec2 upperRightUV{1.0f};  // 屏幕上方采样点
    uint32_t width = 0;            // 物理像素
    uint32_t height = 0;
};

struct FontDiagnostics {
    uint32_t rasterizedGlyphs = 0;
    uint32_t uploadedGlyphs = 0;   // 成功同步到 GPU 的不同字形数量，不含页面重复上传
    uint32_t atlasPages = 0;
    uint32_t atlasOverflows = 0;
    uint32_t missingCodePoints = 0;
    uint32_t rasterizationFailures = 0;
    uint32_t uploadFailures = 0;
};

// 排版只需要度量；测试可提供假实现。
class GlyphProvider {
public:
    virtual ~GlyphProvider() = default;
    virtual std::optional<FontMetrics> GetFontMetrics(
        uint32_t rasterPixelSize) = 0;
    virtual GlyphId ResolveGlyph(char32_t codePoint) = 0;
    virtual std::optional<GlyphMetrics> GetGlyphMetrics(
        uint32_t glyphIndex, uint32_t rasterPixelSize) = 0;
    virtual float GetKerning(
        uint32_t leftGlyph, uint32_t rightGlyph,
        uint32_t rasterPixelSize) = 0;
};

// 绘制还需要图集纹理与 shader。
class GlyphRenderSource : public GlyphProvider {
public:
    virtual std::optional<GlyphRenderData> GetGlyphRenderData(
        uint32_t glyphIndex, uint32_t rasterPixelSize) = 0;
    virtual std::shared_ptr<Texture> GetGlyphTexture(int page) = 0;
    virtual std::shared_ptr<ShaderProgram> GetGlyphShader() const = 0;
};

struct FontRasterizationOptions {
    bool hinting = true;
    bool lightHinting = false;     // FT_LOAD_TARGET_LIGHT；与 hinting 组合使用
};

struct GlyphAtlasDesc {
    uint32_t pageWidth = 1024;
    uint32_t pageHeight = 1024;
    uint32_t maxPages = 4;
    uint32_t padding = 1;          // 每个字形四周的空白物理像素

    bool operator==(const GlyphAtlasDesc& other) const {
        return pageWidth == other.pageWidth
            && pageHeight == other.pageHeight
            && maxPages == other.maxPages
            && padding == other.padding;
    }
};

struct FontAssetDesc {
    std::string path;              // 相对 assets 的路径
    int faceIndex = 0;             // TTC 中的 face；普通 TTF/OTF 为 0
    FontRasterizationOptions rasterization;
    GlyphAtlasDesc atlas;
};




} // namespace eng


#endif // O_FONT_DESC
