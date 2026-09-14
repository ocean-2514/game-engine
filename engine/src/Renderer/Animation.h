#ifndef O_ANIMATION
#define O_ANIMATION

#include <glm/glm.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>

#include <string>
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

struct AnimationPlayback {
    std::shared_ptr<const AnimationClip> clip;
    float time = 0.0f;
    float speed = 1.0f;
    bool looping = true;
    bool playing = false;

    void Reset() {
        clip.reset();
        time = 0.0f;
        speed = 1.0f;
        looping = true;
        playing = false;
    }

    void Update(float deltaTime) {
        if (!clip || !playing || deltaTime <= 0.0f) return;
        time += deltaTime * speed;
        const float duration = clip->duration;
        if (duration <= 0.0f) {
            time = 0.0f;
            playing = false;
        } else if (looping) {
            time = std::fmod(time, duration);
            if (time < 0.0f) time += duration;
        } else {
            if (time >= duration) {
                time = duration;
                playing = false;
            } else if (time <= 0.0f && speed < 0.0f) {
                time = 0.0f;
                playing = false;
            }
        }
    }

    void SetTime(float newTime) {
        if (!clip) return;
        const float duration = clip->duration;
        if (duration <= 0.0f) time = 0.0f;
        else if (looping) {
            time = std::fmod(newTime, duration);
            if (time < 0.0f) time += duration;
        }
        else time = glm::clamp(newTime, 0.0f, duration);
    }
};

struct AnimationTransition {
    AnimationPose sourcePose;
    AnimationPlayback destination;
    float elapsed = 0.0f;
    float duration = 0.2f;
    bool active = false;
};

} // namespace eng


#endif // O_ANIMATION
