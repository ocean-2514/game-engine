#ifndef O_UTF8
#define O_UTF8

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace eng::utf8 {

struct CodePoint {
    char32_t value = U'\0';
    uint32_t byteOffset = 0;
    uint32_t byteLength = 0;
    bool valid = true;
};

// 解码一个 codepoint；到末尾返回 false。
// 非法序列返回 value=U+FFFD、valid=false，并至少前进 1 字节。
bool DecodeNext(std::string_view text, std::size_t& offset, CodePoint& out);

} // namespace eng::utf8

#endif // O_UTF8