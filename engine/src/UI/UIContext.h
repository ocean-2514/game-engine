#ifndef O_UI_CONTEXT
#define O_UI_CONTEXT

#include "UI/UITypes.h"
#include "UI/UIElement.h"
#include "Input/InputEvent.h"
#include "Renderer/RenderCommand.h"

#include <memory>
#include <vector>
#include <variant>

namespace eng
{
    
class RenderQueue;

class UIContext {
public:
    UIContext() = default;
    UIContext(const UIContext&) = delete;
    UIContext& operator=(const UIContext&) = delete;
    UIContext(UIContext&&) = delete;
    UIContext& operator=(UIContext&&) = delete;
    // Handles belong to this context only. During callbacks additions are staged;
    // Resolve returns nullptr until the command is applied at the traversal boundary.
    UIElementHandle AddRoot(std::unique_ptr<UIElement> root);
    UIElementHandle AttachElement(
        std::unique_ptr<UIElement> element, 
        UIElementHandle parentHandle);
    bool Remove(UIElementHandle handle);            
    UIElement* Resolve(UIElementHandle handle); // Borrowed; invalid after removal.
    const UIElement* Resolve(UIElementHandle handle) const;
    // Call when the window loses focus or the application deactivates this UI.
    void CancelPointerInput();
    void SetView(const ScreenViewDesc& desc);
    void Layout();                        
    UICapture ProcessInput(
        const std::vector<InputEvent>& events,
        const WindowMetrics& metrics);
    void Submit(RenderQueue& queue);

private:
    struct UISlot {
        std::unique_ptr<UIElement> element;
        uint32_t generation = 0;
        bool reserved = false;
    };

    struct AddRootElementCommand {
        std::unique_ptr<UIElement> root;
        UIElementHandle handle;
    };
    struct AttachChildElementCommand {
        std::unique_ptr<UIElement> element;
        UIElementHandle handle;
        UIElementHandle parentHandle;
    };
    struct RemoveElementCommand {
        UIElementHandle handle;
    };

    using UICommand = std::variant<AddRootElementCommand,
        AttachChildElementCommand, RemoveElementCommand>;

    void FlushPendingCommands();
    void NotifyElementStateChanged(UIElementHandle handle, bool visibilityChanged);
    friend class UIElement;
    void AddRootElementImmediate(
        std::unique_ptr<UIElement> root, 
        UIElementHandle handle);
    void AttachChildElementImmediate(
        std::unique_ptr<UIElement> element,
        UIElementHandle handle, 
        UIElementHandle parent);
    bool RemoveElementImmediate(UIElementHandle handle);
    void Layout(UIElementHandle handle, 
        const UIRect& parentRect, bool dirty);

    bool IsValidHandle(UIElementHandle handle) const;
    bool IsReservedHandle(UIElementHandle handle) const;
    UIElementHandle AcquireHandle();
    void ReleaseHandle(UIElementHandle handle);
    void CollectVisible(UIElementHandle handle, std::vector<UIElementHandle>& result) const;
    std::vector<UIElementHandle> GetPaintOrder() const;
    bool IsInteractive(UIElementHandle handle) const;
    UIElementHandle HitTest(const glm::vec2& position) const;
    void Dispatch(UIElementHandle handle, UIPointerEventType type,
        Key button = Key::Unknown, uint8_t mods = 0,
        glm::vec2 scroll = glm::vec2{0.0f});
    void UpdateHover();
    void ValidatePointerTargets();
    bool IsViewValid() const;
    bool MapPointer(glm::vec2 position, const WindowMetrics& metrics);
    
    std::vector<UISlot> m_slots;
    std::vector<uint32_t> m_freeSlots;
    std::vector<UIElementHandle> m_roots;

    ScreenViewDesc m_screenView;
    bool m_layoutDirty = true;

    bool m_isTraversing = false;
    std::vector<UICommand> m_pendingCommands;

    UIDrawList m_drawList;
    UIElementHandle m_hovered;
    UIElementHandle m_captured;
    glm::vec2 m_pointerPosition{0.0f};
    glm::vec2 m_windowPointer{0.0f};
    bool m_hasPointer = false;
    bool m_pointerInside = false;
};




} // namespace eng


#endif // O_UI_CONTEXT
