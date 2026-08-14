#include "Scene/Components/PlayerControllerComponent.h"
#include "Input/InputManager.h"
#include "Scene/GameObject.h"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace eng {

PlayerControllerComponent::PlayerControllerComponent(
    InputManager* inputManager, 
    float sensitivity, float moveSpeed) 
    : m_inputManager(inputManager), 
    m_sensitivity(sensitivity), 
    m_moveSpeed(moveSpeed) {}


void PlayerControllerComponent::SetWorldUp(const glm::vec3& worldUp) {
    if (glm::dot(worldUp, worldUp) > 0.0f) {
        m_worldUp = glm::normalize(worldUp);
    }
}

const glm::vec3& PlayerControllerComponent::GetWorldUp() const {
    return m_worldUp;
}

void PlayerControllerComponent::SetMoveHorizontally(bool horizontal) {
    m_moveHorizontally = horizontal;
}

    
void PlayerControllerComponent::OnUpdate(float deltaTime) {
    if (m_inputManager == nullptr) return;

    const glm::vec2 delta =
        m_inputManager->GetMousePositionDelta() * m_sensitivity;
    m_yaw -= delta.x;
    m_pitch -= delta.y;
    m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);

    glm::vec3 referenceForward{0.0f, 0.0f, -1.0f};
    referenceForward -=
        m_worldUp * glm::dot(referenceForward, m_worldUp);
    if (glm::dot(referenceForward, referenceForward) < 0.000001f) {
        referenceForward = glm::vec3{1.0f, 0.0f, 0.0f};
        referenceForward -=
            m_worldUp * glm::dot(referenceForward, m_worldUp);
    }
    referenceForward = glm::normalize(referenceForward);
    const glm::vec3 referenceRight =
        glm::normalize(glm::cross(referenceForward, m_worldUp));
    const glm::quat yawRotation =
        glm::angleAxis(glm::radians(m_yaw), m_worldUp);
    const glm::quat pitchRotation =
        glm::angleAxis(glm::radians(m_pitch), referenceRight);
    m_owner->SetRotation(glm::normalize(yawRotation * pitchRotation));

    glm::vec3 moveDirection{0.0f};
    const glm::vec3 right = m_owner->GetWorldRight();
    const glm::vec3 horizontalFront =
        glm::normalize(glm::cross(m_worldUp, right));
    if (m_inputManager->IsKeyPressed(eng::Key::A)) {
        moveDirection -= right;
    } 
    if (m_inputManager->IsKeyPressed(eng::Key::D)) {
        moveDirection += right;
    }
    if (m_inputManager->IsKeyPressed(eng::Key::W)) {
        moveDirection += m_moveHorizontally ? horizontalFront :
            m_owner->GetWorldForward();
    }
    if (m_inputManager->IsKeyPressed(eng::Key::S)) {
        moveDirection -= m_moveHorizontally ? horizontalFront :
            m_owner->GetWorldForward();
    }
    if (m_inputManager->IsKeyPressed(eng::Key::Q)) {
        moveDirection += m_worldUp;
    }
    if (m_inputManager->IsKeyPressed(eng::Key::E)) {
        moveDirection -= m_worldUp;
    }

    if (glm::dot(moveDirection, moveDirection) > 0.0f) {
        moveDirection = glm::normalize(moveDirection);
        m_owner->SetPosition(m_owner->GetPosition() +
            moveDirection * m_moveSpeed * deltaTime);
    }
}






} // namespace eng
