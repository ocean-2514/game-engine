#ifndef O_UI_ELEMENT
#define O_UI_ELEMENT

#include "UI/UITypes.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace eng
{

class UIContext;
    
class UIElement {
public:
    UIElement() = default;
    virtual ~UIElement() = default;
    UIElement(const UIElement&) = delete;
    UIElement& operator=(const UIElement&) = delete;

    UIElementHandle GetHandle() const;
    UIElementHandle GetParent() const;
    void SetRect(const UIRect& localRect);      
    UIRect GetLocalRect() const;      
    UIRect GetArrangedRect() const;      
    void SetVisibility(UIVisibility visibility);
    UIVisibility GetVisibility() const;
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    void SetHitTestVisible(bool visible);
    bool IsHitTestVisible() const;
    void SetRenderOrder(int32_t order);
    int32_t GetRenderOrder() const;

protected:
    virtual void OnPointerEvent(const UIPointerEvent& event) {}
    virtual void OnSubmit(UIDrawList& drawList) const {}
    virtual void OnLayout() {}              
    virtual void OnVisibilityChanged(bool visible) {}

    glm::mat4 GetModelMatrix() const;
    bool InsideRect(const glm::vec2& viewPosition) const;

private:
    UIContext* m_context = nullptr; // Non-owning; assigned only after mounting.
    UIRect m_localRect{0.0f, 0.0f, 100.0f, 100.0f};
    UIRect m_arrangedRect;
    bool m_layoutDirty = true;

    UIElementHandle m_handle;
    UIElementHandle m_parent;
    std::vector<UIElementHandle> m_children;

    UIVisibility m_visibility = UIVisibility::Visible;
    bool m_enabled = true;
    bool m_hitTestVisible = true;
    int32_t m_renderOrder = 0;

    friend class UIContext;
};




} // namespace eng


#endif // O_UI_ELEMENT
