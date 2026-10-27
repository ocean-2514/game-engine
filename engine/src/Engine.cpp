#include "Engine.h"

#include "Application.h"
#include "Assets/AssetManager.h"
#include "Platform/Window.h"

#include <iostream>

namespace eng {

Engine& Engine::GetInstance() {
    static Engine engine{};
    return engine;
}

bool Engine::Init(Application* app, int width, int height) {
    if (app != nullptr) {
        m_application.reset(app);
    }
    if (!m_application) {
        std::cout << "Engine::Init: Application not set\n";
        return false;
    }

    m_window = Window::Create({width, height, "Engine"});
    if (!m_window) {
        m_application.reset();
        return false;
    }

    m_window->SetKeyCallback([this](int key, bool pressed) {
        m_inputManager.SetKeyPressed(key, pressed);
        if (pressed) {
            m_inputManager.SetKeyWasPressed(key);
        } else {
            m_inputManager.SetKeyWasReleased(key);
        }
    });
    m_window->SetMouseButtonCallback([this](int key, bool pressed) {
        m_inputManager.SetMouseButtonPressed(key, pressed);
        if (pressed) {
            m_inputManager.SetMouseButtonWasPressed(key);
        } else {
            m_inputManager.SetMouseButtonWasReleased(key);
        }
    });
    m_window->SetCursorPosCallback([this](float xpos, float ypos) {
        m_inputManager.RenewMousePosition({xpos, ypos});
    });
    m_window->SetMouseScrollCallback([this](float xoffset, float yoffset) {
        m_inputManager.SetMouseScrollOffset({xoffset, yoffset});
    });

    m_renderDevice = RenderDevice::Create();
    if (!m_renderDevice || !m_renderDevice->Init(*m_window)) {
        m_renderDevice.reset();
        m_window.reset();
        m_application.reset();
        return false;
    }
    if (!m_renderQueue.Init(*m_renderDevice, m_fileSystem)) {
        m_renderQueue.ShutDown();
        m_renderDevice.reset();
        m_window.reset();
        m_application.reset();
        return false;
    }

    m_assetManager = std::make_unique<AssetManager>(
        m_fileSystem, *m_renderDevice, m_audioManager);

    if (!m_audioManager.Init(m_fileSystem)) {
        std::cout << "Engine::Init: AudioManager initialization failed\n";
        m_renderQueue.ShutDown();
        m_application.reset();
        m_assetManager.reset();
        m_renderDevice.reset();
        m_window.reset();
        return false;
    }

    if (!m_application->Init()) {
        std::cout << "Engine::Init: application initialization failed\n";
        m_application->Destroy();
        m_application.reset();
        m_audioManager.Shutdown();
        m_renderQueue.ShutDown();
        m_assetManager.reset();
        m_renderDevice.reset();
        m_window.reset();
        return false;
    }
    return true;
}

void Engine::Run() {
    if (!m_application || !m_window) {
        std::cout << "Engine::Run: engine is not initialized\n";
        return;
    }

    m_lastTimePoint = std::chrono::high_resolution_clock::now();
    while (!m_window->ShouldClose() && !m_application->NeedsToBeClosed()) {
        m_window->PollEvents();
        m_inputManager.BeginFrame();

        const auto currentTimePoint = std::chrono::high_resolution_clock::now();
        const float deltaTime =
            std::chrono::duration<float>(currentTimePoint - m_lastTimePoint).count();
        m_lastTimePoint = currentTimePoint;

        m_application->Update(deltaTime);
        m_audioManager.Update();
        const int framebufferWidth = m_window->GetFrameBufferWidth();
        const int framebufferHeight = m_window->GetFrameBufferHeight();
        if (framebufferWidth > 0 && framebufferHeight > 0 &&
            m_window->GetWindowWidth() > 0 &&
            m_window->GetWindowHeight() > 0) {
            m_application->Render(m_renderQueue);
            m_renderQueue.Execute(*m_renderDevice,
                {0, 0, framebufferWidth, framebufferHeight});
        } else {
            m_renderQueue.Clear();
        }

        m_window->SwapBuffers();

        m_inputManager.ClearState();
    }
}

void Engine::Destroy() {
    if (m_application) {
        m_application->Destroy();
        m_application.reset();
    }
    m_audioManager.Shutdown();
    m_renderQueue.ShutDown();

    // GPU resources owned by the application and device must die while the
    // window's OpenGL context is still alive.
    m_renderQueue.Clear();
    m_renderQueue.InvalidateStateCache();
    m_assetManager.reset();
    m_renderDevice.reset();
    m_window.reset();
}

void Engine::SetApplication(Application* app) {
    m_application.reset(app);
}

Application* Engine::GetApplication() {
    return m_application.get();
}

InputManager& Engine::GetInputManager() {
    return m_inputManager;
}

Window* Engine::GetWindow() {
    return m_window.get();
}

RenderDevice& Engine::GetRenderDevice() {
    return *m_renderDevice;
}

RenderQueue& Engine::GetRenderQueue() {
    return m_renderQueue;
}

FileSystem& Engine::GetFileSystem() {
    return m_fileSystem;
}

AssetManager& Engine::GetAssetManager() {
    return *m_assetManager;
}

AudioManager& Engine::GetAudioManager() {
    return m_audioManager;
}

Engine::~Engine() {
    Destroy();
}

} // namespace eng
