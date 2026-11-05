#ifndef O_UI_TEXT
#define O_UI_TEXT

#include "UI/UIElement.h"
#include "UI/TextPainter.h"

#include <memory>
#include <string>

namespace eng {

class UIText : public UIElement {
public:
    UIText();  // 实现中调用 SetHitTestVisible(false)
    explicit UIText(std::shared_ptr<GlyphRenderSource> source);

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

    // Cached results from the last UIContext::Layout/Submit; setters invalidate layout.
    glm::vec2 GetMeasuredSize() const;
    const TextLayoutResult& GetLayout() const;

protected:
    void OnLayout() override;
    void OnSubmit(UIDrawList& drawList) const override;

private:
    TextPainter m_painter;
};

} // namespace eng

#endif // O_UI_TEXT
