#ifndef O_INPUT_MANAGER
#define O_INPUT_MANAGER

#include "Input/Key.h"

#include <cstdint>
#include <array>
#include <vector>
#include <glm/glm.hpp>

namespace eng {

enum class InputEventType : uint8_t {
    KeyPressed, KeyReleased,
    MouseButtonPressed, MouseButtonReleased,
    MouseMoved,
    Scroll
};

struct InputEvent {
    InputEventType type = InputEventType::MouseMoved;
    // mouse button / key
    Key key = Key::Unknown;
    glm::vec2 mouse{0.0f};
    glm::vec2 scrollOffset{0.0f};
    uint8_t mods = 0;
};
    
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
    void SetKeyWasPressed(int key);
    bool WasKeyPressed(int key);
    bool WasKeyPressed(Key key);
    void SetKeyWasReleased(int key);
    bool WasKeyReleased(int key);
    bool WasKeyReleased(Key key);
    void SetMouseButtonPressed(int key, bool pressed);
    bool IsMouseButtonPressed(int key) const;
    bool IsMouseButtonPressed(Key key) const;
    void SetMouseButtonWasPressed(int key);
    bool WasMouseButtonPressed(int key);
    bool WasMouseButtonPressed(Key key);
    void SetMouseButtonWasReleased(int key);
    bool WasMouseButtonReleased(int key);
    bool WasMouseButtonReleased(Key key);

    const glm::vec2& GetMousePositionOld() const;
    void SetMousePositionOld(const glm::vec2& pos);
    const glm::vec2& GetMousePositionCurrent() const;
    void RenewMousePosition(const glm::vec2& newPos);
    const glm::vec2& GetMouseScrollOffset() const;
    void SetMouseScrollOffset(const glm::vec2& offset);
    glm::vec2 GetMousePositionDelta() const;
    const std::vector<InputEvent>& GetEvents() const;

    // Call after Window::PollEvents; makes callback events visible this frame.
    void BeginFrame();
    void ClearState();

private:
    std::array<bool, 512> m_keys = { false };
    std::array<bool, 512> m_keyPressed = { false };
    std::array<bool, 512> m_keyReleased = { false };
    std::array<bool, 16> m_mouseKeys = { false };
    std::array<bool, 16> m_mouseKeyPressed = { false };
    std::array<bool, 16> m_mouseKeyReleased = { false };

    std::vector<InputEvent> m_events;
    std::vector<InputEvent> m_pendingEvents;

    glm::vec2 m_mousePositionOld{0.0f};
    glm::vec2 m_mousePositionCurrent{0.0f};
    glm::vec2 m_mouseScrollOffset{0.0f};

    bool m_firstMouse = true;

    friend class Engine;
};



}

#endif
