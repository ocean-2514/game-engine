#ifndef O_UI_TYPE
#define O_UI_TYPE

#include "Renderer/RenderCommand.h"
#include "Input/Key.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace eng
{

using UIDrawList = std::vector<ScreenQuadCommand>;

inline constexpr uint32_t InvalidUIIndex = 
    std::numeric_limits<uint32_t>::max();

struct UIElementHandle {
    uint32_t index = InvalidUIIndex;
    uint32_t generation = 0;

    bool operator==(const UIElementHandle& other) const {
        return index == other.index 
            && generation == other.generation;
    }
};
    
struct UIRect { 
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f; 
};

struct UICapture { 
    bool pointer = false; 
    bool keyboard = false; 
};

enum class UIVisibility { 
    Visible, Hidden, Collapsed 
};

enum class UIPointerEventType : uint8_t {
    Enter,       
    Leave,      
    Down,       
    Up,         
    Move,       
    Cancel,
    Scroll
};

struct UIPointerEvent {
    glm::vec2 scrollOffset{0.0f};
    UIPointerEventType type = UIPointerEventType::Move;
    Key button = Key::Unknown;      // Down/Up 有效：GLFW_MOUSE_BUTTON_*，初版只用左键
    glm::vec2 position;             // 节点局部坐标（相对 arrangedRect 左上角）
    glm::vec2 viewPosition;         // view 局部逻辑坐标（相对 UI view 左上角）
    uint8_t mods = 0;               // shift/ctrl/alt 快照，按钮组合键可能用
};


} // namespace eng


#endif // O_UI_TYPE
