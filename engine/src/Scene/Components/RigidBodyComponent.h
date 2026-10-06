#ifndef O_RIGIDBODY_COMPONENT
#define O_RIGIDBODY_COMPONENT

#include "Scene/Component.h"
#include "Physics/PhysicsTypes.h"
#include "Physics/CollisionShape.h"

namespace eng
{

class PhysicsWorld;
    
class RigidBodyComponent : public Component
{
public:
    ENG_COMPONENT_TYPE(RigidBodyComponent);

    explicit RigidBodyComponent(const RigidBodyDesc& desc);

    void AddForce(const glm::vec3& force);
    void AddImpulse(const glm::vec3& impulse);
    void AddTorque(const glm::vec3& torque);
    void SetLinearVelocity(const glm::vec3& velocity);
    glm::vec3 GetLinearVelocity() const;
    void SetAngularVelocity(const glm::vec3& velocity);
    glm::vec3 GetAngularVelocity() const;
    void SetLocalInertia(const glm::vec3& inertia);
    void SetAngularFactor(const glm::vec3& factor);
    void SetWorldRotation(const glm::quat& rotation);
    void Teleport(const glm::vec3& worldPosition,
        const glm::quat& worldRotation,
        bool clearVelocity = false);
    void WakeUp();
    void SetEnabled(bool enabled);

    void PushKinematicTransform();
    void PullDynamicTransform();
    bool ApplyRenderInterpolation(float alpha);
    void RestoreSimulationTransform();

    const PhysicsWorld* GetWorld() const;

protected:
    void OnAttach(Scene& scene) override;
    void OnDetach(Scene& scene) override;

private:
    PhysicsWorld* m_world = nullptr;
    PhysicsBodyHandle m_body;
    RigidBodyDesc m_desc;
    glm::vec3 m_savedWorldPosition{0.0f};
    glm::quat m_savedWorldRotation{1.0f, 0.0f, 0.0f, 0.0f};
    bool m_hasRenderOverride = false;
};



} // namespace eng


#endif // O_RIGIDBODY_COMPONENT
