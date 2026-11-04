#ifndef O_FONT_FACE
#define O_FONT_FACE

#include "Assets/Font/FontDesc.h"

#include <memory>
#include <string>
#include <vector>
#include <limits>
#include <unordered_map>

namespace eng {

inline constexpr uint32_t InvalidSize = 
    std::numeric_limits<uint32_t>::max();
inline constexpr uint32_t InvalidIndex = 
    std::numeric_limits<uint32_t>::max();

class FontFace {
public:
    // Unicode faces; mutable size/glyph state is confined to the calling thread.
    // 失败返回 nullptr。fileData 被 face 持有，调用方无需保活。
    static std::shared_ptr<FontFace> Load(
        std::vector<char> fileData,
        int faceIndex,
        const FontRasterizationOptions& options);
    ~FontFace();

    FontFace(const FontFace&) = delete;
    FontFace& operator=(const FontFace&) = delete;

    int GetFaceCount() const;
    std::string GetFamilyName() const;

    bool SetRasterPixelSize(uint32_t size); // 1..65535 physical pixels
    uint32_t GetRasterPixelSize() const;
    std::optional<FontMetrics> GetMetrics();
    uint32_t GetGlyphIndex(char32_t codePoint) const;
    std::optional<GlyphBitmap> LoadGlyph(
        uint32_t glyphIndex, uint32_t rasterPixelSize);
    float GetKerning(
        uint32_t leftGlyph, uint32_t rightGlyph,
        uint32_t rasterPixelSize);

    bool IsValid() const;

private:
    struct Impl;
    explicit FontFace(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> m_impl;
    uint32_t m_currentSize = InvalidSize;
    FontMetrics m_metrics{};
};

} // namespace eng

#endif // O_FONT_FACE
