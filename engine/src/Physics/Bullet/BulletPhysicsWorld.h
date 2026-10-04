#ifndef O_BULLET_PHYSICS_WORLD
#define O_BULLET_PHYSICS_WORLD

#include "Physics/PhysicsTypes.h"

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <memory>

namespace eng
{
    
class BulletPhysicsWorld
{
public:
    explicit BulletPhysicsWorld(const PhysicsWorldDesc& desc);
    
    void Simulate(float fixedDeltaTime);
    void SetGravity(const glm::vec3& gravity);
    glm::vec3 GetGravity() const;

private:
    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfig;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_world;
};


} // namespace eng


#endif // O_BULLET_PHYSICS_WORLD