#ifndef O_WINDOW
#define O_WINDOW

#include <functional>
#include <memory>
#include <string>

namespace eng {

struct WindowDesc {
    int width = 1280;
    int height = 720;
    std::string title = "Engine";
};

class Window {
public:
    using KeyCallback = std::function<void(int key, bool pressed)>;

    virtual ~Window() = default;

    virtual bool ShouldClose() const = 0;
    virtual void PollEvents() = 0;
    virtual void SwapBuffers() = 0;
    virtual void SetKeyCallback(KeyCallback callback) = 0;
    virtual void* GetGraphicsProcAddress(const char* name) const = 0;

    static std::unique_ptr<Window> Create(const WindowDesc& desc);
};

} // namespace eng

#endif
