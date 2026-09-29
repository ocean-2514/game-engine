#ifndef O_ANIMATION
#define O_ANIMATION

#include "Renderer/AnimatorTypes.h"

#include <glm/glm.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>

#include <string>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace eng
{
    
struct KeyFrameVec3 {
    float time = 0.0f;
    glm::vec3 value{0.0f};
};

struct KeyFrameQuat {
    float time = 0.0f;
    glm::quat value{1.0f, 0.0f, 0.0f, 0.0f};
};

struct TransformTrack {
    static constexpr uint32_t InvalidNodeIndex =
        std::numeric_limits<uint32_t>::max();

    // Name of the target node in the model hierarchy, e.g. "leftHand".
    // This is used to match the track to the corresponding node in the model.
    std::string targetName;
    // corresponding model node index
    uint32_t targetNodeIndex = InvalidNodeIndex;
    // key frames in local(bone) coordinate system
    std::vector<KeyFrameVec3> positions;
    // key frames in local(bone) coordinate system
    std::vector<KeyFrameVec3> scales;
    // key frames in local(bone) coordinate system
    std::vector<KeyFrameQuat> rotations;
};

struct AnimationClip {
    std::string name;
    float duration = 0.0f;
    bool looping = true;
    std::vector<TransformTrack> tracks;
};

// Mutable, per-model-instance output of animation evaluation. Model and Mesh
// remain immutable and can therefore be shared by multiple instances.
struct SkeletonPose {
    std::vector<glm::mat4> skinMatrices;
};

struct AnimationLimits {
    // Must match the shader constant. This value exceeds the OpenGL 3.3
    // minimum vertex-uniform guarantee, so the backend should eventually
    // validate the device limit or move the palette into a buffer resource.
    static constexpr uint32_t MaxBones = 150;
};

struct LocalTransform {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    glm::mat4 ToMatrix() const {
        return glm::translate(glm::mat4{1.0f}, translation) *
            glm::toMat4(glm::normalize(rotation)) *
            glm::scale(glm::mat4{1.0f}, scale);
    }
};

struct AnimationPose {
    // corresponding model node index
    std::vector<LocalTransform> localTransforms;
};

struct MotionPlayback {
    AnimatorMotionId motionId = InvalidAnimatorId;
    float normalizedTime = 0.0f;
    float speed = 1.0f;
    bool looping = true;
    bool playing = false;

    void Reset() {
        motionId = InvalidAnimatorId;
        normalizedTime = 0.0f;
        speed = 1.0f;
        looping = true;
        playing = false;
    }


    void SetNormalizedTime(float newTime) {
        if (!std::isfinite(newTime)) return;
        if (looping) {
            normalizedTime = std::fmod(newTime, 1.0f);
            if (normalizedTime < 0.0f) normalizedTime += 1.0f;
        }
        else normalizedTime = glm::clamp(newTime, 0.0f, 1.0f);
    }
};

struct AnimationBlendTransition {
    AnimationPose sourcePose;
    MotionPlayback destination;
    float elapsed = 0.0f;
    float duration = 0.2f;
    bool active = false;
};

} // namespace eng


#endif // O_ANIMATION
