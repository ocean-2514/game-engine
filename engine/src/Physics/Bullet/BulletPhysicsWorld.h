#ifndef O_BULLET_PHYSICS_WORLD
#define O_BULLET_PHYSICS_WORLD

#include "Physics/PhysicsTypes.h"

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <memory>
#include <vector>
#include <unordered_map>

namespace eng
{
    
class BulletPhysicsWorld
{
public:
    explicit BulletPhysicsWorld(const PhysicsWorldDesc& desc);
    ~BulletPhysicsWorld();
    
    void Simulate(float fixedDeltaTime);
    void SetGravity(const glm::vec3& gravity);
    glm::vec3 GetGravity() const;

    PhysicsBodyHandle CreateBody(const RigidBodyDesc& desc,
        const glm::vec3& position, const glm::quat& rotation);
    void DestroyBody(PhysicsBodyHandle handle);

    void AddForce(PhysicsBodyHandle handle, const glm::vec3& force);
    void AddImpulse(PhysicsBodyHandle handle, const glm::vec3& impulse);
    void AddTorque(PhysicsBodyHandle handle, const glm::vec3& torque);

    void SetLinearVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity);
    glm::vec3 GetLinearVelocity(PhysicsBodyHandle handle) const;
    void SetAngularVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity);
    glm::vec3 GetAngularVelocity(PhysicsBodyHandle handle) const;

    bool Teleport(PhysicsBodyHandle handle,
        const glm::vec3& worldPosition,
        const glm::quat& worldRotation,
        bool clearVelocity = false);
    bool SetKinematicTransform(PhysicsBodyHandle handle,
        const glm::vec3& worldPosition,
        const glm::quat& worldRotation);
    bool GetBodyTransform(PhysicsBodyHandle handle,
        glm::vec3& outWorldPosition,
        glm::quat& outWorldRotation) const;

    void WakeUp(PhysicsBodyHandle handle);
    void SetBodyEnabled(PhysicsBodyHandle handle, bool enabled);

private:
    struct BodySlot {
        uint32_t generation = 1;
        bool alive = false;
        bool enabled = false;
        int16_t collisionLayer = 1;
        int16_t collisionMask = -1;
        glm::vec3 gravityFactor{1.0f};
        // Declaration order matters: reverse destruction must release body,
        // motion state and shapes before triangle mesh backing storage.
        std::vector<std::unique_ptr<btStridingMeshInterface>> meshInterfaces;
        std::vector<std::unique_ptr<btCollisionShape>> shapes;
        std::unique_ptr<btMotionState> motionState;
        std::unique_ptr<btRigidBody> body;
    };

    btCollisionShape* BuildShape(const RigidBodyDesc& desc, BodySlot& slot);
    btRigidBody* GetBody(PhysicsBodyHandle handle) const;

    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfig;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_world;

    std::vector<BodySlot> m_slots;
    std::vector<uint32_t> m_freeIndices;
};


} // namespace eng


#endif // O_BULLET_PHYSICS_WORLD
