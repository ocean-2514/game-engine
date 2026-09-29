#include "Game/Scripts/ThirdPerspectiveController.h"

#include <glm/glm.hpp>
#include <iostream>

ThirdPerspectiveController::ThirdPerspectiveController(
    Player* player,
    eng::GameObject* camera, float distance, 
    float sensitivity) 
    : m_player(player),
    m_camera(camera), m_distance(distance), 
    m_sensitivity(sensitivity) {
    camera->SetPosition({0.0f, 0.0f, m_distance});
}

void ThirdPerspectiveController::SetAnimationComponent(
    eng::AnimationComponent* comp) {
    m_animationComponent = comp;
}


void ThirdPerspectiveController::OnUpdate(float deltaTime) {
    if (m_camera == nullptr || m_player == nullptr) {
        return;
    }

    auto& inputManager = eng::Engine::GetInstance().GetInputManager();

    const glm::vec2 delta =
        inputManager.GetMousePositionDelta() * m_sensitivity;
    m_cameraYawWorld -= delta.x;
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

    glm::vec3 moveDirection{0.0f};
    // right and front vectors are in camera space
    const glm::vec3 right = m_camera->GetWorldRight();
    const glm::vec3 horizontalFront =
        glm::normalize(glm::cross(m_worldUp, right));
    if (inputManager.IsKeyPressed(eng::Key::A)) {
        moveDirection -= right;
    } 
    if (inputManager.IsKeyPressed(eng::Key::D)) {
        moveDirection += right;
    }
    if (inputManager.IsKeyPressed(eng::Key::W)) {
        moveDirection += horizontalFront;
    }
    if (inputManager.IsKeyPressed(eng::Key::S)) {
        moveDirection -= horizontalFront;
    }
    if (inputManager.IsKeyPressed(eng::Key::Q)) {
        moveDirection += m_worldUp;
    }
    if (inputManager.IsKeyPressed(eng::Key::E)) {
        moveDirection -= m_worldUp;
    }
    if (inputManager.IsKeyPressed(eng::Key::Space)) {
        if (!m_spacePressed) {
            m_spacePressed = true;
            m_animationComponent->SetTrigger("Jump");
        }
    } else {
        if (m_spacePressed) m_spacePressed = false;
    }
    if (inputManager.IsMouseButtonPressed(eng::Key::MouseRight)) {
        if (!m_rightMousePressed) {
            m_rightMousePressed = true;
            if (m_player->IsMoving()) {
                m_shouldRunning = !m_shouldRunning;
            }
        }
    } else {
        if (m_rightMousePressed) m_rightMousePressed = false;
    }

    bool moving = glm::dot(moveDirection, moveDirection) > 0.0f;
    if (moving) {
        m_player->SetMoveState(
            m_shouldRunning ? PlayerMoveState::Running : PlayerMoveState::Walking
        );
        m_player->SetSpeed(m_shouldRunning ? 4.0f : 2.0f);
    } else {
        m_shouldRunning = false;
        m_player->SetMoveState(PlayerMoveState::Idle);
        m_player->SetSpeed(0.0f);
    }
    m_animationComponent->SetFloat("Speed", m_player->GetSpeed());
    if (moving) {
        moveDirection = glm::normalize(moveDirection);
        m_owner->SetPosition(m_owner->GetPosition() +
            moveDirection * m_player->GetSpeed() * deltaTime);
    }
    
    ComputeModelYaw(moveDirection, deltaTime);
    glm::quat modelRotation = glm::angleAxis(glm::radians(m_modelCurrentYaw), m_worldUp);
    m_owner->SetRotation(modelRotation);

    float cameraYawLocal = m_cameraYawWorld - m_modelCurrentYaw;
    glm::quat cameraYawRotation = glm::angleAxis(
        glm::radians(cameraYawLocal), m_worldUp);
    const glm::quat cameraPitchRotation =
        glm::angleAxis(glm::radians(m_pitch), referenceRight);
    m_camera->SetPosition({
        m_distance * std::cos(glm::radians(m_pitch)) * std::sin(glm::radians(cameraYawLocal)),
        - m_distance * std::sin(glm::radians(m_pitch)), 
        m_distance * std::cos(glm::radians(m_pitch)) * std::cos(glm::radians(cameraYawLocal))
    });
    m_camera->SetRotation(cameraYawRotation * cameraPitchRotation);
}

void ThirdPerspectiveController::ComputeModelYaw(const glm::vec3& moveDirection, 
    float deltaTime) {
    glm::vec3 direction = moveDirection - glm::dot(moveDirection, m_worldUp) * m_worldUp;
    bool moving = glm::dot(direction, direction) > 1e-6f;
    if (!moving) {
        m_modelTargetYaw = m_modelCurrentYaw;
        return;
    } else {
        m_modelTargetYaw = glm::degrees(std::atan2(-direction.x, -direction.z));
        m_modelTargetYaw = std::fmod(m_modelTargetYaw, 360.0f);
        if (m_modelTargetYaw < 0.0f) {
            m_modelTargetYaw += 360.0f;
        }
        float diff = m_modelTargetYaw - m_modelCurrentYaw;
        if (diff > 180.0f) {
            m_modelTargetYaw -= 360.0f;
        } else if (diff < -180.0f) {
            m_modelTargetYaw += 360.0f;
        }
        m_modelCurrentYaw = glm::mix(m_modelCurrentYaw, m_modelTargetYaw, deltaTime * 20.0f);
    }
}