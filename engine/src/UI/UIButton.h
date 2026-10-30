#ifndef O_UI_BUTTON
#define O_UI_BUTTON

#include "UI/UIImage.h"
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

protected:
    void OnPointerEvent(const UIPointerEvent& event) override;

private:
    bool m_pressed = false;

    // Shared entries keep callable state and lifetime stable across a notification snapshot.
    std::vector<std::shared_ptr<std::function<void()>>> m_clickListeners;
};



} // namespace eng


#endif // O_UI_BUTTON
