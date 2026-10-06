#ifndef O_PHYSICS_WORLD
#define O_PHYSICS_WORLD

#include "Physics/PhysicsTypes.h"

#include <memory>

namespace eng
{
    
class PhysicsWorld
{
public:
    explicit PhysicsWorld(const PhysicsWorldDesc& desc);
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld& other) = delete;
    PhysicsWorld& operator=(const PhysicsWorld& other) = delete;
    PhysicsWorld(PhysicsWorld&& other) = delete;
    PhysicsWorld& operator=(PhysicsWorld&& other) = delete;

    void Simulate(float fixedDeltaTime);
    void UpdateCollisionEvents();
    void SetGravity(const glm::vec3& gravity);
    glm::vec3 GetGravity() const;
    const PhysicsWorldDesc& GetDesc() const;
    const std::vector<PhysicsEvent>& GetCollisionEvents() const;
    void ClearCollisionEvents();
    bool IsValid() const;

    void AddForce(PhysicsBodyHandle handle, const glm::vec3& force);
    void AddImpulse(PhysicsBodyHandle handle, const glm::vec3& impulse);
    void AddTorque(PhysicsBodyHandle handle, const glm::vec3& torque);

    void SetLinearVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity);
    glm::vec3 GetLinearVelocity(PhysicsBodyHandle handle) const;
    void SetAngularVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity);
    glm::vec3 GetAngularVelocity(PhysicsBodyHandle handle) const;
    void SetLocalInertia(PhysicsBodyHandle handle, const glm::vec3& inertia);
    void SetAngularFactor(PhysicsBodyHandle handle, const glm::vec3& factor);
    void SetWorldRotation(PhysicsBodyHandle handle, const glm::quat& rotation);
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
    bool GetInterpolatedBodyTransform(PhysicsBodyHandle handle,
        float alpha,
        glm::vec3& outWorldPosition,
        glm::quat& outWorldRotation) const;
    bool RaycastClosest(const RaycastDesc& query,
        RaycastHit& hit) const;

    void WakeUp(PhysicsBodyHandle handle);
    void SetBodyEnabled(PhysicsBodyHandle handle, bool enabled);

    PhysicsBodyHandle CreateBody(const RigidBodyDesc& desc,
        const glm::vec3& position, const glm::quat& rotation);
    void DestroyBody(PhysicsBodyHandle handle);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    PhysicsWorldDesc m_desc;
};

    
} // namespace eng


#endif // O_PHYSICS_WORLD
