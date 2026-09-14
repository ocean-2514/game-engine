#include "Camera.h"

Camera::Camera() {
    auto& engine = eng::Engine::GetInstance();
    AddComponent<eng::CameraComponent>(&engine.GetInputManager());
    // AddComponent<eng::PlayerControllerComponent>(&engine.GetInputManager());
    SetPosition({.0f, .0f, 3.0f});
}


void Camera::OnUpdate(float deltaTime) {

}
