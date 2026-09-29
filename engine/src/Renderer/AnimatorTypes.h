#ifndef O_ANIMATOR_TYPES
#define O_ANIMATOR_TYPES

#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include <limits>

namespace eng
{
    
using AnimatorParameterId = uint32_t;
using AnimatorStateId = uint32_t;
using AnimatorMotionId = uint32_t;

inline constexpr uint32_t InvalidAnimatorId = 
    std::numeric_limits<uint32_t>::max();

enum class AnimatorParameterType {
    Float, Bool, Int, Trigger
};

using AnimatorParameterValue = std::variant<float, bool, int32_t>;

struct AnimatorParameterDefinition {
    std::string name;
    AnimatorParameterType type = AnimatorParameterType::Float;
    AnimatorParameterValue defaultValue = 0.0f;
};

struct AnimationStateDefinition {
    std::string name;
    AnimatorMotionId motionId = InvalidAnimatorId;
    float speed = 1.0f;
    bool looping = true;
};

enum class AnimatorConditionOp {
    IsTrue, IsFalse, 
    Greater, GreaterEqual, Less, LessEqual, Equal, NotEqual,
    IsTriggered
};

struct AnimatorCondition {
    AnimatorParameterId parameterId = InvalidAnimatorId;
    AnimatorConditionOp op = AnimatorConditionOp::IsTrue;
    AnimatorParameterValue threshold = false;
};

struct AnimatorTransitionDefinition {
    AnimatorStateId sourceState = InvalidAnimatorId;
    AnimatorStateId destinationState = InvalidAnimatorId;

    float blendDuration = 0.3f;
    bool hasExitTime = false;
    float exitTimeNormalized = 1.0f;
    float destinationStartNormalized = 0.0f;

    std::vector<AnimatorCondition> conditions;
};

struct ClipMotionDefinition {
    std::string clipName;
};

struct BlendTree1DChild {
    float threshold = 0.0f;
    AnimatorMotionId motionId = InvalidAnimatorId;
};

struct BlendTree1DDefinition {
    AnimatorParameterId parameterId = InvalidAnimatorId;
    std::vector<BlendTree1DChild> children;
};

using AnimatorMotionData = std::variant<
    ClipMotionDefinition, BlendTree1DDefinition>; 

struct AnimatorMotionDefinition {
    std::string name;
    AnimatorMotionData data;
};

} // namespace eng


#endif // O_ANIMATOR_TYPES
