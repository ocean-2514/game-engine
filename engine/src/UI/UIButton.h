#ifndef O_UI_BUTTON
#define O_UI_BUTTON

#include "UI/UIImage.h"
#include "UI/TextPainter.h"
#include <functional>
#include <vector>

namespace eng
{

class UIButton : public UIImage
{
public:
    UIButton() = default;

    // Copy appearance only; no dependency on the source UI node's lifetime/layout.
    void SetBackground(const UIImage* image);
    void SetBackGround(const UIImage* image) { SetBackground(image); }
    void AddClickListener(std::function<void()> listener);
    void ClearClickListeners();

    void SetLabel(std::shared_ptr<GlyphRenderSource> source, 
        std::string text);
    void SetLabelStyle(const TextStyle& style);
    void ClearLabel();

protected:
    void OnPointerEvent(const UIPointerEvent& event) override;
    void OnLayout() override;
    void OnSubmit(UIDrawList& drawList) const override;

private:
    bool m_pressed = false;
    TextPainter m_label;

    // Shared entries keep callable state and lifetime stable across a notification snapshot.
    std::vector<std::shared_ptr<std::function<void()>>> m_clickListeners;
};



} // namespace eng


#endif // O_UI_BUTTON
