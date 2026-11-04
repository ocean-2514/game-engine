#ifndef O_GLYPH_ATLAS
#define O_GLYPH_ATLAS

#include "Assets/Font/FontDesc.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace eng {

struct GlyphAllocation {
    int page = -1;
    uint32_t x = 0;       // 内容区左下角（不含 padding）
    uint32_t y = 0;       // 左下原点
    uint32_t width = 0;   // 内容宽
    uint32_t height = 0;
};

class GlyphAtlas {
public:
    explicit GlyphAtlas(const GlyphAtlasDesc& desc);

    // width/height 为字形 bitmap 尺寸，不含 padding。
    // 页满且未超过 maxPages 时自动新建页；仍失败返回 nullopt。
    std::optional<GlyphAllocation> Allocate(
        uint32_t width, uint32_t height);

    // 把 top-down 的 bitmap 翻转写入左下原点的页。
    bool Write(const GlyphAllocation& allocation,
        const GlyphBitmap& bitmap);

    bool IsValid() const;

    int GetPageCount() const;
    uint32_t GetPageWidth() const;
    uint32_t GetPageHeight() const;
    uint32_t GetPadding() const;
    const std::vector<uint8_t>& GetPagePixels(int page) const;
    void SetPageDirty(int page, bool dirty);
    bool IsPageDirty(int page) const;

private:
    struct Shelf {
        uint32_t y = 0;
        uint32_t height = 0;
        uint32_t cursorX = 0;
    };
    struct Page {
        Page(uint32_t width, uint32_t height);
        std::vector<Shelf> shelves;
        uint32_t usedHeight = 0;
        std::vector<uint8_t> pixels; // width * height，R8，左下原点
    };
    struct PageSlot {
        Page page;
        bool dirty = true;
    };

    std::optional<GlyphAllocation> TryAllocateInExistingPage(
        uint32_t width, uint32_t height);

    GlyphAtlasDesc m_desc;
    std::vector<PageSlot> m_pages;
};

} // namespace eng

#endif // O_GLYPH_ATLAS
