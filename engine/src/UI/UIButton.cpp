#include "UI/UIButton.h"
#include <utility>

namespace eng {

void UIButton::SetBackground(const UIImage* image) {
    SetTexture(image ? image->GetTexture() : nullptr);
    SetUV(image ? image->GetLowerLeftUV() : glm::vec2{0.0f},
        image ? image->GetUpperRightUV() : glm::vec2{1.0f});
    SetTint(image ? image->GetTint() : glm::vec4{1.0f});
}

void UIButton::AddClickListener(std::function<void()> listener) {
    if (listener)
        m_clickListeners.push_back(std::make_shared<std::function<void()>>(std::move(listener)));
}

void UIButton::ClearClickListeners() { m_clickListeners.clear(); }

void UIButton::OnPointerEvent(const UIPointerEvent& event) {
    // Cancel must work even after disabling/hiding this control.
    if (event.type == UIPointerEventType::Cancel) {
        m_pressed = false;
        return;
    }
    if (event.button != Key::MouseLeft) return;
    if (event.type == UIPointerEventType::Down) {
        m_pressed = IsEnabled() && event.insideView && InsideRect(event.viewPosition);
    } else if (event.type == UIPointerEventType::Up) {
        const bool clicked = m_pressed && IsEnabled() && event.insideView && InsideRect(event.viewPosition);
        m_pressed = false;
        if (!clicked) return;
        // Add/Clear during notification affects the next click, not this iteration.
        const auto listeners = m_clickListeners;
        for (const auto& listener : listeners) (*listener)();
    }
}

} // namespace eng
