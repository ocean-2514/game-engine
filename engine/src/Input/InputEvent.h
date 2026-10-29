#ifndef O_INPUT_EVENT
#define O_INPUT_EVENT

#include "Input/Key.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace eng
{
    
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


} // namespace eng


#endif // O_INPUT_EVENT