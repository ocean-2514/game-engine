#ifndef O_ENGINE
#define O_ENGINE

#include <memory>
#include <chrono>

#include "Input/InputManager.h"
#include "Renderer/RenderDevice.h"
#include "Renderer/RenderQueue.h"
#include "IO/FileSystem.h"

namespace eng {

class Application;
class Window;

class Engine {
public:
    Engine(const Engine& other) = delete;
    Engine(Engine&& other) = delete;
    Engine& operator=(const Engine& other) = delete;
    Engine& operator=(Engine&& other) = delete;
    
    static Engine& GetInstance();
    
    bool Init(Application* app = nullptr, int width = 1280, int height = 720);
    void Run();
    void Destroy();
    
    void SetApplication(Application* app);
    Application* GetApplication();
    InputManager& GetInputManager();
    Window* GetWindow();
    RenderDevice& GetRenderDevice();
    RenderQueue& GetRenderQueue();
    FileSystem& GetFileSystem();
    
private:
    std::unique_ptr<Application> m_application;
    std::unique_ptr<Window> m_window;
    std::unique_ptr<RenderDevice> m_renderDevice;
    InputManager m_inputManager;
    RenderQueue m_renderQueue;
    FileSystem m_fileSystem;

    std::chrono::system_clock::time_point m_lastTimePoint;
    Engine() = default;
    ~Engine();

};

    
}


#endif
