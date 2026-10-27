#include "Game.h"
#include "Game/GameObjects/TestObject.h"
#include "Game/Factory/ModelFactory.h"
#include "Game/Scripts/ThirdPerspectiveController.h"
#include <iostream>
#include <string>
#include <utility>

bool Game::Init() {
    auto& engine = eng::Engine::GetInstance();
    auto& assetManager = engine.GetAssetManager();
    engine.GetWindow()->SetCursorMode(eng::CursorMode::Normal);

    auto result = assetManager.LoadScene(
        "scenes/scene1.json", &engine.GetInputManager());
    if (result.HasError()) {
        for (const auto& error : *result.GetErrors()) {
            std::cout << error.jsonPath << ": " 
                << error.message << '\n';
        }
        return false;
    }
    scene = std::move(result.Value());

    auto texture = assetManager.LoadTexture({.path = "texture/awesomeface.png"});
    if (!texture) return false;
    auto* obj = scene->CreateObject("testObj");
    if (!obj || !obj->AddComponent<eng::SpriteComponent>(std::move(texture)))
        return false;
    glm::vec2 size{1920.f, 1080.f};
    obj->SetScale2D(size);
    obj->SetPosition2D(size / 2.f);

    const auto model = ModelFactory::CreatePlayerModel();
    eng::GameObject* modelObj = scene->InstantiateModel(model);
    if (!modelObj) return false;
    modelObj->SetPosition(glm::vec3{0.0f, 0.1f, 1.0f});
    auto* rigidBodyComp = modelObj->AddComponent<eng::RigidBodyComponent>(
        eng::RigidBodyDesc{
            .motionType = eng::BodyMotionType::Dynamic,
            .mass = 1.0f,
            .colliders = {
                eng::ColliderDesc{
                    .shape = eng::BoxShapeDesc{glm::vec3{0.55f, 0.975f, 0.35f}},
                    .localPosition = glm::vec3{0.0f, -0.125f, 0.0f},
                }
            },
        }); 
    auto* animationComp = modelObj->GetComponent<eng::AnimationComponent>();
    animationComp->SetController(ModelFactory::CreatePlayerAnimatorController());
    camera = scene->CreateObject<Camera>("MainCamera", modelObj);
    if (!camera || !scene->SetAudioListener(
        camera->AddComponent<eng::AudioListenerComponent>())) return false;
    auto* controller = modelObj->AddComponent<ThirdPerspectiveController>(
        &player,
        static_cast<eng::GameObject*>(camera),
        animationComp,
        rigidBodyComp
    );
    
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
    // if (input.WasMouseButtonPressed(eng::Key::MouseLeft)) {
    //     std::cout << "left mouse button pressed" << std::endl;
    // }

    scene->Update(deltaTime);

    // const auto& collisionEvents = scene->GetCollisionEvents();
    // for (const auto& event : collisionEvents) {
    //     if (event.key.first.index == 0 
    //         || event.key.second.index == 0) continue;
    //     if (event.phase == eng::PhysicsEventPhase::Enter) {
    //         std::cout << "Collision Enter: "
    //             << event.key.first.index << " <-> "
    //             << event.key.second.index << "\n";
    //     } else if (event.phase == eng::PhysicsEventPhase::Stay) {
    //         std::cout << "Collision Stay: "
    //             << event.key.first.index << " <-> "
    //             << event.key.second.index << "\n";
    //     } else if (event.phase == eng::PhysicsEventPhase::Exit) {
    //         std::cout << "Collision Exit: "
    //             << event.key.first.index << " <-> "
    //             << event.key.second.index << "\n";
    //     }
    // }
}

void Game::Render(eng::RenderQueue& queue) {
    eng::Window* window = eng::Engine::GetInstance().GetWindow();
    scene->SetMainCamera(camera->GetComponent<eng::CameraComponent>());
    int width = window->GetWindowWidth();
    int height = window->GetWindowHeight();
    int framebufferWidth = window->GetFrameBufferWidth();
    int framebufferHeight = window->GetFrameBufferHeight();
    if (width <= 0 || height <= 0 ||
        framebufferWidth <= 0 || framebufferHeight <= 0) return;
    // std::cout << width << ' ' << height << '\n';
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    scene->Render(queue, aspect);
    scene->Render2D(queue, eng::ScreenViewDesc{
        .logicalWidth = static_cast<float>(width), 
        .logicalHeight = static_cast<float>(height),
        .viewport = {framebufferWidth / 4, framebufferHeight / 4,
            framebufferWidth / 2, framebufferHeight / 2}
    });
}

void Game::Destroy() {}
