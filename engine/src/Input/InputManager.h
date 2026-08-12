#ifndef O_INPUT_MANAGER
#define O_INPUT_MANAGER

#include <array>
#include "Input/Key.h"

namespace eng {
    
class InputManager
{
private:
    InputManager() = default;
    InputManager(const InputManager& other) = delete;
    InputManager(InputManager&& other) = delete;
    InputManager& operator=(const InputManager& other) = delete;
    InputManager& operator=(InputManager&& other) = delete;

public:
    void SetKeyPressed(int key, bool pressed);
    bool IsKeyPressed(int key);
    bool IsKeyPressed(Key key);

private:
    std::array<bool, 512> m_keys = { false };

    friend class Engine;
};



}

#endif
