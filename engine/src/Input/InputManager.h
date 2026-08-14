#ifndef O_INPUT_MANAGER
#define O_INPUT_MANAGER

#include <array>
#include "Input/Key.h"
#include <glm/glm.hpp>

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
    bool IsKeyPressed(int key) const;
    bool IsKeyPressed(Key key) const;
    void SetMouseButtonPressed(int key, bool pressed);
    bool IsMouseButtonPressed(int key) const;
    bool IsMouseButtonPressed(Key key) const;

    const glm::vec2& GetMousePositionOld() const;
    void SetMousePositionOld(const glm::vec2& pos);
    const glm::vec2& GetMousePositionCurrent() const;
    void SetMousePositionCurrent(const glm::vec2& pos);
    void RenewMousePosition(const glm::vec2& newPos);
    const glm::vec2& GetMouseScrollOffset() const;
    void SetMouseScrollOffset(const glm::vec2& offset);
    glm::vec2 GetMousePositionDelta() const;

    void RenewDataOnFrameEnd();

private:
    std::array<bool, 512> m_keys = { false };
    std::array<bool, 16> m_mouseKeys = { false };
    glm::vec2 m_mousePositionOld{0.0f};
    glm::vec2 m_mousePositionCurrent{0.0f};
    glm::vec2 m_mouseScrollOffset{0.0f};

    bool m_firstMouse = true;

    friend class Engine;
};



}

#endif
