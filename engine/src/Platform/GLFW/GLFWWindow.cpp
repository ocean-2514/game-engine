#include "Platform/GLFW/GLFWWindow.h"

#include <iostream>
#include <utility>
#include <GLFW/glfw3.h>

namespace eng {

bool GLFWWindow::Init(const WindowDesc& desc) {
    if (glfwInit() != GLFW_TRUE) {
        std::cout << "GLFWWindow::Init: failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(desc.width, desc.height, desc.title.c_str(), nullptr, nullptr);
    if (m_window == nullptr) {
        std::cout << "GLFWWindow::Init: failed to create window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, OnKey);
    return true;
}

GLFWWindow::~GLFWWindow() {
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

bool GLFWWindow::ShouldClose() const {
    return glfwWindowShouldClose(m_window) == GLFW_TRUE;
}

void GLFWWindow::PollEvents() {
    glfwPollEvents();
}

void GLFWWindow::SwapBuffers() {
    glfwSwapBuffers(m_window);
}

void GLFWWindow::SetKeyCallback(KeyCallback callback) {
    m_keyCallback = std::move(callback);
}

void* GLFWWindow::GetGraphicsProcAddress(const char* name) const {
    return reinterpret_cast<void*>(glfwGetProcAddress(name));
}

void GLFWWindow::OnKey(GLFWwindow* window, int key, int, int action, int) {
    auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self == nullptr || !self->m_keyCallback) {
        return;
    }

    if (action == GLFW_PRESS) {
        self->m_keyCallback(key, true);
    } else if (action == GLFW_RELEASE) {
        self->m_keyCallback(key, false);
    }
}

} // namespace eng
