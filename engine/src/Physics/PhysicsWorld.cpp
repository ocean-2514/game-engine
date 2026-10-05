#include "Physics/PhysicsWorld.h"
#include "Physics/Bullet/BulletPhysicsWorld.h"

namespace eng
{

struct PhysicsWorld::Impl {
    std::unique_ptr<BulletPhysicsWorld> world;

    Impl(const PhysicsWorldDesc& desc) {
        world = std::make_unique<BulletPhysicsWorld>(desc);
    }
};

PhysicsWorld::PhysicsWorld(const PhysicsWorldDesc& desc)
    : m_desc(desc) {
    if (desc.IsValid()) {
        m_impl = std::make_unique<Impl>(desc);
    }
}

PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::Simulate(float fixedDeltaTime) {
    if (!m_impl || !std::isfinite(fixedDeltaTime) ||
        fixedDeltaTime <= 0.0f) return;
    m_impl->world->Simulate(fixedDeltaTime);
}

void PhysicsWorld::SetGravity(const glm::vec3& gravity) {
    if (!m_impl || !IsFiniteVector(gravity)) return;
    m_impl->world->SetGravity(gravity);
    m_desc.gravity = gravity;
}

glm::vec3 PhysicsWorld::GetGravity() const {
    if (!m_impl) return glm::vec3{0.0f};
    return m_impl->world->GetGravity();
}

const PhysicsWorldDesc& PhysicsWorld::GetDesc() const {
    return m_desc;
}

bool PhysicsWorld::IsValid() const {
    return m_impl != nullptr;
}

void PhysicsWorld::AddForce(PhysicsBodyHandle handle, const glm::vec3& force) {
    if (!m_impl || !IsFiniteVector(force)) return;
    m_impl->world->AddForce(handle, force);
}

void PhysicsWorld::AddImpulse(PhysicsBodyHandle handle, const glm::vec3& impulse) {
    if (!m_impl || !IsFiniteVector(impulse)) return;
    m_impl->world->AddImpulse(handle, impulse);
}

void PhysicsWorld::AddTorque(PhysicsBodyHandle handle, const glm::vec3& torque) {
    if (!m_impl || !IsFiniteVector(torque)) return;
    m_impl->world->AddTorque(handle, torque);
}

void PhysicsWorld::SetLinearVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity) {
    if (!m_impl || !IsFiniteVector(velocity)) return;
    m_impl->world->SetLinearVelocity(handle, velocity);
}

glm::vec3 PhysicsWorld::GetLinearVelocity(PhysicsBodyHandle handle) const {
    if (!m_impl) return glm::vec3{0.0f};
    return m_impl->world->GetLinearVelocity(handle);
}

void PhysicsWorld::SetAngularVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity) {
    if (!m_impl || !IsFiniteVector(velocity)) return;
    m_impl->world->SetAngularVelocity(handle, velocity);
}

glm::vec3 PhysicsWorld::GetAngularVelocity(PhysicsBodyHandle handle) const {
    if (!m_impl) return glm::vec3{0.0f};
    return m_impl->world->GetAngularVelocity(handle);
}

bool PhysicsWorld::Teleport(PhysicsBodyHandle handle,
    const glm::vec3& worldPosition, const glm::quat& worldRotation,
    bool clearVelocity) {
    if (!m_impl || !IsFiniteVector(worldPosition) ||
        !IsFiniteQuaternion(worldRotation) ||
        glm::length(worldRotation) <= 0.0f) return false;
    return m_impl->world->Teleport(
        handle, worldPosition, worldRotation, clearVelocity);
}

bool PhysicsWorld::SetKinematicTransform(PhysicsBodyHandle handle,
    const glm::vec3& worldPosition,
    const glm::quat& worldRotation) {
    if (!m_impl || !IsFiniteVector(worldPosition) ||
        !IsFiniteQuaternion(worldRotation) ||
        glm::length(worldRotation) <= 0.0f) return false;
    return m_impl->world->SetKinematicTransform(handle, worldPosition, worldRotation);
}

bool PhysicsWorld::GetBodyTransform(PhysicsBodyHandle handle,
    glm::vec3& outWorldPosition,
    glm::quat& outWorldRotation) const {
    if (!m_impl) return false;
    return m_impl->world->GetBodyTransform(handle, outWorldPosition, outWorldRotation);
}

void PhysicsWorld::WakeUp(PhysicsBodyHandle handle) {
    if (!m_impl) return;
    m_impl->world->WakeUp(handle);
}

void PhysicsWorld::SetBodyEnabled(PhysicsBodyHandle handle, bool enabled) {
    if (!m_impl) return;
    m_impl->world->SetBodyEnabled(handle, enabled);
}

PhysicsBodyHandle PhysicsWorld::CreateBody(const RigidBodyDesc& desc,
    const glm::vec3& position, const glm::quat& rotation) {
    if (!m_impl) return {};
    return m_impl->world->CreateBody(desc, position, rotation);
}

void PhysicsWorld::DestroyBody(PhysicsBodyHandle handle) {
    if (!m_impl) return;
    m_impl->world->DestroyBody(handle);
}


} // namespace eng
