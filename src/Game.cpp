#include "Game.h"
#include <iostream>
#include <string>

bool Game::Init() {
    auto& renderDevice = eng::Engine::GetInstance().GetRenderDevice();
    auto shader = renderDevice.CreateShaderProgram(
        std::string(SHADER_DIR) + "/cloth_vs.glsl",
        std::string(SHADER_DIR) + "/cloth_fs.glsl");
    return shader != nullptr;
}

void Game::Update(float deltaTime) {
    auto& input = eng::Engine::GetInstance().GetInputManager();
    if (input.IsKeyPressed(eng::Key::A)) {
        std::cout << "A is Pressed" << std::endl;
    }
}

void Game::Destroy() {

}
