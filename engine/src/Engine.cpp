#include "Engine.h"

#include "Application.h"
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
    });

    m_renderDevice = RenderDevice::Create();
    if (!m_renderDevice || !m_renderDevice->Init(*m_window)) {
        m_renderDevice.reset();
        m_window.reset();
        m_application.reset();
        return false;
    }

    if (!m_application->Init()) {
        std::cout << "Engine::Init: application initialization failed\n";
        m_application->Destroy();
        m_application.reset();
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

        const auto currentTimePoint = std::chrono::high_resolution_clock::now();
        const float deltaTime =
            std::chrono::duration<float>(currentTimePoint - m_lastTimePoint).count();
        m_lastTimePoint = currentTimePoint;

        m_application->Update(deltaTime);

        m_renderQueue.Execute();

        m_window->SwapBuffers();
    }
}

void Engine::Destroy() {
    if (m_application) {
        m_application->Destroy();
        m_application.reset();
    }

    // GPU resources owned by the application and device must die while the
    // window's OpenGL context is still alive.
    m_renderQueue.Clear();
    m_renderQueue.InvalidateStateCache();
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

Engine::~Engine() {
    Destroy();
}

} // namespace eng
