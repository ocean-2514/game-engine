#ifndef O_UI_TEXT
#define O_UI_TEXT

#include "UI/UIElement.h"
#include "Assets/Font/FontDesc.h"

namespace eng {

class UIText : public UIElement {
public:
    UIText();  // 实现中调用 SetHitTestVisible(false)
    explicit UIText(std::shared_ptr<GlyphRenderSource> source);

    void SetSource(std::shared_ptr<GlyphRenderSource> source);
    const std::shared_ptr<GlyphRenderSource>& GetSource() const;

    void SetText(std::string text);
    const std::string& GetText() const;

    void SetColor(const glm::vec4& color);
    void SetPixelSize(float pixelSize);

protected:
    void OnLayout() override;
    void OnSubmit(UIDrawList& drawList) const override;

private:

};



}

#endif // O_UI_TEXT