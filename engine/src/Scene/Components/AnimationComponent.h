#ifndef O_ANIMATION_COMPONENT
#define O_ANIMATION_COMPONENT

#include "Renderer/Animation.h"
#include "Scene/Component.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace eng {

class GameObject;
class Model;

struct ObjectBinding {
    uint32_t nodeIndex = 0;
    GameObject* object = nullptr;
};

class AnimationComponent : public Component {
public:
    ENG_COMPONENT_TYPE(AnimationComponent);

    explicit AnimationComponent(std::shared_ptr<const Model> model);

    const AnimationClip* GetCurrentClip() const;
    const std::shared_ptr<const Model>& GetModel() const;
    const std::shared_ptr<SkeletonPose>& GetPose() const;
    bool IsLooping() const;
    void SetLooping(bool looping);
    bool IsPlaying() const;
    void SetPlaying(bool playing);
    float GetTime() const;
    void SetTime(float time);
    void Register(std::shared_ptr<const AnimationClip> clip);
    bool Play(const std::string& name, bool looping = true);
    bool CrossFade(const std::string& name, float duration = 0.3f, bool looping = true);

protected:
    void OnUpdate(float deltaTime) override;

private:
    void Renew();
    void BuildBindings();
    void ApplyPoseToBindings();
    void BuildSkinningPalette();
    void BuildSkinningPaletteRecursive(uint32_t nodeIndex,
        const glm::mat4& parentTransform);
    const TransformTrack* FindTrack(const AnimationClip* clip, 
        uint32_t nodeIndex) const;
    LocalTransform SampleLocalTransform(const AnimationClip* clip, 
        uint32_t nodeIndex, float time) const;
    AnimationPose SampleLocalPose(const AnimationClip* clip, float time) const;
    AnimationPose SampleLocalPose(const AnimationPlayback& playback) const;
    AnimationPose SampleLocalPose(const AnimationTransition& transition) const;
    static glm::vec3 Interpolate(
        const std::vector<KeyFrameVec3>& keyFrames, float time);
    static glm::quat Interpolate(
        const std::vector<KeyFrameQuat>& keyFrames, float time);

    std::shared_ptr<const Model> m_model;
    std::shared_ptr<SkeletonPose> m_skeletonPose;
    AnimationPlayback m_playback;
    AnimationTransition m_transition;
    AnimationPose m_currentPose;

    std::unordered_map<std::string,
        std::shared_ptr<const AnimationClip>> m_clips;
    std::unordered_map<GameObject*, std::unique_ptr<ObjectBinding>> m_bindings;
};

} // namespace eng

#endif // O_ANIMATION_COMPONENT
