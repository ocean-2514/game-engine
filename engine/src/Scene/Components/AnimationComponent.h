#ifndef O_ANIMATION_COMPONENT
#define O_ANIMATION_COMPONENT

#include "Renderer/Animation.h"
#include "Scene/Component.h"
#include "Renderer/AnimatorController.h"

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

    // const AnimationClip* GetCurrentClip() const;
    const std::shared_ptr<const Model>& GetModel() const;
    const std::shared_ptr<SkeletonPose>& GetPose() const;
    const std::shared_ptr<const AnimatorController>& GetController() const;
    bool SetController(std::shared_ptr<const AnimatorController> controller);
    bool IsLooping() const;
    void SetLooping(bool looping);
    bool IsPlaying() const;
    void SetPlaying(bool playing);
    float GetNormalizedTime() const;
    void SetNormalizedTime(float time);

    void Register(std::shared_ptr<const AnimationClip> clip);
    bool Play(const std::string& clipName, bool looping = true);
    bool CrossFade(const std::string& clipName, float duration = 0.3f, bool looping = true);

    bool SetFloat(std::string_view name, float value);
    bool SetBool(std::string_view name, bool value);
    bool SetInt(std::string_view name, int32_t value);
    bool SetTrigger(std::string_view name);
    bool ResetTrigger(std::string_view name);

    AnimatorStateId GetCurrentState() const;
    bool IsInTransition() const;

protected:
    void OnUpdate(float deltaTime) override;

private:
    bool SetValue(std::string_view name, AnimatorParameterValue value);
    bool PlayClip(const std::string& name, bool looping);
    bool PlayMotion(AnimatorMotionId motionId, bool looping);
    bool CrossFadeClip(const std::string& name, float duration, bool looping);
    bool CrossFadeMotion(AnimatorMotionId motionId, float duration, bool looping);
    bool AdvanceMotion(MotionPlayback& playback, float deltaTime);
    float GetEffectiveDuration(AnimatorMotionId motionId) const;
    void Renew();
    void BuildBindings();
    void ApplyPoseToBindings();
    void BuildSkinningPalette();
    void BuildSkinningPaletteRecursive(uint32_t nodeIndex,
        const glm::mat4& parentTransform);
    bool TryStartStateTransition();
    bool AreConditionsMet(
        const AnimatorTransitionDefinition& transition) const;
    bool IsConditionMet(const AnimatorCondition& condition) const;
    void ConsumeTriggersUsedBy(
        const AnimatorTransitionDefinition& transition);
    const TransformTrack* FindTrack(const AnimationClip* clip, 
        uint32_t nodeIndex) const;
    const AnimationClip* FindAnimationClip(const std::string& name) const;
    LocalTransform SampleLocalTransform(const AnimationClip* clip, 
        uint32_t nodeIndex, float time) const;
    AnimationPose SampleLocalPose(const AnimatorMotionData& motionData,
        float normalizedTime) const;
    AnimationPose SampleLocalPose(const ClipMotionDefinition& clipDef,
        float normalizedTime) const;
    AnimationPose SampleLocalPose(const BlendTree1DDefinition& blendTree,
        float normalizedTime) const;
    AnimationPose SampleLocalPose(const MotionPlayback& playback) const;
    AnimationPose SampleLocalPose(const AnimationBlendTransition& transition) const;

    bool ValidateControllerClipName(const AnimatorController& controller) const;
    bool ValidateMotionClipName(const AnimatorController& controller,
        const AnimatorMotionData& motion) const;
    bool ValidateClipMotionClipName(const ClipMotionDefinition& clipDef) const;
    bool ValidateBlendTree1DClipName(const AnimatorController& controller,
        const BlendTree1DDefinition& blendTreeDef) const;

    std::shared_ptr<const Model> m_model;
    std::shared_ptr<SkeletonPose> m_skeletonPose;
    MotionPlayback m_playback;
    AnimationBlendTransition m_transition;
    AnimationPose m_currentPose;

    std::shared_ptr<const AnimatorController> m_controller;
    std::vector<AnimatorParameterValue> m_parameterValues;
    AnimatorStateId m_currentStateId = InvalidAnimatorId;
    AnimatorStateId m_destinationStateId = InvalidAnimatorId;

    std::unordered_map<std::string,
        std::shared_ptr<const AnimationClip>> m_clips;
    std::unordered_map<GameObject*, std::unique_ptr<ObjectBinding>> m_bindings;
};

} // namespace eng

#endif // O_ANIMATION_COMPONENT
