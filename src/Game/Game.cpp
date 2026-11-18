#include "Game.h"
#include "Game/GameObjects/TestObject.h"
#include "Game/Factory/ModelFactory.h"
#include "Game/Scripts/ThirdPerspectiveController.h"
#include <iostream>
#include <string>
#include <utility>

bool Game::Init() {
    auto& engine = eng::Engine::GetInstance();
    engine.GetWindow()->SetCursorMode(eng::CursorMode::Normal);

    if (!InitSampleScene()) return false;
    if (!InitMainMenuScene()) return false;

    m_activeScene = m_mainMenuScene.get();
    m_activeScene->Activate();
    
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
    auto* uiContext = m_activeScene->GetUIContext();
    if (uiContext) {
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
    }

    m_activeScene->Update(deltaTime);
}

void Game::Render(eng::RenderQueue& queue) {
    eng::Window* window = eng::Engine::GetInstance().GetWindow();
    int width = window->GetWindowWidth();
    int height = window->GetWindowHeight();
    int framebufferWidth = window->GetFrameBufferWidth();
    int framebufferHeight = window->GetFrameBufferHeight();
    if (width <= 0 || height <= 0 ||
        framebufferWidth <= 0 || framebufferHeight <= 0) return;
    float aspect = static_cast<float>(width) / static_cast<float>(height);

    m_activeScene->Render(queue, aspect);
    // m_activeScene->Render2D(queue, eng::ScreenViewDesc{
    //     .logicalWidth = static_cast<float>(width), 
    //     .logicalHeight = static_cast<float>(height),
    //     .viewport = {framebufferWidth / 4, framebufferHeight / 4,
    //         framebufferWidth / 2, framebufferHeight / 2}
    // });

    auto* uiContext = m_activeScene->GetUIContext();
    if (uiContext) uiContext->Submit(queue); 
}

void Game::Destroy() {}

bool Game::InitSampleScene() {
    auto& engine = eng::Engine::GetInstance();
    auto& assetManager = engine.GetAssetManager();

    auto result = assetManager.LoadScene(
        "scenes/scene1.json", &engine.GetInputManager());
    if (result.HasError()) {
        for (const auto& error : *result.GetErrors()) {
            std::cout << error.jsonPath << ": " 
                << error.message << '\n';
        }
        return false;
    }

    m_sampleScene = std::move(result.Value());

    const auto model = ModelFactory::CreatePlayerModel();
    eng::GameObject* modelObj = m_sampleScene->InstantiateModel(model);
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
    Camera* camera = m_sampleScene->CreateObject<Camera>("MainCamera", modelObj);
    if (!camera || !m_sampleScene->SetAudioListener(
        camera->AddComponent<eng::AudioListenerComponent>())) return false;
    m_sampleScene->SetMainCamera(camera->GetComponent<eng::CameraComponent>());
    modelObj->AddComponent<ThirdPerspectiveController>(
        &player,
        static_cast<eng::GameObject*>(camera),
        animationComp,
        rigidBodyComp
    );

    return true;
}

bool Game::InitMainMenuScene() {
    auto& engine = eng::Engine::GetInstance();
    auto& assetManager = engine.GetAssetManager();

    m_mainMenuScene = std::make_unique<eng::Scene>();
    auto& uiContext = m_mainMenuScene->GetOrCreateUIContext();

    // ui
    auto texture = assetManager.LoadTexture({.path = "texture/awesomeface.png"});
    auto image = std::make_unique<eng::UIImage>(texture);
    // image->SetVisibility(eng::UIVisibility::Collapsed);

    auto button = std::make_unique<eng::UIButton>();
    button->SetRect({100.0f, 100.0f, 100.0f, 100.0f});
    button->SetBackground(image.get());
    button->AddClickListener([this]{
        std::cout << "clicked" << std::endl;
        EnterSampleScene();
    });

    auto font = assetManager.LoadFont({.path = "font/仿宋_GB2312.ttf"});
    auto text = std::make_unique<eng::UIText>(font);
    text->SetText("hello, 世界");
    text->SetPixelSize(30.0f);
    text->SetAlign(eng::TextAlign::Right);
    button->SetLabel(font, "开始");
    button->SetLabelStyle({
        .pixelSize = 30.0f,
        .color = {1.f, 0.f, 0.f, 1.f},
        .align = eng::TextAlign::Center, 
    });

    uiContext.AddRoot(std::move(image));
    uiContext.AddRoot(std::move(button));
    uiContext.AddRoot(std::move(text));

    return true;
}

void Game::EnterSampleScene() {
    m_activeScene->Deactivate();
    m_activeScene = m_sampleScene.get();
    m_activeScene->Activate();
}