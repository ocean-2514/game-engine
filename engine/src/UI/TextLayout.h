#ifndef O_TEXT_LAYOUT
#define O_TEXT_LAYOUT

#include "UI/TextTypes.h"

namespace eng {

class TextLayout {
public:
    // 纯 CPU；不创建纹理、不访问 RenderDevice。失败时 result.valid=false。
    // 非法字号/scale/行距使用安全默认值；栅格字号 round 后限制为 1..256。
    // Word 是空格/简化 CJK 断行；超长词降级到逐字符。Tab 为固定数量空格。
    // CRLF/CR/LF 为硬换行；末尾硬换行不额外追加空行。仅自动换行跳过行首空白。
    // 无 box 宽度时按内容最大行宽对齐；无 box 高度时不做垂直偏移。
    // measuredSize 是 advance/行盒尺寸（不包含字形 bearing 越界的墨迹包围盒）。
    static TextLayoutResult Build(
        const TextLayoutParams& params,
        GlyphProvider& font);
};

} // namespace eng

#endif // O_TEXT_LAYOUT
