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

struct RaycastDesc {
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
    float maxDistance = 1000.0f;
    uint16_t collisionMask = 0xFFFF;
    bool includeTriggers = false;
};

struct RaycastHit {
    PhysicsBodyHandle body{};
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    float distance = 0.0f;
    float fraction = 0.0f;
};

struct ContactPairKey {
    PhysicsBodyHandle first;
    PhysicsBodyHandle second;

    bool operator==(const ContactPairKey& other) const {
        return (first.index == other.first.index &&
                first.generation == other.first.generation &&
                second.index == other.second.index &&
                second.generation == other.second.generation) ||
               (first.index == other.second.index &&
                first.generation == other.second.generation &&
                second.index == other.first.index &&
                second.generation == other.first.generation);
    }
};

struct ContactPairKeyHash {
    std::size_t operator()(const ContactPairKey& key) const {
        auto& min = key.first.index < key.second.index ? key.first : key.second;
        auto& max = key.first.index < key.second.index ? key.second : key.first;
        std::size_t h1 = std::hash<uint32_t>()(min.index);
        std::size_t h2 = std::hash<uint32_t>()(min.generation);
        std::size_t h3 = std::hash<uint32_t>()(max.index);
        std::size_t h4 = std::hash<uint32_t>()(max.generation);

        // Combine the hashes using a simple method
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

struct ContactPoint {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
    float penetration = 0.0f;
    float impulse = 0.0f;
};

enum class PhysicsEventPhase {
    Enter, Stay, Exit
};

struct PhysicsEvent {
    ContactPairKey key;
    PhysicsEventPhase phase;
    bool trigger = false;
    std::vector<ContactPoint> points;
};

} // namespace eng


#endif // O_PHYSICS_TYPES
