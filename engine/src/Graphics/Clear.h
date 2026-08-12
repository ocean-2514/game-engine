#ifndef O_CLEAR
#define O_CLEAR

#include <cstdint>

namespace eng {

enum class ClearBuffer : uint8_t {
    None = 0,
    Color = 1 << 0,
    Depth = 1 << 1,
    Stencil = 1 << 2
};

constexpr ClearBuffer operator|(ClearBuffer lhs, ClearBuffer rhs) {
    return static_cast<ClearBuffer>(
        static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

constexpr ClearBuffer operator&(ClearBuffer lhs, ClearBuffer rhs) {
    return static_cast<ClearBuffer>(
        static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

constexpr bool HasClearBuffer(ClearBuffer buffers, ClearBuffer buffer) {
    return (buffers & buffer) != ClearBuffer::None;
}

struct ClearColor {
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    float alpha = 1.0f;
};

struct ClearDesc {
    ClearBuffer buffers = ClearBuffer::Color;
    ClearColor color;
    float depth = 1.0f;
    int32_t stencil = 0;
};

} // namespace eng

#endif
