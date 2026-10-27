#include "Input/InputManager.h"
#include <iostream>

namespace eng {
    
    
void InputManager::SetKeyPressed(int key, bool pressed) {
    if (key < 0 || key >= static_cast<int>(m_keys.size())) {
        std::cout << "InputManager::SetKeyPressed: key index out of bound" << std::endl;
        return;
    }
    m_keys[key] = pressed;
}


bool InputManager::IsKeyPressed(int key) const {
    if (key < 0 || key >= static_cast<int>(m_keys.size())) {
        std::cout << "InputManager::IsKeyPressed: key index out of bound" << std::endl;
        return false;
    }        
    return m_keys[key];
}

bool InputManager::IsKeyPressed(Key key) const {
    return IsKeyPressed(static_cast<int>(key));
}

void InputManager::SetKeyWasPressed(int key) {
    if (key < 0 || key >= static_cast<int>(m_keys.size())) {
        std::cout << "InputManager::SetKeyWasPressed: key index out of bound" << std::endl;
        return;
    }  
    m_pendingEvents.push_back(InputEvent{
        .type = InputEventType::KeyPressed,
        .key = static_cast<Key>(key)
    });
    m_keyPressed[key] = true;
}

bool InputManager::WasKeyPressed(int key) {
    if (key < 0 || key >= static_cast<int>(m_keys.size())) {
        std::cout << "InputManager::WasKeyPressed: key index out of bound" << std::endl;
        return false;
    }  
    return m_keyPressed[key];
}

bool InputManager::WasKeyPressed(Key key) {
    return WasKeyPressed(static_cast<int>(key));
}

void InputManager::SetKeyWasReleased(int key) {
    if (key < 0 || key >= static_cast<int>(m_keys.size())) {
        std::cout << "InputManager::SetKeyWasReleased: key index out of bound" << std::endl;
        return;
    }  
    m_pendingEvents.push_back(InputEvent{
        .type = InputEventType::KeyReleased,
        .key = static_cast<Key>(key)
    });
    m_keyReleased[key] = true;
}

bool InputManager::WasKeyReleased(int key) {
    if (key < 0 || key >= static_cast<int>(m_keys.size())) {
        std::cout << "InputManager::WasKeyReleased: key index out of bound" << std::endl;
        return false;
    }  
    return m_keyReleased[key];
}

bool InputManager::WasKeyReleased(Key key) {
    return WasKeyReleased(static_cast<int>(key));
}

void InputManager::SetMouseButtonPressed(int key, bool pressed) {
    if (key < 0 || key >= static_cast<int>(m_mouseKeys.size())) {
        std::cout << "InputManager::SetMouseButtonPressed: key index out of bound" << std::endl;
        return;
    }
    m_mouseKeys[key] = pressed;
}

bool InputManager::IsMouseButtonPressed(int key) const {
    if (key < 0 || key >= static_cast<int>(m_mouseKeys.size())) {
        std::cout << "InputManager::IsMouseButtonPressed: key index out of bound" << std::endl;
        return false;
    }
    return m_mouseKeys[key];
}

bool InputManager::IsMouseButtonPressed(Key key) const {
    return IsMouseButtonPressed(static_cast<int>(key));
}

void InputManager::SetMouseButtonWasPressed(int key) {
    if (key < 0 || key >= static_cast<int>(m_mouseKeys.size())) {
        std::cout << "InputManager::SetMouseButtonWasPressed: "
            "key index out of bound" << std::endl;
        return;
    }
    m_pendingEvents.push_back(InputEvent{
        .type = InputEventType::MouseButtonPressed,
        .key = static_cast<Key>(key),
        .mouse = m_mousePositionCurrent
    });
    m_mouseKeyPressed[key] = true;
}

bool InputManager::WasMouseButtonPressed(int key) {
    if (key < 0 || key >= static_cast<int>(m_mouseKeys.size())) {
        std::cout << "InputManager::WasMouseButtonPressed: key index out of bound" << std::endl;
        return false;
    }
    return m_mouseKeyPressed[key];
}

bool InputManager::WasMouseButtonPressed(Key key) {
    return WasMouseButtonPressed(static_cast<int>(key));
}

void InputManager::SetMouseButtonWasReleased(int key) {
    if (key < 0 || key >= static_cast<int>(m_mouseKeys.size())) {
        std::cout << "InputManager::SetMouseButtonWasReleased: "
            "key index out of bound" << std::endl;
        return;
    }
    m_pendingEvents.push_back(InputEvent{
        .type = InputEventType::MouseButtonReleased,
        .key = static_cast<Key>(key),
        .mouse = m_mousePositionCurrent
    });
    m_mouseKeyReleased[key] = true;
}

bool InputManager::WasMouseButtonReleased(int key) {
    if (key < 0 || key >= static_cast<int>(m_mouseKeys.size())) {
        std::cout << "InputManager::WasMouseButtonReleased: "
            "key index out of bound" << std::endl;
        return false;
    }
    return m_mouseKeyReleased[key];
}

bool InputManager::WasMouseButtonReleased(Key key) {
    return WasMouseButtonReleased(static_cast<int>(key));
}

const glm::vec2& InputManager::GetMousePositionOld() const {
    return m_mousePositionOld;
}

void InputManager::SetMousePositionOld(const glm::vec2& pos) {
    m_mousePositionOld = pos;
}

const glm::vec2& InputManager::GetMousePositionCurrent() const {
    return m_mousePositionCurrent;
}

void InputManager::RenewMousePosition(const glm::vec2& newPos) {
    if (m_firstMouse) {
        m_firstMouse = false;
        m_mousePositionOld = newPos;
        m_mousePositionCurrent = newPos;
        return;
    }
    m_mousePositionCurrent = newPos;

    m_pendingEvents.push_back(InputEvent{
        .type = InputEventType::MouseMoved,
        .mouse = newPos
    });
}

const glm::vec2& InputManager::GetMouseScrollOffset() const {
    return m_mouseScrollOffset;
}

void InputManager::SetMouseScrollOffset(const glm::vec2& offset) {
    m_mouseScrollOffset += offset;

    m_pendingEvents.push_back(InputEvent{
        .type = InputEventType::Scroll,
        .scrollOffset = offset
    });
}

glm::vec2 InputManager::GetMousePositionDelta() const {
    return m_mousePositionCurrent - m_mousePositionOld;
}

const std::vector<InputEvent>& 
    InputManager::GetEvents() const {
    return m_events;
}

void InputManager::BeginFrame() {
    m_events.swap(m_pendingEvents);
    m_pendingEvents.clear();
}

void InputManager::ClearState() {
    m_mousePositionOld = m_mousePositionCurrent;
    m_mouseScrollOffset = glm::vec2{0.0f};
    for (auto& key : m_mouseKeyPressed) {
        key = false;
    }
    for (auto& key : m_mouseKeyReleased) {
        key = false;
    }
    for (auto& key : m_keyPressed) {
        key = false;
    }
    for (auto& key : m_keyReleased) {
        key = false;
    }    
    m_events.clear();
}


}
