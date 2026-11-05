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

    // ui
    auto texture = assetManager.LoadTexture({.path = "texture/awesomeface.png"});
    uiContext = std::make_unique<eng::UIContext>();
    auto image = std::make_unique<eng::UIImage>(texture);
    image->SetVisibility(eng::UIVisibility::Collapsed);
    auto button = std::make_unique<eng::UIButton>();
    button->SetRect({100.0f, 100.0f, 100.0f, 100.0f});
    button->SetBackground(image.get());
    button->AddClickListener([]{
        std::cout << "clicked" << std::endl;
    });
    auto font = assetManager.LoadFont({.path = "font/仿宋_GB2312.ttf"});
    auto text = std::make_unique<eng::UIText>(font);
    text->SetText("hello, 世界");
    text->SetPixelSize(30.0f);
    text->SetAlign(eng::TextAlign::Right);
    button->SetLabel(font, "AHA!! 啊哈 --version 1.345");
    button->SetLabelStyle({
        .pixelSize = 30.0f,
        .color = {1.f, 0.f, 0.f, 1.f},
        .align = eng::TextAlign::Center, 
    });
    // uiContext->AddRoot(std::move(image));
    uiContext->AddRoot(std::move(button));
    uiContext->AddRoot(std::move(text));
    
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

    auto* window = eng::Engine::GetInstance().GetWindow();
    uiContext->SetView({
        static_cast<float>(window->GetWindowWidth()),
        static_cast<float>(window->GetWindowHeight()), eng::ScreenOrigin::TopLeft,
        {0, 0, window->GetFrameBufferWidth(), window->GetFrameBufferHeight()}
    });
    uiContext->ProcessInput(input.GetEvents(), 
        {
            static_cast<float>(window->GetWindowWidth()), 
            static_cast<float>(window->GetWindowHeight()), 
            window->GetFrameBufferWidth(), window->GetFrameBufferHeight()
        });
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
    float aspect = static_cast<float>(width) / static_cast<float>(height);

    scene->Render(queue, aspect);
    scene->Render2D(queue, eng::ScreenViewDesc{
        .logicalWidth = static_cast<float>(width), 
        .logicalHeight = static_cast<float>(height),
        .viewport = {framebufferWidth / 4, framebufferHeight / 4,
            framebufferWidth / 2, framebufferHeight / 2}
    });
    uiContext->Submit(queue); // UI is the final screen-space overlay.
}

void Game::Destroy() {}
