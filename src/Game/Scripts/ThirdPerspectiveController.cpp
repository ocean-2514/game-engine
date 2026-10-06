#include "Game/Scripts/ThirdPerspectiveController.h"

#include <glm/glm.hpp>
#include <cmath>
#include <iostream>

ThirdPerspectiveController::ThirdPerspectiveController(
    Player* player,
    eng::GameObject* camera, 
    eng::AnimationComponent* animationComponent,
    eng::RigidBodyComponent* rigidBodyComponent,
    float distance, float sensitivity) 
    : m_player(player), m_camera(camera), 
    m_animationComponent(animationComponent),
    m_rigidBodyComponent(rigidBodyComponent),
    m_distance(distance), m_sensitivity(sensitivity) {
    camera->SetPosition({0.0f, 0.0f, m_distance});
    m_rigidBodyComponent->SetLocalInertia(glm::vec3(0.f, 0.f, 0.f));
    m_rigidBodyComponent->SetAngularFactor(glm::vec3(0.f, 0.f, 0.f));
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

    glm::vec3 moveDirection{0.0f};
    const float cameraYawRadians = glm::radians(m_cameraYawWorld);
    const glm::vec3 right{
        std::cos(cameraYawRadians), 0.0f, -std::sin(cameraYawRadians)};
    const glm::vec3 horizontalFront{
        -std::sin(cameraYawRadians), 0.0f, -std::cos(cameraYawRadians)};
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
            if (CanJump()) {
                m_animationComponent->SetTrigger("Jump");
                m_rigidBodyComponent->AddImpulse(glm::vec3{0.0f, 5.0f, 0.0f});
            }
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
    const float verticalInput = glm::dot(moveDirection, m_worldUp);
    glm::vec3 velocity{0.0f};
    if (moving) {
        moveDirection = glm::normalize(moveDirection);
        velocity = moveDirection * m_player->GetSpeed();
    }
    if (std::abs(verticalInput) <= 1e-6f) {
        const glm::vec3 currentVelocity =
            m_rigidBodyComponent->GetLinearVelocity();
        velocity += m_worldUp * glm::dot(currentVelocity, m_worldUp);
    }
    m_rigidBodyComponent->SetLinearVelocity(velocity);
    
    ComputeModelYaw(moveDirection, deltaTime);
    glm::quat modelRotation = glm::angleAxis(glm::radians(m_modelCurrentYaw), m_worldUp);
    m_rigidBodyComponent->SetWorldRotation(modelRotation);

    float cameraYawLocal = m_cameraYawWorld - m_modelCurrentYaw;
    glm::quat cameraYawRotation = glm::angleAxis(
        glm::radians(cameraYawLocal), m_worldUp);
    const glm::quat cameraPitchRotation = glm::angleAxis(
        glm::radians(m_pitch), glm::vec3{1.0f, 0.0f, 0.0f});
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
        const float blend = 1.0f - std::exp(-20.0f * deltaTime);
        m_modelCurrentYaw = glm::mix(
            m_modelCurrentYaw, m_modelTargetYaw, blend);
    }
}

bool ThirdPerspectiveController::CanJump() const {
    const auto* world = m_rigidBodyComponent->GetWorld();
    glm::vec3 pos = m_rigidBodyComponent->GetPosition();
    pos.y -= 1.08f;
    eng::RaycastHit hit;
    if (world->RaycastClosest({pos, -m_worldUp, 0.1f}, hit)) {
        return true;
    }
    return false;
}