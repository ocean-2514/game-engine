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

const glm::vec2& InputManager::GetMousePositionOld() const {
    return m_mousePositionOld;
}

void InputManager::SetMousePositionOld(const glm::vec2& pos) {
    m_mousePositionOld = pos;
}

const glm::vec2& InputManager::GetMousePositionCurrent() const {
    return m_mousePositionCurrent;
}

void InputManager::SetMousePositionCurrent(const glm::vec2& pos) {
    if (m_firstMouse) {
        m_firstMouse = false;
        m_mousePositionOld = pos;
    }
    m_mousePositionCurrent = pos;
}

void InputManager::RenewMousePosition(const glm::vec2& newPos) {
    if (m_firstMouse) {
        m_firstMouse = false;
        m_mousePositionOld = newPos;
        m_mousePositionCurrent = newPos;
        return;
    }

    m_mousePositionCurrent = newPos;
}

const glm::vec2& InputManager::GetMouseScrollOffset() const {
    return m_mouseScrollOffset;
}

void InputManager::SetMouseScrollOffset(const glm::vec2& offset) {
    m_mouseScrollOffset += offset;
}

glm::vec2 InputManager::GetMousePositionDelta() const {
    return m_mousePositionCurrent - m_mousePositionOld;
}


void InputManager::RenewDataOnFrameEnd() {
    m_mousePositionOld = m_mousePositionCurrent;
    m_mouseScrollOffset = glm::vec2{0.0f};
}


}
