#include "Game.h"
#include "TestObject.h"
#include <iostream>
#include <string>
#include <utility>

bool Game::Init() {
    scene.CreateObject<TestObject>("test object", nullptr);
    return true;
}

void Game::Update(float deltaTime) {
    auto& renderDevice = eng::Engine::GetInstance().GetRenderDevice();
    renderDevice.Clear({
        eng::ClearBuffer::Color,
        {0.08f, 0.08f, 1.0f, 0.5f}
    });
    
    auto& input = eng::Engine::GetInstance().GetInputManager();
    if (input.IsKeyPressed(eng::Key::Escape)) {
        eng::Engine::GetInstance().GetApplication()->SetNeedsToBeClosed(true);
    }

    scene.Update(deltaTime);

}

void Game::Destroy() {}