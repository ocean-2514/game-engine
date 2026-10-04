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


} // namespace eng
