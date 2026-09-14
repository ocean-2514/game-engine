#include "Game.h"
#include "TestObject.h"
#include "ModelFactory.h"
#include "ThirdPerspectiveController.h"
#include <iostream>
#include <string>
#include <utility>

bool Game::Init() {
    auto& engine = eng::Engine::GetInstance();
    auto& assets = engine.GetAssetManager();

    auto* cube = scene.CreateObject<TestObject>(
        "cube", nullptr, TestObject::Shape::Cube,
        glm::vec3(0.9f, 0.3f, 0.2f), assets);
    auto* sphere = scene.CreateObject<TestObject>(
        "sphere", nullptr, TestObject::Shape::Sphere,
        glm::vec3(0.2f, 0.6f, 0.9f), assets);
    auto* plane = scene.CreateObject<TestObject>(
        "plane", nullptr, TestObject::Shape::Plane,
        glm::vec3(0.3f, 0.7f, 0.3f), assets);

    cube->SetPosition({-1.5f, 0.0f, 0.0f});
    sphere->SetPosition({1.5f, 0.0f, 0.0f});
    plane->SetPosition({0.0f, 0.0f, 0.0f});

    // camera = scene.CreateObject<Camera>("MainCamera", nullptr);

    auto* light = scene.CreateObject("directionalLight");
    light->AddComponent<eng::DirectionalLightComponent>();
    light->Rotate(10.0f, glm::vec3(-1.0f, 0.0f, 0.0f));

    const auto model = ModelFactory::CreatePlayerModel();
    // const auto model = assets.LoadModel("model/odette/奥黛塔.pmx"); 
    eng::GameObject* modelObj = scene.InstantiateModel(model);
    camera = scene.CreateObject<Camera>("MainCamera", modelObj);
    modelObj->AddComponent<ThirdPerspectiveController>(
        &player,
        static_cast<eng::GameObject*>(camera));
    if (modelObj) { 
        modelObj->SetPosition(glm::vec3{0.0f, 0.0f, 1.0f});
        // modelObj->SetScale(glm::vec3{0.01f});
        // modelObj->SetPosition(glm::vec3{0.0f, 1.0f, -1.0f});
    }
    auto* animationComp = modelObj->GetComponent<eng::AnimationComponent>();
    if (animationComp) {
        // animationComp->Play("Walking", true);
        animationComp->SetPlaying(true);
    }

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
