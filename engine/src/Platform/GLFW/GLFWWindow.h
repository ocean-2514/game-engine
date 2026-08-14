#ifndef O_GLFW_WINDOW
#define O_GLFW_WINDOW

#include "Platform/Window.h"

struct GLFWwindow;

namespace eng {

class GLFWWindow final : public Window {
public:
    GLFWWindow() = default;
    ~GLFWWindow() override;

    bool Init(const WindowDesc& desc);
    bool ShouldClose() const override;
    void PollEvents() override;
    void SwapBuffers() override;
    void SetKeyCallback(KeyCallback callback) override;
    void SetMouseButtonCallback(MouseButtonCallback callback) override;
    void SetCursorPosCallback(CursorPosCallback callback) override;
    void SetMouseScrollCallback(MouseScrollCallback callback) override;
    void* GetGraphicsProcAddress(const char* name) const override;

private:
    static void OnKey(GLFWwindow* window, int key, int scanCode, int action, int mods);
    static void OnMouseButton(GLFWwindow* window, int button, int action, int mods);
    static void OnCursorPos(GLFWwindow* window, double xpos, double ypos);
    static void OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset);


    GLFWwindow* m_window = nullptr;
    KeyCallback m_keyCallback;
    MouseButtonCallback m_mouseButtonCallback;
    CursorPosCallback m_cursorPosCallback;
    MouseScrollCallback m_mouseScrollCallback;
};

} // namespace eng

#endif
