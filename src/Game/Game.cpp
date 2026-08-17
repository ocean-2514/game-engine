#include "Game.h"
#include "TestObject.h"
#include <iostream>
#include <string>
#include <utility>

bool Game::Init() {
    auto& engine = eng::Engine::GetInstance();
    m_materialAssetLoader = std::make_unique<eng::MaterialAssetLoader>(
        engine.GetFileSystem(), engine.GetRenderDevice());

    auto* cube = scene.CreateObject<TestObject>(
        "cube", nullptr, TestObject::Shape::Cube,
        glm::vec3(0.9f, 0.3f, 0.2f), *m_materialAssetLoader);
    auto* sphere = scene.CreateObject<TestObject>(
        "sphere", nullptr, TestObject::Shape::Sphere,
        glm::vec3(0.2f, 0.6f, 0.9f), *m_materialAssetLoader);
    auto* plane = scene.CreateObject<TestObject>(
        "plane", nullptr, TestObject::Shape::Plane,
        glm::vec3(0.3f, 0.7f, 0.3f), *m_materialAssetLoader);

    cube->SetPosition({-1.5f, 0.0f, 0.0f});
    sphere->SetPosition({1.5f, 0.0f, 0.0f});
    plane->SetPosition({0.0f, 0.0f, 0.0f});

    camera = scene.CreateObject<Camera>("MainCamera", nullptr);

    return true;
}

void Game::Update(float deltaTime) {
    auto& renderDevice = eng::Engine::GetInstance().GetRenderDevice();
    renderDevice.Clear({
        eng::ClearBuffer::Color | eng::ClearBuffer::Depth,
        {0.08f, 0.08f, 1.0f, 0.5f}
    });
    
    auto& input = eng::Engine::GetInstance().GetInputManager();
    if (input.IsKeyPressed(eng::Key::Escape)) {
        eng::Engine::GetInstance().GetApplication()->SetNeedsToBeClosed(true);
    }

    scene.Update(deltaTime);
}

void Game::Render(eng::RenderQueue& queue) {
    eng::Window* window = eng::Engine::GetInstance().GetWindow();
    scene.SetMainCamera(camera->GetComponent<eng::CameraComponent>());
    const float aspect = static_cast<float>(window->GetWidth()) /
        static_cast<float>(window->GetHeight());
    scene.Render(queue, aspect);
}

void Game::Destroy() {}
