#ifndef O_PHYSICS_TYPES
#define O_PHYSICS_TYPES

#include "Physics/CollisionShape.h"

#include <glm/glm.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>
#include <cstdint>
#include <cmath>
#include <variant>
#include <vector>
#include <limits>

namespace eng
{

static constexpr uint32_t InvalidIndex = std::numeric_limits<uint32_t>::max();

inline bool IsFiniteVector(const glm::vec3& v) noexcept {
    return std::isfinite(v.x) && 
        std::isfinite(v.y) && std::isfinite(v.z);
}

inline bool IsFiniteQuaternion(const glm::quat& q) noexcept {
    return std::isfinite(q.w) && std::isfinite(q.x) &&
        std::isfinite(q.y) && std::isfinite(q.z);
}
    
struct PhysicsWorldDesc {
    glm::vec3 gravity{0.0f, -9.81f, 0.0f};
    float fixedTimeStep = 1.0f / 60.0f;
    uint32_t maxSubStepsPerFrame = 8;
    float maxAccumulatedTime = 0.25f;

    bool IsValid() const noexcept {
        return IsFiniteVector(gravity) && std::isfinite(fixedTimeStep)
            && std::isfinite(maxAccumulatedTime) &&
            fixedTimeStep > 0.0f && maxSubStepsPerFrame > 0 &&
            maxAccumulatedTime >= fixedTimeStep;
    }
};

enum class BodyMotionType {
    Static,
    Kinematic,
    Dynamic
};

struct ColliderDesc {
    CollisionShapeDesc shape;
    glm::vec3 localPosition{0.0f};
    glm::quat localRotation{1.0f, 0.0f, 0.0f, 0.0f};
    bool isTrigger = false;
};

struct RigidBodyDesc {
    BodyMotionType motionType = BodyMotionType::Static;
    float mass = 0.0f;
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;
    float friction = 0.5f;
    float restitution = 0.0f;
    glm::vec3 gravityFactor{1.0f};
    uint16_t collisionLayer = 1;
    uint16_t collisionMask = 0xFFFF;
    bool useContinuousCollisionDetection = false;
    std::vector<ColliderDesc> colliders;
};

struct PhysicsBodyHandle {
    uint32_t index = InvalidIndex;
    uint32_t generation = 0;

    bool IsValid() const { return index != InvalidIndex; }
};

} // namespace eng


#endif // O_PHYSICS_TYPES
