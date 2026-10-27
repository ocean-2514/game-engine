#include "Platform/Window.h"
#include "Platform/GLFW/GLFWWindow.h"

namespace eng {

std::unique_ptr<Window> Window::Create(const WindowDesc& desc) {
    auto window = std::make_unique<GLFWWindow>();
    return window->Init(desc) ? std::move(window) : nullptr;
}

int Window::GetWindowWidth() const {
    return m_windowWidth;
}

int Window::GetWindowHeight() const {
    return m_windowHeight;
}

int Window::GetFrameBufferWidth() const {
    return m_frameBufferWidth;
}

int Window::GetFrameBufferHeight() const {
    return m_frameBufferHeight;
}


} // namespace eng
