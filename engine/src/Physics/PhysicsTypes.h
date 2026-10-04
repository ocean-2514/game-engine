#ifndef O_PHYSICS_TYPES
#define O_PHYSICS_TYPES

#include <glm/glm.hpp>
#include <cstdint>
#include <cmath>

namespace eng
{

inline bool IsFiniteVector(const glm::vec3& v) noexcept {
    return std::isfinite(v.x) && 
        std::isfinite(v.y) && std::isfinite(v.z);
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


} // namespace eng


#endif // O_PHYSICS_TYPES
