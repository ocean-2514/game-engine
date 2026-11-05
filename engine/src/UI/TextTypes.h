#ifndef O_UI_TEXT_TYPES
#define O_UI_TEXT_TYPES

#include "Assets/Font/FontDesc.h"

#include <cstdint>
#include <string_view>
#include <vector>
#include <glm/glm.hpp>

namespace eng {

enum class TextWrap { None, Word, Character };
enum class TextAlign { Left, Center, Right };
enum class TextVerticalAlign { Top, Middle, Bottom };

struct TextStyle {
    float pixelSize = 16.0f;       // 逻辑像素 em 尺寸
    glm::vec4 color{1.0f};
    TextAlign align = TextAlign::Left;
    TextVerticalAlign verticalAlign = TextVerticalAlign::Top;
    TextWrap wrap = TextWrap::Word;
    float lineSpacing = 1.0f;      // 行距倍数，1.0 = 字体建议行高
    bool pixelSnap = true;         // 字形物理原点对齐到整数像素
    uint32_t tabSize = 4;          // 制表符按空格数展开
};

struct GlyphPlacement {
    uint32_t glyphIndex = 0;
    char32_t codePoint = U'\0';
    uint32_t sourceByteOffset = 0;
    glm::vec2 position{0.0f};      // 物理像素，相对文本框左上角
    glm::vec2 size{0.0f};          // 物理像素；空格/Tab/无 bitmap 字形为 0
    bool missing = false;          // 使用了 fallback/.notdef
};

struct TextLine {
    uint32_t firstGlyph = 0;
    uint32_t glyphCount = 0;
    uint32_t sourceByteOffset = 0;
    float width = 0.0f;            // 物理像素，已去掉行尾空格
    float baseline = 0.0f;         // 物理像素，相对文本框顶部
};

struct TextLayoutResult {
    std::vector<GlyphPlacement> glyphs;
    std::vector<TextLine> lines;
    glm::vec2 measuredSize{0.0f};  // 逻辑像素
    uint32_t rasterPixelSize = 0;  // 实际栅格字号
    uint32_t missingGlyphCount = 0; // 受影响的源字符数；解码/映射/度量失败不重复计数
    bool valid = false;
};

struct TextLayoutParams {
    std::string_view text;
    glm::vec2 boxSize{0.0f};       // 逻辑像素；x<=0 表示不限制宽度
    float pixelSize = 16.0f;       // 逻辑像素
    float contentScale = 1.0f;     // 物理像素 / 逻辑像素
    TextAlign align = TextAlign::Left;
    TextVerticalAlign verticalAlign = TextVerticalAlign::Top;
    TextWrap wrap = TextWrap::Word;
    float lineSpacing = 1.0f;
    bool pixelSnap = true;
    uint32_t tabSize = 4;
};

} // namespace eng

#endif // O_UI_TEXT_TYPES
