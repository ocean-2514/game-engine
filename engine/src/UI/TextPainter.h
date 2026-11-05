#ifndef O_TEXT_PAINTER
#define O_TEXT_PAINTER

#include "Assets/Font/FontDesc.h"
#include "UI/TextTypes.h"
#include "UI/UITypes.h"

#include <memory>
#include <string>

namespace eng {

class TextPainter {
public:
    TextPainter() = default;

    void SetSource(std::shared_ptr<GlyphRenderSource> source);
    const std::shared_ptr<GlyphRenderSource>& GetSource() const;
    void SetText(std::string text);
    const std::string& GetText() const;
    void SetStyle(const TextStyle& style);
    const TextStyle& GetStyle() const;
    void SetColor(const glm::vec4& color);
    void SetPixelSize(float pixelSize);
    void SetAlign(TextAlign align);
    void SetVerticalAlign(TextVerticalAlign align);
    void SetWrap(TextWrap wrap);

    void Invalidate(); // Invalidate cached layout only; preserve source, text and style.

    // 只在 text/style/box 宽高/scale 变化时重建；返回是否重建。
    // 只改变 box 的 x/y 时不会重排。
    bool UpdateLayout(const UIRect& box, float pixelScale);

    const TextLayoutResult& GetLayout() const;
    glm::vec2 GetMeasuredSize() const;

    // 生成字形 quad。要求之前已用相同 box 宽高/scale 调用过 UpdateLayout。
    // 脏布局/零面积不提交；TopLeft/BottomLeft 使用相同的图集 UV 约定。
    void AppendQuads(UIDrawList& drawList, const UIRect& box, int32_t renderOrder,
        ScreenOrigin origin = ScreenOrigin::TopLeft) const;

private:
    std::shared_ptr<GlyphRenderSource> m_source;
    std::string m_text;
    TextStyle m_style;
    TextLayoutResult m_layout;
    glm::vec2 m_lastBoxSize{0.0f};
    float m_lastPixelScale = 1.0f;
    bool m_dirty = true;
};

} // namespace eng


#endif // O_TEXT_PAINTER
