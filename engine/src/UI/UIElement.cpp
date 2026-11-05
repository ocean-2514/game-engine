#include "UI/UIElement.h"
#include "UI/UIContext.h"
#include <cmath>

namespace eng
{
    
UIElementHandle UIElement::GetHandle() const {
    return m_handle;
}

UIElementHandle UIElement::GetParent() const {
    return m_parent;
}

void UIElement::SetRect(const UIRect& localRect) {
    if (!std::isfinite(localRect.x) || !std::isfinite(localRect.y) ||
        !std::isfinite(localRect.width) || !std::isfinite(localRect.height) ||
        localRect.width < 0.0f || localRect.height < 0.0f) return;
    if (m_localRect.x == localRect.x && m_localRect.y == localRect.y &&
        m_localRect.width == localRect.width && m_localRect.height == localRect.height) return;
    m_localRect = localRect;
    m_layoutDirty = true;
}

UIRect UIElement::GetLocalRect() const {
    return m_localRect;
}

UIRect UIElement::GetArrangedRect() const {
    return m_arrangedRect;
}

void UIElement::SetVisibility(UIVisibility visibility) {
    if (visibility != UIVisibility::Visible && visibility != UIVisibility::Hidden &&
        visibility != UIVisibility::Collapsed) return;
    if (m_visibility == visibility) return;
    m_visibility = visibility;
    m_layoutDirty = true;
    if (m_context) m_context->NotifyElementStateChanged(m_handle, true);
    else OnVisibilityChanged(visibility == UIVisibility::Visible);
}

UIVisibility UIElement::GetVisibility() const {
    return m_visibility;
}

void UIElement::SetEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    if (m_context) m_context->NotifyElementStateChanged(m_handle, false);
}

bool UIElement::IsEnabled() const {
    return m_enabled;
}

void UIElement::SetRenderOrder(int32_t order) {
    m_renderOrder = order;
}

void UIElement::SetHitTestVisible(bool visible) {
    if (m_hitTestVisible == visible) return;
    m_hitTestVisible = visible;
    if (m_context) m_context->NotifyElementStateChanged(m_handle, false);
}

bool UIElement::IsHitTestVisible() const { return m_hitTestVisible; }

int32_t UIElement::GetRenderOrder() const {
    return m_renderOrder;
}

glm::mat4 UIElement::GetModelMatrix() const {
    return MakeUIRectMatrix(m_arrangedRect);
}

void UIElement::InvalidateLayout() { m_layoutDirty = true; }
const UIContext* UIElement::GetUIContext() const { return m_context; }

bool UIElement::InsideRect(const glm::vec2& viewPosition) const {
    return viewPosition.x >= m_arrangedRect.x
        && viewPosition.x < m_arrangedRect.x + m_arrangedRect.width
        && viewPosition.y >= m_arrangedRect.y
        && viewPosition.y < m_arrangedRect.y + m_arrangedRect.height;
}
    
} // namespace eng
