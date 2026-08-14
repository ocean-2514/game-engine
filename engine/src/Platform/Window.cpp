#include "Platform/Window.h"
#include "Platform/GLFW/GLFWWindow.h"

namespace eng {

std::unique_ptr<Window> Window::Create(const WindowDesc& desc) {
    auto window = std::make_unique<GLFWWindow>();
    return window->Init(desc) ? std::move(window) : nullptr;
}

float Window::GetWidth() const {
    return m_width;
}

float Window::GetHeight() const {
    return m_height;
}


} // namespace eng
