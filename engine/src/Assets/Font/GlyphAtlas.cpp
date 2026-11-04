#include "Assets/Font/GlyphAtlas.h"

#include <cstring>
#include <limits>
#include <algorithm>

namespace eng
{

GlyphAtlas::Page::Page(uint32_t width, uint32_t height) {
    pixels.resize(static_cast<std::size_t>(width) * height, 0);
}
    
GlyphAtlas::GlyphAtlas(const GlyphAtlasDesc& desc) 
    : m_desc(desc) {}

bool GlyphAtlas::IsValid() const {
    const auto maxInt = static_cast<uint32_t>(std::numeric_limits<int>::max());
    if (m_desc.pageWidth == 0 || m_desc.pageHeight == 0 || m_desc.maxPages == 0
        || m_desc.pageWidth > maxInt || m_desc.pageHeight > maxInt
        || m_desc.maxPages > maxInt) return false;
    if (m_desc.padding > (std::min(m_desc.pageWidth, m_desc.pageHeight) - 1) / 2)
        return false;
    return m_desc.pageWidth <= std::vector<uint8_t>{}.max_size() / m_desc.pageHeight;
}


std::optional<GlyphAllocation> GlyphAtlas::Allocate(
    uint32_t width, uint32_t height) {
    if (!IsValid() || width == 0 || height == 0
        || width > m_desc.pageWidth - 2 * m_desc.padding
        || height > m_desc.pageHeight - 2 * m_desc.padding) return std::nullopt;
    uint32_t paddedWidth = width + 2 * m_desc.padding;
    uint32_t paddedHeight = height + 2 * m_desc.padding;

    if (paddedHeight > m_desc.pageHeight 
        || paddedWidth > m_desc.pageWidth) return std::nullopt;
    if (auto a = TryAllocateInExistingPage(width, height)) {
        return a;
    }

    if (m_pages.size() >= m_desc.maxPages) {
        return std::nullopt;
    }
    m_pages.push_back({Page{m_desc.pageWidth, m_desc.pageHeight}, true});
    auto& page = m_pages.back().page;
    page.shelves.push_back({0, paddedHeight, 0});
    page.usedHeight += paddedHeight;

    auto& shelf = page.shelves.back();
    shelf.cursorX += paddedWidth;
    GlyphAllocation allocation;
    allocation.x = m_desc.padding;
    allocation.y = m_desc.padding;
    allocation.width = width;
    allocation.height = height;
    allocation.page = static_cast<int>(m_pages.size() - 1);
    
    return allocation;
}
    
bool GlyphAtlas::Write(const GlyphAllocation& allocation,
    const GlyphBitmap& bitmap) {
    if (allocation.page < 0 || allocation.page >= static_cast<int>(m_pages.size())) {
        return false;
    }
    if (allocation.width != bitmap.metrics.width 
        || allocation.height != bitmap.metrics.height) {
        return false;
    }
    if (allocation.width == 0 || allocation.height == 0
        || allocation.x > m_desc.pageWidth || allocation.y > m_desc.pageHeight
        || allocation.width > m_desc.pageWidth - allocation.x
        || allocation.height > m_desc.pageHeight - allocation.y
        || bitmap.pitch < allocation.width) {
        return false;
    }
    // Last row need not include trailing padding; check without overflowing.
    if (bitmap.pixels.size() < allocation.width
        || allocation.height - 1 > (bitmap.pixels.size() - allocation.width) / bitmap.pitch) {
        return false;
    }
    m_pages[allocation.page].dirty = true;
    for (uint32_t row = 0; row < allocation.height; ++row) {
        const uint8_t* srcRow = bitmap.pixels.data() + static_cast<std::size_t>(row) * bitmap.pitch;
        uint8_t* dstRow = m_pages[allocation.page].page.pixels.data() 
            + static_cast<std::size_t>(allocation.y + allocation.height - 1 - row) * m_desc.pageWidth
            + allocation.x;
        std::memcpy(dstRow, srcRow, allocation.width);
    }
    return true;
}
    

int GlyphAtlas::GetPageCount() const {
    return static_cast<int>(m_pages.size());
}

uint32_t GlyphAtlas::GetPageWidth() const {
    return m_desc.pageWidth;
}

uint32_t GlyphAtlas::GetPageHeight() const {
    return m_desc.pageHeight;
}

uint32_t GlyphAtlas::GetPadding() const {
    return m_desc.padding;
}

const std::vector<uint8_t>& GlyphAtlas::GetPagePixels(int page) const {
    if (page < 0 || page >= static_cast<int>(m_pages.size())) {
        static const std::vector<uint8_t> empty;
        return empty;
    }
    return m_pages[page].page.pixels;
}

void GlyphAtlas::SetPageDirty(int page, bool dirty) {
    if (page < 0 || page >= static_cast<int>(m_pages.size())) {
        return;
    }
    m_pages[page].dirty = dirty;
}

bool GlyphAtlas::IsPageDirty(int page) const {
    if (page < 0 || page >= static_cast<int>(m_pages.size())) {
        return false;
    }
    return m_pages[page].dirty;
}

std::optional<GlyphAllocation> GlyphAtlas::TryAllocateInExistingPage(
    uint32_t width, uint32_t height) {
    uint32_t paddedWidth = width + 2 * m_desc.padding;
    uint32_t paddedHeight = height + 2 * m_desc.padding;

    for (std::size_t i = 0; i < m_pages.size(); ++i) {
        auto& page = m_pages[i].page;
        for (auto& shelf : page.shelves) {
            if (shelf.height >= paddedHeight 
                && paddedWidth <= m_desc.pageWidth - shelf.cursorX) {
                GlyphAllocation allocation;
                allocation.page = static_cast<int>(i);
                allocation.x = shelf.cursorX + m_desc.padding;
                allocation.y = shelf.y + m_desc.padding;
                allocation.width = width;
                allocation.height = height;

                shelf.cursorX += paddedWidth;
                return allocation;
            }
        }
        if (paddedHeight <= m_desc.pageHeight - page.usedHeight) {
            page.shelves.push_back({page.usedHeight, paddedHeight, paddedWidth});
            page.usedHeight += paddedHeight;
            GlyphAllocation allocation;
            allocation.page = static_cast<int>(i);
            allocation.x = m_desc.padding;
            allocation.y = page.usedHeight + m_desc.padding - paddedHeight;
            allocation.width = width;
            allocation.height = height;

            page.shelves.back().cursorX = paddedWidth;
            return allocation;
        }
    }

    return std::nullopt;
}


} // namespace eng
