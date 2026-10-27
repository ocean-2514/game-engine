#ifndef O_WINDOW
#define O_WINDOW

#include <functional>
#include <memory>
#include <string>

namespace eng {

struct WindowDesc {
    int width = 1920;
    int height = 1080;
    std::string title = "Engine";
};

enum class CursorMode {
    Normal, Hidden, Disabled, Captured
};

class Window {
public:
    using KeyCallback = std::function<void(int key, bool pressed)>;
    using MouseButtonCallback = std::function<void(int key, bool pressed)>;
    using CursorPosCallback = std::function<void(float xpos, float ypos)>;
    using MouseScrollCallback = std::function<void(float xoffset, float yoffset)>;

    virtual ~Window() = default;

    virtual bool ShouldClose() const = 0;
    virtual void PollEvents() = 0;
    virtual void SwapBuffers() = 0;
    virtual void SetCursorMode(CursorMode mode) = 0;
    virtual void SetKeyCallback(KeyCallback callback) = 0;
    virtual void SetMouseButtonCallback(MouseButtonCallback callback) = 0;
    virtual void SetCursorPosCallback(CursorPosCallback callback) = 0;
    virtual void SetMouseScrollCallback(MouseScrollCallback callback) = 0;
    virtual void* GetGraphicsProcAddress(const char* name) const = 0;

    int GetWindowWidth() const;
    int GetWindowHeight() const;
    int GetFrameBufferWidth() const;
    int GetFrameBufferHeight() const;

    static std::unique_ptr<Window> Create(const WindowDesc& desc);

protected:
    int m_windowWidth = 0;
    int m_windowHeight = 0;
    int m_frameBufferWidth = 0;
    int m_frameBufferHeight = 0;
};

} // namespace eng

#endif
