#include "Scene/Components/CameraComponent.h"
#include "Scene/GameObject.h"
#include "Input/InputManager.h"
#include <glm/gtx/quaternion.hpp>

namespace eng {
    

CameraComponent::CameraComponent(InputManager* inputManager,
    float fov, float znear, float zfar)
    : m_inputManager(inputManager), m_fov(fov),
    m_znear(znear), m_zfar(zfar) {}

void CameraComponent::OnUpdate(float deltaTime) {
    if (!m_inputManager) return;

    m_fov -= m_inputManager->GetMouseScrollOffset().y * m_zoomSpeed;
    m_fov = glm::clamp(m_fov, 30.0f, 70.0f);
}


glm::mat4 CameraComponent::GetViewMatrix() const {
    auto pos = m_owner->GetWorldPosition();
    auto up = m_owner->GetWorldUpward();
    auto front = m_owner->GetWorldForward();
    return glm::lookAt(pos, pos + front, up);
}

glm::mat4 CameraComponent::GetProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(m_fov), aspect, m_znear, m_zfar);
}




} // namespace eng
