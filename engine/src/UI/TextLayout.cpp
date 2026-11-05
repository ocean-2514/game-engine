#include "UI/TextLayout.h"
#include "UI/Utf8.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string_view>

namespace eng {
namespace {
bool IsSpace(char32_t cp) { return cp == U' ' || cp == U'\t'; }
bool IsCjk(char32_t cp) {
    return (cp >= 0x3400 && cp <= 0x9FFF) || (cp >= 0x20000 && cp <= 0x323AF)
        || (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0x3040 && cp <= 0x30FF)
        || (cp >= 0xAC00 && cp <= 0xD7AF);
}
bool CanBreakBetween(char32_t left, char32_t right) {
    // A deliberately small CJK rule set, not full Unicode line breaking.
    constexpr std::u32string_view opening = U"([{\uFF08\u300C\u300E\u3010\u300A\u3008\u3014\u2018\u201C";
    constexpr std::u32string_view closing = U")]},.!?:;\uFF09\u300D\u300F\u3011\u300B\u3009\u3015\u3002\uFF0C\u3001\uFF01\uFF1F\uFF1A\uFF1B\u2019\u201D";
    if (opening.find(left) != opening.npos || closing.find(right) != closing.npos) return false;
    return IsSpace(left) || IsCjk(left) || IsCjk(right);
}
float PositiveOr(float value, float fallback) {
    return std::isfinite(value) && value > 0 ? value : fallback;
}
bool FitsFloat(double value) {
    return std::isfinite(value) && std::abs(value) <= std::numeric_limits<float>::max();
}
struct Item {
    GlyphPlacement placement;
    double advance = 0;
    double kern = 0; // From the preceding item, excluded at the start of a line.
};
struct Paragraph {
    std::size_t begin, end;
    uint32_t byteOffset;
};
struct LineRange {
    std::size_t begin, end;
    uint32_t byteOffset;
    double width;
};
} // namespace

TextLayoutResult TextLayout::Build(const TextLayoutParams& params, GlyphProvider& font) {
    // Source offsets in the public result are 32-bit.
    if (params.text.size() > std::numeric_limits<uint32_t>::max()) return {};
    TextLayoutResult result;
    const double scale = PositiveOr(params.contentScale, 1.0f);
    const double pixelSize = PositiveOr(params.pixelSize, 16.0f);
    const double spacing = PositiveOr(params.lineSpacing, 1.0f);
    const auto tabSize = std::clamp(params.tabSize, 1u, 32u);
    result.rasterPixelSize = static_cast<uint32_t>(std::clamp(std::round(pixelSize * scale), 1.0, 256.0));
    if (params.text.empty()) { result.valid = true; return result; }

    const auto metrics = font.GetFontMetrics(result.rasterPixelSize);
    if (!metrics || !std::isfinite(metrics->ascender) || !std::isfinite(metrics->descender)
        || !std::isfinite(metrics->lineHeight)) return {};
    const double ascent = std::max(0.0f, metrics->ascender);
    const double descent = std::max(0.0f, -metrics->descender);
    const double cellHeight = ascent + descent > 0 ? ascent + descent : metrics->lineHeight;
    const double lineStep = (metrics->lineHeight > 0 ? metrics->lineHeight : cellHeight) * spacing;
    if (cellHeight <= 0 || lineStep <= 0) return {};
    const double boxWidth = std::isfinite(params.boxSize.x) && params.boxSize.x > 0
        ? params.boxSize.x * scale : 0;
    const double boxHeight = std::isfinite(params.boxSize.y) && params.boxSize.y > 0
        ? params.boxSize.y * scale : 0;
    const double wrapWidth = params.wrap != TextWrap::None && boxWidth > 0
        ? boxWidth : std::numeric_limits<double>::infinity();

    // Resolve glyphs once. Keep paragraph breaks separate from drawable items.
    std::vector<Item> items;
    std::vector<Paragraph> paragraphs;
    std::size_t paragraphStart = 0;
    uint32_t paragraphByte = 0;
    bool paragraphHasText = false;
    std::optional<uint32_t> previous;
    std::size_t offset = 0;
    utf8::CodePoint decoded;
    while (utf8::DecodeNext(params.text, offset, decoded)) {
        const auto cp = decoded.value; // Invalid UTF-8 already carries U+FFFD.
        if (cp == U'\r' || cp == U'\n') {
            if (cp == U'\r' && offset < params.text.size() && params.text[offset] == '\n') ++offset;
            paragraphs.push_back({paragraphStart, items.size(), paragraphByte});
            paragraphStart = items.size();
            paragraphByte = static_cast<uint32_t>(offset);
            paragraphHasText = false;
            previous.reset();
            continue;
        }
        if ((decoded.byteOffset == 0 && cp == 0xFEFF)
            || (cp < 0x20 && cp != U'\t') || cp == 0x7F) continue;
        paragraphHasText = true;

        const auto id = font.ResolveGlyph(cp == U'\t' ? U' ' : cp);
        auto glyph = font.GetGlyphMetrics(id.index, result.rasterPixelSize);
        const bool missing = !decoded.valid || id.missing || !glyph;
        if (missing) ++result.missingGlyphCount;
        if (!glyph && cp == U'\t') glyph = GlyphMetrics{}; // Fall back to half-em spaces below.
        if (!glyph) { previous.reset(); continue; }
        if (!std::isfinite(glyph->advanceX) || glyph->advanceX < 0
            || !std::isfinite(glyph->bearingX) || !std::isfinite(glyph->bearingY)) return {};

        Item item;
        item.placement.glyphIndex = id.index;
        item.placement.codePoint = cp;
        item.placement.sourceByteOffset = decoded.byteOffset;
        item.placement.missing = missing;
        item.placement.position = {glyph->bearingX, -glyph->bearingY};
        item.placement.size = glyph->hasBitmap && !IsSpace(cp)
            ? glm::vec2(glyph->width, glyph->height) : glm::vec2(0);
        item.advance = glyph->advanceX;
        if (cp == U'\t') {
            // Fixed tabSize spaces. Keeping tabs as layout items also lets them wrap.
            item.advance = (item.advance > 0 ? item.advance : result.rasterPixelSize * 0.5) * tabSize;
            previous.reset();
        } else {
            if (previous) item.kern = font.GetKerning(*previous, id.index, result.rasterPixelSize);
            previous = id.index;
        }
        if (!std::isfinite(item.kern)) return {};
        items.push_back(item);
    }
    // A trailing hard break terminates the last line, but does not add a phantom line.
    if (paragraphHasText) paragraphs.push_back({paragraphStart, items.size(), paragraphByte});

    std::vector<double> advances(items.size() + 1, 0), kerns(items.size() + 1, 0);
    for (std::size_t i = 0; i < items.size(); ++i) {
        advances[i + 1] = advances[i] + items[i].advance;
        kerns[i + 1] = kerns[i] + items[i].kern;
    }
    const auto width = [&](std::size_t begin, std::size_t end) {
        return end == begin ? 0.0 : std::max(0.0, advances[end] - advances[begin]
            + kerns[end] - kerns[begin + 1]);
    };
    std::vector<LineRange> ranges;
    double maxLineWidth = 0;
    const auto addLine = [&](std::size_t begin, std::size_t end, uint32_t emptyByte) {
        while (end > begin && IsSpace(items[end - 1].placement.codePoint)) --end;
        const auto lineWidth = width(begin, end);
        ranges.push_back({begin, end, begin < end ? items[begin].placement.sourceByteOffset : emptyByte,
            lineWidth});
        maxLineWidth = std::max(maxLineWidth, lineWidth);
    };

    // Choose line ranges before positioning. No pending glyphs or coordinate rebasing.
    for (const auto& paragraph : paragraphs) {
        const auto before = ranges.size();
        auto begin = paragraph.begin;
        auto cursor = begin;
        auto lastBreak = begin;
        while (cursor < paragraph.end) {
            if (cursor > begin && CanBreakBetween(items[cursor - 1].placement.codePoint,
                items[cursor].placement.codePoint)) lastBreak = cursor;
            if (cursor > begin && width(begin, cursor + 1) > wrapWidth) {
                // An overflowing trailing space should not move the preceding word.
                const auto end = params.wrap == TextWrap::Word && lastBreak > begin
                    && !IsSpace(items[cursor].placement.codePoint) ? lastBreak : cursor;
                auto trimmed = end;
                while (trimmed > begin && IsSpace(items[trimmed - 1].placement.codePoint)) --trimmed;
                if (trimmed > begin) addLine(begin, trimmed, paragraph.byteOffset);
                begin = end;
                while (begin < paragraph.end && IsSpace(items[begin].placement.codePoint)) ++begin;
                cursor = lastBreak = begin;
            } else {
                ++cursor; // Even an over-wide first glyph is consumed, so we always progress.
            }
        }
        if (begin < paragraph.end) addLine(begin, paragraph.end, paragraph.byteOffset);
        else if (ranges.size() == before) addLine(paragraph.begin, paragraph.begin, paragraph.byteOffset);
    }

    const double totalHeight = ranges.empty() ? 0 : static_cast<double>(ranges.size() - 1) * lineStep + cellHeight;
    double offsetY = 0;
    if (boxHeight > 0) {
        if (params.verticalAlign == TextVerticalAlign::Middle) offsetY = (boxHeight - totalHeight) / 2;
        else if (params.verticalAlign == TextVerticalAlign::Bottom) offsetY = boxHeight - totalHeight;
    }
    if (!FitsFloat(totalHeight / scale) || !FitsFloat(maxLineWidth / scale)) return {};

    for (const auto& range : ranges) {
        const double availableWidth = boxWidth > 0 ? boxWidth : maxLineWidth;
        double offsetX = 0;
        if (params.align == TextAlign::Center) offsetX = (availableWidth - range.width) / 2;
        else if (params.align == TextAlign::Right) offsetX = availableWidth - range.width;
        const double baseline = static_cast<double>(result.lines.size()) * lineStep + ascent + offsetY;
        if (!FitsFloat(baseline) || !FitsFloat(range.width)) return {};
        TextLine line;
        line.firstGlyph = static_cast<uint32_t>(result.glyphs.size());
        line.sourceByteOffset = range.byteOffset;
        line.width = static_cast<float>(range.width);
        line.baseline = static_cast<float>(baseline);
        for (auto i = range.begin; i < range.end; ++i) {
            auto placement = items[i].placement;
            double x = offsetX + advances[i] - advances[range.begin]
                + kerns[i + 1] - kerns[range.begin + 1] + placement.position.x;
            double y = baseline + placement.position.y;
            if (params.pixelSnap) { x = std::round(x); y = std::round(y); }
            if (!FitsFloat(x) || !FitsFloat(y)) return {};
            placement.position = {static_cast<float>(x), static_cast<float>(y)};
            result.glyphs.push_back(placement);
        }
        line.glyphCount = static_cast<uint32_t>(result.glyphs.size()) - line.firstGlyph;
        result.lines.push_back(line);
    }
    result.measuredSize = {static_cast<float>(maxLineWidth / scale), static_cast<float>(totalHeight / scale)};
    result.valid = true;
    return result;
}
} // namespace eng
