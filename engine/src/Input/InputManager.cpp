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


bool InputManager::IsKeyPressed(int key) {
    if (key < 0 || key >= static_cast<int>(m_keys.size())) {
        std::cout << "InputManager::IsKeyPressed: key index out of bound" << std::endl;
        return false;
    }        
    return m_keys[key];
}

bool InputManager::IsKeyPressed(Key key) {
    return IsKeyPressed(static_cast<int>(key));
}

}
