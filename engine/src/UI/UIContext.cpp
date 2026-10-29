#include "UI/UIContext.h"
#include "Renderer/RenderQueue.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace eng {
namespace {
struct TraversalGuard {
    bool& flag;
    explicit TraversalGuard(bool& value) : flag(value) { flag = true; }
    ~TraversalGuard() { flag = false; }
};
void RemoveFrom(std::vector<UIElementHandle>& handles, UIElementHandle handle) {
    handles.erase(std::remove(handles.begin(), handles.end(), handle), handles.end());
}
bool Contains(const UIRect& rect, glm::vec2 point) {
    return point.x >= rect.x && point.y >= rect.y &&
        point.x < rect.x + rect.width && point.y < rect.y + rect.height;
}
}

UIElementHandle UIContext::AddRoot(std::unique_ptr<UIElement> root) {
    if (!root) return {};
    auto handle = AcquireHandle();
    if (m_isTraversing)
        m_pendingCommands.push_back(AddRootElementCommand{std::move(root), handle});
    else AddRootElementImmediate(std::move(root), handle);
    return handle;
}

UIElementHandle UIContext::AttachElement(std::unique_ptr<UIElement> element,
    UIElementHandle parent) {
    if (!element || !IsReservedHandle(parent)) return {};
    auto handle = AcquireHandle();
    if (m_isTraversing)
        m_pendingCommands.push_back(AttachChildElementCommand{std::move(element), handle, parent});
    else AttachChildElementImmediate(std::move(element), handle, parent);
    return handle;
}

bool UIContext::Remove(UIElementHandle handle) {
    if (!IsReservedHandle(handle)) return false;
    if (m_isTraversing) {
        m_pendingCommands.push_back(RemoveElementCommand{handle});
        return true;
    }
    bool removed;
    {
        TraversalGuard guard(m_isTraversing);
        removed = RemoveElementImmediate(handle);
    }
    FlushPendingCommands();
    return removed;
}

UIElement* UIContext::Resolve(UIElementHandle handle) {
    return IsValidHandle(handle) ? m_slots[handle.index].element.get() : nullptr;
}
const UIElement* UIContext::Resolve(UIElementHandle handle) const {
    return IsValidHandle(handle) ? m_slots[handle.index].element.get() : nullptr;
}

void UIContext::SetView(const ScreenViewDesc& desc) {
    const auto& old = m_screenView;
    if (old == desc) return;
    m_screenView = desc;
    m_layoutDirty = true;
}

void UIContext::NotifyElementStateChanged(UIElementHandle handle, bool visibilityChanged) {
    auto notify = [&] {
        auto* element = Resolve(handle);
        if (element && visibilityChanged)
            element->OnVisibilityChanged(element->m_visibility == UIVisibility::Visible);
        ValidatePointerTargets();
    };
    if (m_isTraversing) { notify(); return; }
    {
        TraversalGuard guard(m_isTraversing);
        notify();
    }
    FlushPendingCommands();
}

bool UIContext::IsViewValid() const {
    const auto& view = m_screenView;
    return std::isfinite(view.logicalWidth) && view.logicalWidth > 0.0f &&
        std::isfinite(view.logicalHeight) && view.logicalHeight > 0.0f &&
        view.viewport.x >= 0 && view.viewport.y >= 0 &&
        view.viewport.width > 0 && view.viewport.height > 0 &&
        (view.origin == ScreenOrigin::TopLeft || view.origin == ScreenOrigin::BottomLeft);
}

void UIContext::Layout() {
    if (m_isTraversing || !IsViewValid()) return;
    FlushPendingCommands();
    {
        TraversalGuard guard(m_isTraversing);
        const bool dirty = m_layoutDirty;
        m_layoutDirty = false;
        const UIRect rootRect{0, 0, m_screenView.logicalWidth, m_screenView.logicalHeight};
        for (auto handle : m_roots) Layout(handle, rootRect, dirty);
    }
    FlushPendingCommands();
}

void UIContext::Layout(UIElementHandle handle, const UIRect& parentRect, bool dirty) {
    auto* element = Resolve(handle);
    if (!element) return;
    dirty = dirty || element->m_layoutDirty;
    if (dirty) {
        const auto& local = element->m_localRect;
        element->m_arrangedRect = {parentRect.x + local.x, parentRect.y + local.y,
            local.width, local.height};
        if (element->m_visibility == UIVisibility::Collapsed)
            element->m_arrangedRect.width = element->m_arrangedRect.height = 0.0f;
        element->m_layoutDirty = false;
        element->OnLayout();
    }
    for (auto child : element->m_children) Layout(child, element->m_arrangedRect, dirty);
}

void UIContext::CollectVisible(UIElementHandle handle,
    std::vector<UIElementHandle>& result) const {
    const auto* element = Resolve(handle);
    if (!element || element->m_visibility != UIVisibility::Visible) return;
    result.push_back(handle);
    for (auto child : element->m_children) CollectVisible(child, result);
}

std::vector<UIElementHandle> UIContext::GetPaintOrder() const {
    std::vector<UIElementHandle> result;
    for (auto root : m_roots) CollectVisible(root, result);
    std::stable_sort(result.begin(), result.end(), [this](auto a, auto b) {
        return Resolve(a)->m_renderOrder < Resolve(b)->m_renderOrder;
    });
    return result;
}

bool UIContext::IsInteractive(UIElementHandle handle) const {
    auto* element = Resolve(handle);
    if (!element || !element->m_hitTestVisible) return false;
    while (element) {
        if (!element->m_enabled || element->m_visibility != UIVisibility::Visible) return false;
        handle = element->m_parent;
        if (handle.index == InvalidUIIndex) return true;
        element = Resolve(handle);
    }
    return false;
}

UIElementHandle UIContext::HitTest(const glm::vec2& position) const {
    if (!m_pointerInside) return {};
    const auto order = GetPaintOrder();
    for (auto it = order.rbegin(); it != order.rend(); ++it)
        if (IsInteractive(*it) && Contains(Resolve(*it)->m_arrangedRect, position)) return *it;
    return {};
}

void UIContext::Dispatch(UIElementHandle handle, UIPointerEventType type,
    Key button, uint8_t mods, glm::vec2 scroll) {
    auto* element = Resolve(handle);
    if (!element) return;
    UIPointerEvent event;
    event.type = type;
    event.button = button;
    event.mods = mods;
    event.scrollOffset = scroll;
    event.viewPosition = m_pointerPosition;
    event.position = m_pointerPosition - glm::vec2(element->m_arrangedRect.x, element->m_arrangedRect.y);
    element->OnPointerEvent(event);
}

void UIContext::ValidatePointerTargets() {
    if (m_captured.index != InvalidUIIndex && !IsInteractive(m_captured)) {
        auto previous = m_captured;
        m_captured = {};
        Dispatch(previous, UIPointerEventType::Cancel, Key::MouseLeft);
    }
    if (m_hovered.index != InvalidUIIndex && !IsInteractive(m_hovered)) {
        auto previous = m_hovered;
        m_hovered = {};
        Dispatch(previous, UIPointerEventType::Leave);
    }
}

void UIContext::UpdateHover() {
    const auto next = HitTest(m_pointerPosition);
    if (next == m_hovered) return;
    const auto previous = m_hovered;
    m_hovered = {};
    Dispatch(previous, UIPointerEventType::Leave);
    if (!m_hasPointer) return;
    m_hovered = HitTest(m_pointerPosition);
    Dispatch(m_hovered, UIPointerEventType::Enter);
}

bool UIContext::MapPointer(glm::vec2 position, const WindowMetrics& metrics) {
    m_pointerInside = false;
    if (!IsViewValid() || !std::isfinite(metrics.logicalWidth) ||
        !std::isfinite(metrics.logicalHeight) || metrics.logicalWidth <= 0 ||
        metrics.logicalHeight <= 0 || metrics.framebufferWidth <= 0 ||
        metrics.framebufferHeight <= 0 || !std::isfinite(position.x) ||
        !std::isfinite(position.y)) return false;
    const auto& vp = m_screenView.viewport;
    if (static_cast<int64_t>(vp.x) + vp.width > metrics.framebufferWidth ||
        static_cast<int64_t>(vp.y) + vp.height > metrics.framebufferHeight) return false;
    const float x = position.x * metrics.framebufferWidth / metrics.logicalWidth - vp.x;
    const float y = position.y * metrics.framebufferHeight / metrics.logicalHeight -
        (metrics.framebufferHeight - vp.y - vp.height);
    const float localY = m_screenView.origin == ScreenOrigin::TopLeft ? y : vp.height - y;
    m_pointerInside = x >= 0 && localY >= 0 && x < vp.width && localY < vp.height;
    m_pointerPosition = {x * m_screenView.logicalWidth / vp.width,
        localY * m_screenView.logicalHeight / vp.height};
    return true;
}

UICapture UIContext::ProcessInput(const std::vector<InputEvent>& events,
    const WindowMetrics& metrics) {
    UICapture capture;
    if (m_isTraversing) return capture;
    Layout();
    if (!MapPointer(m_windowPointer, metrics)) { CancelPointerInput(); return capture; }
    // Apply callback mutations between events, not just once at the end of the frame.
    for (const auto& event : events) {
        if ((event.type == InputEventType::MouseMoved ||
             event.type == InputEventType::MouseButtonPressed ||
             event.type == InputEventType::MouseButtonReleased) &&
            (!std::isfinite(event.mouse.x) || !std::isfinite(event.mouse.y))) continue;
        {
            TraversalGuard guard(m_isTraversing);
            ValidatePointerTargets();
            if (event.type == InputEventType::MouseMoved ||
                event.type == InputEventType::MouseButtonPressed ||
                event.type == InputEventType::MouseButtonReleased) {
                m_windowPointer = event.mouse;
                m_hasPointer = true;
                MapPointer(event.mouse, metrics);
            }
            if (m_hasPointer) UpdateHover();
            ValidatePointerTargets();
            capture.pointer = capture.pointer || IsValidHandle(m_hovered) || IsValidHandle(m_captured);
            if (event.type == InputEventType::MouseButtonPressed && event.key == Key::MouseLeft) {
                if (!IsValidHandle(m_captured) && IsInteractive(m_hovered)) {
                    m_captured = m_hovered;
                    Dispatch(m_captured, UIPointerEventType::Down, event.key, event.mods);
                }
            } else if (event.type == InputEventType::MouseButtonReleased && event.key == Key::MouseLeft) {
                auto target = m_captured;
                m_captured = {};
                Dispatch(target, UIPointerEventType::Up, event.key, event.mods);
            } else if (event.type == InputEventType::MouseMoved) {
                Dispatch(IsValidHandle(m_captured) ? m_captured : m_hovered,
                    UIPointerEventType::Move, Key::Unknown, event.mods);
            } else if (event.type == InputEventType::Scroll) {
                Dispatch(m_hovered, UIPointerEventType::Scroll, Key::Unknown, event.mods, event.scrollOffset);
            }
            ValidatePointerTargets();
        }
        FlushPendingCommands();
        Layout();
    }
    {
        TraversalGuard guard(m_isTraversing);
        ValidatePointerTargets();
        if (m_hasPointer) UpdateHover();
        capture.pointer = capture.pointer || IsValidHandle(m_hovered) || IsValidHandle(m_captured);
    }
    FlushPendingCommands();
    return capture; // Keyboard focus/navigation is not part of the initial pointer API.
}

void UIContext::CancelPointerInput() {
    auto cancel = [&] {
        auto captured = m_captured;
        auto hovered = m_hovered;
        m_captured = m_hovered = {};
        m_hasPointer = m_pointerInside = false;
        Dispatch(captured, UIPointerEventType::Cancel, Key::MouseLeft);
        Dispatch(hovered, UIPointerEventType::Leave);
    };
    if (m_isTraversing) { cancel(); return; }
    {
        TraversalGuard guard(m_isTraversing);
        cancel();
    }
    FlushPendingCommands();
}

void UIContext::Submit(RenderQueue& queue) {
    if (m_isTraversing || !IsViewValid()) return;
    Layout();
    m_drawList.clear();
    {
        TraversalGuard guard(m_isTraversing);
        for (auto handle : GetPaintOrder()) {
            auto* element = Resolve(handle);
            const auto begin = m_drawList.size();
            const auto order = element->m_renderOrder;
            element->OnSubmit(m_drawList);
            for (auto i = begin; i < m_drawList.size(); ++i)
                m_drawList[i].renderOrder = order;
        }
    }
    FlushPendingCommands();
    if (queue.BeginScreenView(m_screenView)) {
        for (const auto& command : m_drawList) queue.SubmitScreenQuad(command);
        queue.EndScreenView();
    }
    m_drawList.clear();
}

void UIContext::FlushPendingCommands() {
    if (m_isTraversing) return;
    TraversalGuard guard(m_isTraversing);
    while (!m_pendingCommands.empty()) {
        auto commands = std::move(m_pendingCommands);
        m_pendingCommands.clear();
        for (auto& command : commands) std::visit([this](auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, AddRootElementCommand>)
                AddRootElementImmediate(std::move(value.root), value.handle);
            else if constexpr (std::is_same_v<T, AttachChildElementCommand>)
                AttachChildElementImmediate(std::move(value.element), value.handle, value.parentHandle);
            else RemoveElementImmediate(value.handle);
        }, command);
    }
}

void UIContext::AddRootElementImmediate(std::unique_ptr<UIElement> root, UIElementHandle handle) {
    if (!root || !IsReservedHandle(handle)) { ReleaseHandle(handle); return; }
    root->m_handle = handle;
    root->m_context = this;
    root->m_parent = {};
    m_slots[handle.index].element = std::move(root);
    m_roots.push_back(handle);
    m_layoutDirty = true;
}

void UIContext::AttachChildElementImmediate(std::unique_ptr<UIElement> element,
    UIElementHandle handle, UIElementHandle parent) {
    if (!element || !IsReservedHandle(handle) || !IsValidHandle(parent)) {
        ReleaseHandle(handle);
        return;
    }
    element->m_handle = handle;
    element->m_context = this;
    element->m_parent = parent;
    m_slots[handle.index].element = std::move(element);
    Resolve(parent)->m_children.push_back(handle);
    m_layoutDirty = true;
}

bool UIContext::RemoveElementImmediate(UIElementHandle handle) {
    auto* element = Resolve(handle);
    if (!element) return false;
    const auto children = element->m_children; // Removing children mutates the original list.
    for (auto child : children) RemoveElementImmediate(child);
    if (m_captured == handle) {
        m_captured = {};
        Dispatch(handle, UIPointerEventType::Cancel, Key::MouseLeft);
    }
    if (m_hovered == handle) {
        m_hovered = {};
        Dispatch(handle, UIPointerEventType::Leave);
    }
    if (auto* parent = Resolve(element->m_parent)) RemoveFrom(parent->m_children, handle);
    else RemoveFrom(m_roots, handle);
    ReleaseHandle(handle);
    m_layoutDirty = true;
    return true;
}

bool UIContext::IsReservedHandle(UIElementHandle handle) const {
    return handle.index < m_slots.size() && m_slots[handle.index].reserved &&
        m_slots[handle.index].generation == handle.generation;
}
bool UIContext::IsValidHandle(UIElementHandle handle) const {
    return IsReservedHandle(handle) && m_slots[handle.index].element != nullptr;
}
UIElementHandle UIContext::AcquireHandle() {
    uint32_t index;
    if (m_freeSlots.empty()) {
        if (m_slots.size() >= InvalidUIIndex) throw std::length_error("UI handle capacity exhausted");
        index = static_cast<uint32_t>(m_slots.size());
        m_slots.emplace_back();
    } else {
        index = m_freeSlots.back();
        m_freeSlots.pop_back();
    }
    m_slots[index].reserved = true;
    return {index, m_slots[index].generation};
}
void UIContext::ReleaseHandle(UIElementHandle handle) {
    if (!IsReservedHandle(handle)) return;
    // The destructor may call back into this context, so hold no slot reference across it.
    auto element = std::move(m_slots[handle.index].element);
    if (element) element->m_context = nullptr;
    m_slots[handle.index].reserved = false;
    if (m_slots[handle.index].generation != std::numeric_limits<uint32_t>::max()) {
        ++m_slots[handle.index].generation;
        m_freeSlots.push_back(handle.index);
    }
}
} // namespace eng
