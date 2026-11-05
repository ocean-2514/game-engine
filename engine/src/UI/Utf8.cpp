#include "UI/Utf8.h"

namespace eng::utf8 {
bool DecodeNext(std::string_view text, std::size_t& offset, CodePoint& out) {
    if (offset >= text.size()) return false;
    const auto finish = [&](char32_t value, uint32_t length, bool valid) {
        out = {value, static_cast<uint32_t>(offset), length, valid};
        offset += length;
        return true;
    };
    const auto first = static_cast<uint8_t>(text[offset]);
    if (first <= 0x7F) return finish(first, 1, true);

    uint32_t length;
    char32_t cp;
    if (first >= 0xC2 && first <= 0xDF) { length = 2; cp = first & 0x1F; }
    else if (first >= 0xE0 && first <= 0xEF) { length = 3; cp = first & 0x0F; }
    else if (first >= 0xF0 && first <= 0xF4) { length = 4; cp = first & 0x07; }
    else return finish(U'\uFFFD', 1, false);

    for (uint32_t i = 1; i < length; ++i) {
        // Consume only the valid prefix, never the next ASCII or leading byte.
        if (i >= text.size() - offset) return finish(U'\uFFFD', i, false);
        const auto byte = static_cast<uint8_t>(text[offset + i]);
        if ((byte & 0xC0) != 0x80) return finish(U'\uFFFD', i, false);
        if (i == 1 && ((first == 0xE0 && byte < 0xA0) || (first == 0xED && byte > 0x9F)
            || (first == 0xF0 && byte < 0x90) || (first == 0xF4 && byte > 0x8F)))
            return finish(U'\uFFFD', 1, false);
        cp = (cp << 6) | (byte & 0x3F);
    }
    return finish(cp, length, true);
}
} // namespace eng::utf8
