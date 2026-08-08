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
    void* GetGraphicsProcAddress(const char* name) const override;

private:
    static void OnKey(GLFWwindow* window, int key, int scanCode, int action, int mods);

    GLFWwindow* m_window = nullptr;
    KeyCallback m_keyCallback;
};

} // namespace eng

#endif
