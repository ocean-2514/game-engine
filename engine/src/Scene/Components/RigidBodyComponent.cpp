#include "Scene/Components/RigidBodyComponent.h"
#include "Physics/PhysicsWorld.h"
#include "Scene/Scene.h"

#include <iostream>

namespace eng
{

RigidBodyComponent::RigidBodyComponent(const RigidBodyDesc& desc)
    : m_desc(desc) {}
    
void RigidBodyComponent::AddForce(const glm::vec3& force) {
    if (m_world) {
        m_world->AddForce(m_body, force);
    }
}

void RigidBodyComponent::AddImpulse(const glm::vec3& impulse) {
    if (m_world) {
        m_world->AddImpulse(m_body, impulse);
    }
}

void RigidBodyComponent::AddTorque(const glm::vec3& torque) {
    if (m_world) {
        m_world->AddTorque(m_body, torque);
    }
}

void RigidBodyComponent::SetLinearVelocity(const glm::vec3& velocity) {
    if (m_world) {
        m_world->SetLinearVelocity(m_body, velocity);
    }
}

glm::vec3 RigidBodyComponent::GetLinearVelocity() const {
    if (m_world) {
        return m_world->GetLinearVelocity(m_body);
    }
    return glm::vec3(0.0f);
}

void RigidBodyComponent::SetAngularVelocity(const glm::vec3& velocity) {
    if (m_world) {
        m_world->SetAngularVelocity(m_body, velocity);
    }
}

glm::vec3 RigidBodyComponent::GetAngularVelocity() const {
    return m_world
        ? m_world->GetAngularVelocity(m_body)
        : glm::vec3{0.0f};
}

void RigidBodyComponent::SetLocalInertia(const glm::vec3& inertia) {
    if (m_world) {
        m_world->SetLocalInertia(m_body, inertia);
    }
}

void RigidBodyComponent::SetAngularFactor(const glm::vec3& factor) {
    if (m_world) {
        m_world->SetAngularFactor(m_body, factor);
    }
}

void RigidBodyComponent::SetWorldRotation(const glm::quat& rotation) {
    if (m_world) {
        m_world->SetWorldRotation(m_body, rotation);
    }
}

void RigidBodyComponent::Teleport(const glm::vec3& worldPosition,
    const glm::quat& worldRotation, bool clearVelocity) {
    if (m_world && m_world->Teleport(
            m_body, worldPosition, worldRotation, clearVelocity)) {
        m_owner->SetWorldPosition(worldPosition);
        m_owner->SetWorldRotation(glm::normalize(worldRotation));
    }
}

void RigidBodyComponent::WakeUp() {
    if (m_world) {
        m_world->WakeUp(m_body);
    }
}

void RigidBodyComponent::SetEnabled(bool enabled) {
    if (m_world) {
        m_world->SetBodyEnabled(m_body, enabled);
    }
}

void RigidBodyComponent::PushKinematicTransform() {
    if (m_world && m_desc.motionType == BodyMotionType::Kinematic) {
        m_world->SetKinematicTransform(m_body, GetPosition(), GetRotation());
    }
}

void RigidBodyComponent::PullDynamicTransform() {
    if (m_world && m_desc.motionType == BodyMotionType::Dynamic) {
        glm::vec3 position;
        glm::quat rotation;
        if (m_world->GetBodyTransform(m_body, position, rotation)) {
            m_owner->SetWorldPosition(position);
            m_owner->SetWorldRotation(rotation);
        }
    }
}

bool RigidBodyComponent::ApplyRenderInterpolation(float alpha) {
    if (!m_world || !m_owner || m_hasRenderOverride ||
        m_desc.motionType != BodyMotionType::Dynamic) {
        return false;
    }

    glm::vec3 position;
    glm::quat rotation;
    if (!m_world->GetInterpolatedBodyTransform(
            m_body, alpha, position, rotation)) {
        return false;
    }

    m_savedWorldPosition = m_owner->GetWorldPosition();
    m_savedWorldRotation = m_owner->GetWorldRotation();
    m_owner->SetWorldPosition(position);
    m_owner->SetWorldRotation(rotation);
    m_hasRenderOverride = true;
    return true;
}

void RigidBodyComponent::RestoreSimulationTransform() {
    if (!m_hasRenderOverride || !m_owner) return;
    m_owner->SetWorldPosition(m_savedWorldPosition);
    m_owner->SetWorldRotation(m_savedWorldRotation);
    m_hasRenderOverride = false;
}

void RigidBodyComponent::OnAttach(Scene& scene) {
    m_world = scene.GetPhysicsWorld();
    if (m_world) {
        m_body = m_world->CreateBody(m_desc, GetPosition(), GetRotation());
        if (!m_body.IsValid()) {
            std::cout << "RigidBodyComponent::OnAttach: failed to create body\n";
            m_world = nullptr;
            MarkForDestroy();
            return;
        }
        scene.RegisterRigidBody(this, m_body);
    }
}

void RigidBodyComponent::OnDetach(Scene& scene) {
    RestoreSimulationTransform();
    scene.UnregisterRigidBody(this);
    if (m_world && m_body.IsValid()) {
        m_world->DestroyBody(m_body);
    }
    m_body = {};
    m_world = nullptr;
}

const PhysicsWorld* RigidBodyComponent::GetWorld() const {
    return m_world;
}

} // namespace eng
