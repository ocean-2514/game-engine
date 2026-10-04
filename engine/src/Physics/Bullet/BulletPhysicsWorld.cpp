#include "Physics/Bullet/BulletPhysicsWorld.h"
#include "Physics/Bullet/BulletConversions.h"

namespace eng
{
    
BulletPhysicsWorld::BulletPhysicsWorld(const PhysicsWorldDesc& desc) {
    m_collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collisionConfig.get());
    m_broadphase = std::make_unique<btDbvtBroadphase>();
    m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    m_world = std::make_unique<btDiscreteDynamicsWorld>(
        m_dispatcher.get(), m_broadphase.get(), m_solver.get(), m_collisionConfig.get()
    );

    m_world->setGravity(ToBtVector3(desc.gravity));
}

void BulletPhysicsWorld::Simulate(float fixedDeltaTime) {
    m_world->stepSimulation(fixedDeltaTime, 0, fixedDeltaTime);
}

void BulletPhysicsWorld::SetGravity(const glm::vec3& gravity) {
    m_world->setGravity(ToBtVector3(gravity));
}

glm::vec3 BulletPhysicsWorld::GetGravity() const {
    return ToGlmVec3(m_world->getGravity());
}




} // namespace eng
