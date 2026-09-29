#include "Scene/Components/AnimationComponent.h"
#include "Renderer/Model.h"
#include "Scene/GameObject.h"

#include <iostream>
#include <algorithm>
#include <cmath>
#include <type_traits>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace eng {

namespace  {

glm::vec3 Interpolate(
    const std::vector<KeyFrameVec3>& keys, float time) {
    if (keys.empty()) return glm::vec3{0.0f};
    if (keys.size() == 1 || time <= keys.front().time) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;
    std::size_t end = 1;
    while (end < keys.size() && time > keys[end].time) ++end;
    const std::size_t start = end - 1;
    const float span = keys[end].time - keys[start].time;
    return span <= 0.0f ? keys[start].value :
        glm::mix(keys[start].value, keys[end].value,
            (time - keys[start].time) / span);
}

glm::quat Interpolate(
    const std::vector<KeyFrameQuat>& keys, float time) {
    if (keys.empty()) return glm::quat{1.0f, 0.0f, 0.0f, 0.0f};
    if (keys.size() == 1 || time <= keys.front().time) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;
    std::size_t end = 1;
    while (end < keys.size() && time > keys[end].time) ++end;
    const std::size_t start = end - 1;
    const float span = keys[end].time - keys[start].time;
    return span <= 0.0f ? keys[start].value :
        glm::normalize(glm::slerp(keys[start].value, keys[end].value,
            (time - keys[start].time) / span));
}

AnimationPose Mix(const AnimationPose& start,
    const AnimationPose& end, float a) {
    if (start.localTransforms.size() != end.localTransforms.size()) {
        return AnimationPose{};
    }
    AnimationPose pose;
    pose.localTransforms.resize(start.localTransforms.size());
    for (std::size_t i = 0; i < pose.localTransforms.size(); ++i) {
        pose.localTransforms[i].translation =
            glm::mix(start.localTransforms[i].translation, end.localTransforms[i].translation, a);
        pose.localTransforms[i].scale =
            glm::mix(start.localTransforms[i].scale, end.localTransforms[i].scale, a);
        pose.localTransforms[i].rotation = glm::normalize(
            glm::slerp(start.localTransforms[i].rotation,
                end.localTransforms[i].rotation, a));
    }
    return pose;
}

} // namespace


AnimationComponent::AnimationComponent(std::shared_ptr<const Model> model)
    : m_model(std::move(model)), m_skeletonPose(std::make_shared<SkeletonPose>()) {
    if (!m_model) return;
    m_skeletonPose->skinMatrices.resize(m_model->GetBones().size(), glm::mat4{1.0f});
    auto defaultController = std::make_shared<AnimatorController>();
    for (const auto& clip : m_model->GetAnimationClips()) {
        Register(clip);
        defaultController->AddClipMotion(clip->name, clip->name);
    }
    m_controller = std::move(defaultController);
    if (m_controller->GetMotions().size() > 0) {
        m_playback.motionId = 0;
        const auto& motionDef = m_controller->GetMotion(0);
        std::visit([this](const auto& value) {
            using ValueType = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<ValueType, ClipMotionDefinition>) {
                const AnimationClip* clip = FindAnimationClip(value.clipName);
                m_playback.looping = clip->looping;
            } else if constexpr (std::is_same_v<ValueType, BlendTree1DDefinition>) {
                m_playback.looping = true;
            }
        }, motionDef->data);
    }
    m_currentPose = SampleLocalPose(m_playback);
    BuildSkinningPalette();
}

// const AnimationClip* AnimationComponent::GetCurrentClip() const {
//     if (m_transition.active) return m_transition.destination.clip.get();
//     else return m_playback.clip.get();
// }

const std::shared_ptr<const Model>& AnimationComponent::GetModel() const {
    return m_model;
}

const std::shared_ptr<SkeletonPose>& AnimationComponent::GetPose() const {
    return m_skeletonPose;
}

const std::shared_ptr<const AnimatorController>&
    AnimationComponent::GetController() const {
    return m_controller;
}

bool AnimationComponent::SetController(
    std::shared_ptr<const AnimatorController> controller) {
    if (!controller) {
        m_controller.reset();
        m_parameterValues.clear();
        m_currentStateId = InvalidAnimatorId;
        m_destinationStateId = InvalidAnimatorId;
        m_transition.active = false;
        m_playback.Reset();
        return true;
    }
    std::string error;
    if (!controller->Validate(&error)) {
        std::cout << "AnimationComponent::SetController: invalid controller: "
                  << error << '\n';
        return false;
    }
    if (!ValidateControllerClipName(*controller)) {
        std::cout << "AnimationComponent::SetController: controller references "
            "a missing or invalid clip\n";
        return false;
    }

    const AnimatorStateId defaultState = controller->GetDefaultStateId();
    const auto* state = controller->GetState(defaultState);
    if (!state) return false;

    std::vector<AnimatorParameterValue> values;
    const auto& parameters = controller->GetParameters();
    values.resize(parameters.size());
    for (std::size_t i = 0; i < parameters.size(); ++i) {
        values[i] = parameters[i].defaultValue;
    }

    m_controller = std::move(controller);
    m_parameterValues = std::move(values);
    m_currentStateId = defaultState;
    m_destinationStateId = InvalidAnimatorId;
    m_transition.active = false;
    m_playback.Reset();
    m_playback.motionId = state->motionId;
    m_playback.speed = state->speed;
    m_playback.looping = state->looping;
    m_playback.playing = true;
    BuildBindings();
    Renew();
    return true;
}

bool AnimationComponent::IsLooping() const { 
    if (m_transition.active) return m_transition.destination.looping;
    else return m_playback.looping;
}

void AnimationComponent::SetLooping(bool looping) { 
    if (m_transition.active) m_transition.destination.looping = looping;
    else m_playback.looping = looping;
}

bool AnimationComponent::IsPlaying() const { 
    return m_playback.playing;
}

void AnimationComponent::SetPlaying(bool playing) { 
    if (m_transition.active) {
        m_playback.playing = playing;
        m_transition.destination.playing = playing;
    }
    else m_playback.playing = playing;
}

float AnimationComponent::GetNormalizedTime() const {
    if (m_transition.active) return m_transition.destination.normalizedTime;
    else return m_playback.normalizedTime;
}

void AnimationComponent::SetNormalizedTime(float time) {
    if (!std::isfinite(time)) return;
    if (m_transition.active) {
        m_transition.destination.SetNormalizedTime(time);
    } else {
        m_playback.SetNormalizedTime(time);
    }

    Renew();
}

void AnimationComponent::Register(std::shared_ptr<const AnimationClip> clip) {
    if (!clip || clip->name.empty()) return;
    m_clips.insert_or_assign(clip->name, std::move(clip));
}

// cancels any ongoing transition and starts playing the specified clip immediately
bool AnimationComponent::Play(const std::string& name, bool looping) {
    if (!PlayClip(name, looping)) return false;
    m_currentStateId = InvalidAnimatorId;
    m_destinationStateId = InvalidAnimatorId;
    return true;
}

bool AnimationComponent::PlayClip(const std::string& name, bool looping) {
    const auto* clip = FindAnimationClip(name);
    if (clip == nullptr) {
        std::cout << "AnimationComponent::Play: clip not found: " << name << std::endl;
        return false;
    }

    if (!m_controller) return false;
    AnimatorMotionId motionId = m_controller->FindClipMotionByClipName(name);
    return PlayMotion(motionId, looping);
}

bool AnimationComponent::PlayMotion(
    AnimatorMotionId motionId, bool looping) {
    if (!m_controller || !m_controller->GetMotion(motionId)) return false;

    m_transition.active = false;
    m_destinationStateId = InvalidAnimatorId;

    m_playback.motionId = motionId;
    m_playback.normalizedTime = 0.0f;
    m_playback.speed = 1.0f;
    m_playback.looping = looping;
    m_playback.playing = true;
    BuildBindings();
    Renew();
    return true;
}

bool AnimationComponent::CrossFade(const std::string& name, 
    float duration, bool looping) {
    if (!CrossFadeClip(name, duration, looping)) return false;
    m_currentStateId = InvalidAnimatorId;
    m_destinationStateId = InvalidAnimatorId;
    return true;
}

bool AnimationComponent::CrossFadeClip(const std::string& name,
    float duration, bool looping) {
    if (!std::isfinite(duration)) return false;
    if (duration <= 0.0f) return PlayClip(name, looping);

    const auto* clip = FindAnimationClip(name);
    if (clip == nullptr) {
        std::cout << "AnimationComponent::CrossFade: clip not found: " << name << std::endl;
        return false;
    }
    if (!m_controller) return false;
    AnimatorMotionId motionId = m_controller->FindClipMotionByClipName(name);
    return CrossFadeMotion(motionId, duration, looping);
}

bool AnimationComponent::CrossFadeMotion(
    AnimatorMotionId motionId, float duration, bool looping) {
    if (!m_controller || !m_controller->GetMotion(motionId) ||
        !std::isfinite(duration)) return false;
    if (duration <= 0.0f) return PlayMotion(motionId, looping);

    if (m_transition.active &&
        m_transition.destination.motionId == motionId) {
        m_playback.playing = true;
        m_transition.destination.looping = looping;
        m_transition.destination.playing = true;
        return true;
    }
    if (!m_transition.active && m_playback.motionId == motionId) {
        m_playback.looping = looping;
        m_playback.playing = true;
        return true;
    }

    m_transition.active = true;
    m_destinationStateId = InvalidAnimatorId;
    m_playback.playing = true;
    m_transition.sourcePose = m_currentPose;
    m_transition.destination.Reset();
    m_transition.destination.motionId = motionId;
    m_transition.destination.normalizedTime = 0.0f;
    m_transition.destination.looping = looping;
    m_transition.destination.playing = true;
    m_transition.elapsed = 0.0f;
    m_transition.duration = duration;
    return true;
}

bool AnimationComponent::AdvanceMotion(
    MotionPlayback& playback, float deltaTime) {
    if (!playback.playing) return true;
    if (!std::isfinite(deltaTime) || deltaTime <= 0.0f ||
        !std::isfinite(playback.speed)) {
        return false;
    }

    const float duration = GetEffectiveDuration(playback.motionId);
    if (!std::isfinite(duration) || duration <= 0.0f) {
        playback.playing = false;
        return false;
    }

    const float nextTime = playback.normalizedTime +
        deltaTime * playback.speed / duration;
    if (!std::isfinite(nextTime)) {
        playback.playing = false;
        return false;
    }
    playback.SetNormalizedTime(nextTime);
    if (!playback.looping &&
        ((playback.speed >= 0.0f && nextTime >= 1.0f) ||
         (playback.speed < 0.0f && nextTime <= 0.0f))) {
        playback.playing = false;
    }
    return true;
}

float AnimationComponent::GetEffectiveDuration(AnimatorMotionId motionId) const {
    if (!m_controller) return 0.0f;
    const auto* motion = m_controller->GetMotion(motionId);
    if (!motion) return 0.0f;

    if (const auto* clipMotion =
            std::get_if<ClipMotionDefinition>(&motion->data)) {
        const auto* clip = FindAnimationClip(clipMotion->clipName);
        return clip ? clip->duration : 0.0f;
    }

    const auto& tree = std::get<BlendTree1DDefinition>(motion->data);
    if (tree.children.empty() || tree.parameterId >= m_parameterValues.size() ||
        !std::holds_alternative<float>(m_parameterValues[tree.parameterId])) {
        return 0.0f;
    }
    const float value = std::get<float>(m_parameterValues[tree.parameterId]);
    if (!std::isfinite(value)) return 0.0f;

    auto childDuration = [this](const BlendTree1DChild& child) {
        return GetEffectiveDuration(child.motionId);
    };

    const auto& children = tree.children;
    if (children.size() == 1 || value <= children.front().threshold)
        return childDuration(children.front());
    if (value >= children.back().threshold)
        return childDuration(children.back());

    const auto right = std::lower_bound(children.begin() + 1, children.end(), value,
        [](const BlendTree1DChild& child, float input) {
            return child.threshold < input;
        });
    if (value == right->threshold) return childDuration(*right);
    const auto left = right - 1;
    const float span = right->threshold - left->threshold;
    if (span <= 0.0f) return 0.0f;
    const float weight = glm::clamp(
        (value - left->threshold) / span, 0.0f, 1.0f);
    const float leftDuration = childDuration(*left);
    const float rightDuration = childDuration(*right);
    if (leftDuration <= 0.0f || rightDuration <= 0.0f) return 0.0f;
    return glm::mix(leftDuration, rightDuration, weight);
}

bool AnimationComponent::SetValue(
    std::string_view name, AnimatorParameterValue value) {
    if (!m_controller) return false;
    AnimatorParameterId id = m_controller->FindParameter(name);
    if (id == InvalidAnimatorId) {
        std::cout << "AnimationComponent::SetValue: Cannot find parameter named"
            << name << std::endl;
        return false;
    }
    if (id >= m_parameterValues.size()) {
        std::cout << "AnimationComponent::SetValue: Parameter index out of bounds"
            << std::endl;
        return false;
    }
    const auto* definition = m_controller->GetParameter(id);
    if (!definition) return false;
    const bool typeMatches =
        (definition->type == AnimatorParameterType::Float &&
            std::holds_alternative<float>(value)) ||
        (definition->type == AnimatorParameterType::Int &&
            std::holds_alternative<int32_t>(value)) ||
        ((definition->type == AnimatorParameterType::Bool ||
          definition->type == AnimatorParameterType::Trigger) &&
            std::holds_alternative<bool>(value));
    if (!typeMatches) return false;
    m_parameterValues[id] = value;
    return true;
}

bool AnimationComponent::SetFloat(std::string_view name, float value) {
    if (!m_controller) return false;
    const auto* parameter = m_controller->GetParameter(
        m_controller->FindParameter(name));
    if (!parameter || parameter->type != AnimatorParameterType::Float ||
        !std::isfinite(value)) return false;
    return SetValue(name, value);
}

bool AnimationComponent::SetBool(std::string_view name, bool value) {
    if (!m_controller) return false;
    const auto* parameter = m_controller->GetParameter(
        m_controller->FindParameter(name));
    if (!parameter || parameter->type != AnimatorParameterType::Bool) return false;
    return SetValue(name, value);
}

bool AnimationComponent::SetInt(std::string_view name, int32_t value) {
    if (!m_controller) return false;
    const auto* parameter = m_controller->GetParameter(
        m_controller->FindParameter(name));
    if (!parameter || parameter->type != AnimatorParameterType::Int) return false;
    return SetValue(name, value);
}

bool AnimationComponent::SetTrigger(std::string_view name) {
    if (!m_controller) return false;
    const auto* parameter = m_controller->GetParameter(
        m_controller->FindParameter(name));
    if (!parameter || parameter->type != AnimatorParameterType::Trigger) return false;
    return SetValue(name, true);
}

bool AnimationComponent::ResetTrigger(std::string_view name) {
    if (!m_controller) return false;
    const auto* parameter = m_controller->GetParameter(
        m_controller->FindParameter(name));
    if (!parameter || parameter->type != AnimatorParameterType::Trigger) return false;
    return SetValue(name, false);
}


AnimatorStateId AnimationComponent::GetCurrentState() const {
    return m_currentStateId;
}

bool AnimationComponent::IsInTransition() const {
    return m_transition.active;
}

void AnimationComponent::OnUpdate(float deltaTime) {
    if (!std::isfinite(deltaTime) || deltaTime <= 0.0f) return;
    if (m_bindings.empty()) BuildBindings();
    // update playback and transition
    if (IsInTransition()) {
        if (m_playback.playing) {
            m_transition.elapsed += deltaTime;
            AdvanceMotion(m_transition.destination, deltaTime);
        }
        if (m_transition.elapsed >= m_transition.duration) {
            m_playback = m_transition.destination;
            BuildBindings();
            m_transition.active = false;
            if (m_destinationStateId != InvalidAnimatorId) {
                m_currentStateId = m_destinationStateId;
                m_destinationStateId = InvalidAnimatorId;
            }
        }
    } else if (m_playback.playing) {
        AdvanceMotion(m_playback, deltaTime);
    }

    if (m_controller && !IsInTransition()) {
        TryStartStateTransition();
    }

    Renew();
}

void AnimationComponent::Renew() {
    if (!m_model) return;

    if (m_transition.active) {
        m_currentPose = SampleLocalPose(m_transition);
    } else {
        m_currentPose = SampleLocalPose(m_playback);
    }

    BuildSkinningPalette();
    ApplyPoseToBindings();
}

void AnimationComponent::BuildBindings() {
    m_bindings.clear();
    if (!m_model || !m_owner) return;
    const auto& nodes = m_model->GetNodes();
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        GameObject* object = nullptr;
        for (std::size_t childIndex = 0;
             childIndex < m_owner->GetChildCount(); ++childIndex) {
            GameObject* child = m_owner->GetChild(childIndex);
            object = child ? child->GetChildByName(nodes[i].name) : nullptr;
            if (object) break;
        }
        if (!object || !object->IsAlive()) continue;
        auto& binding = m_bindings[object];
        if (!binding) {
            binding = std::make_unique<ObjectBinding>(ObjectBinding{
                static_cast<uint32_t>(i), object});
        }
    }
}

void AnimationComponent::ApplyPoseToBindings() {
    for (const auto& [object, binding] : m_bindings) {
        if (!object || !object->IsAlive()) continue;
        if (binding->nodeIndex >= m_currentPose.localTransforms.size()) continue;
        object->SetLocalTransform(
            m_currentPose.localTransforms[binding->nodeIndex].ToMatrix());
    }
}

void AnimationComponent::BuildSkinningPalette() {
    if (!m_model || m_model->GetNodes().empty() || m_model->GetBones().empty()) return;
    m_skeletonPose->skinMatrices.assign(m_model->GetBones().size(), glm::mat4{1.0f});
    
    BuildSkinningPaletteRecursive(m_model->GetRootNodeIndex(), glm::mat4{1.0f});
}

void AnimationComponent::BuildSkinningPaletteRecursive(
    uint32_t nodeIndex, const glm::mat4& parentTransform) {
    const auto& nodes = m_model->GetNodes();
    if (nodeIndex >= nodes.size() ||
        nodeIndex >= m_currentPose.localTransforms.size()) return;
    const glm::mat4 globalTransform =
        parentTransform * m_currentPose.localTransforms[nodeIndex].ToMatrix();
    const auto& bones = m_model->GetBones();
    for (std::size_t i = 0; i < bones.size(); ++i) {
        if (bones[i].nodeIndex == nodeIndex) {
            m_skeletonPose->skinMatrices[i] = m_model->GetGlobalRootInverseMat() *
                globalTransform * bones[i].inverseBindMatrix;
        }
    }
    for (const uint32_t child : nodes[nodeIndex].children)
        BuildSkinningPaletteRecursive(child, globalTransform);
}

bool AnimationComponent::TryStartStateTransition() {
    if (!m_controller || m_transition.active ||
        m_currentStateId == InvalidAnimatorId ||
        m_playback.motionId == InvalidAnimatorId) {
        return false;
    }
    const auto& transitions = m_controller->GetTransitionsFrom(m_currentStateId);
    if (transitions.empty()) return false;
    for (const auto& transition : transitions) {
        if (!transition.hasExitTime ||
            m_playback.normalizedTime >= transition.exitTimeNormalized) {
            if (!AreConditionsMet(transition)) continue;

            const auto* newState =
                m_controller->GetState(transition.destinationState);
            if (!newState || !CrossFadeMotion(newState->motionId,
                    transition.blendDuration, newState->looping)) {
                continue;
            }
            const float startNormalized = glm::clamp(
                transition.destinationStartNormalized, 0.0f, 1.0f);
            if (m_transition.active) {
                m_transition.destination.speed = newState->speed;
                m_transition.destination.SetNormalizedTime(startNormalized);
                m_destinationStateId = transition.destinationState;
            } else {
                m_playback.speed = newState->speed;
                m_playback.SetNormalizedTime(startNormalized);
                m_currentStateId = transition.destinationState;
                m_destinationStateId = InvalidAnimatorId;
            }
            ConsumeTriggersUsedBy(transition);
            return true;
        }
    }
    return false;
}

bool AnimationComponent::AreConditionsMet(
    const AnimatorTransitionDefinition& transition) const {
    for (const auto& condition : transition.conditions) {
        if (!IsConditionMet(condition)) return false;
    }
    return true;
}

bool AnimationComponent::IsConditionMet(
    const AnimatorCondition& condition) const {
    const auto* paramDef = m_controller->GetParameter(condition.parameterId);
    if (!paramDef) return false;

    const AnimatorParameterValue& paramValue =
        m_parameterValues[condition.parameterId];
    switch (paramDef->type)
    {
    case AnimatorParameterType::Float: {
        if (!std::holds_alternative<float>(paramValue)) return false;
        const float value = std::get<float>(paramValue);
        if (!std::holds_alternative<float>(condition.threshold)) return false;
        const float threshold = std::get<float>(condition.threshold);
        switch (condition.op)
        {
        case AnimatorConditionOp::Greater:
            return value > threshold;
        case AnimatorConditionOp::GreaterEqual:
            return value >= threshold;
        case AnimatorConditionOp::Less:
            return value < threshold;
        case AnimatorConditionOp::LessEqual:
            return value <= threshold;
        case AnimatorConditionOp::Equal: {
            const float tolerance = std::numeric_limits<float>::epsilon() *
                std::max({1.0f, std::fabs(value), std::fabs(threshold)});
            return std::fabs(value - threshold) <= tolerance;
        }
        case AnimatorConditionOp::NotEqual: {
            const float tolerance = std::numeric_limits<float>::epsilon() *
                std::max({1.0f, std::fabs(value), std::fabs(threshold)});
            return std::fabs(value - threshold) > tolerance;
        }
        default:
            return false;
        }
    }
    case AnimatorParameterType::Bool: {
        if (!std::holds_alternative<bool>(paramValue)) return false;
        const bool value = std::get<bool>(paramValue);
        switch (condition.op)
        {
        case AnimatorConditionOp::IsTrue:
            return value;
        case AnimatorConditionOp::IsFalse:
            return !value;
        default:
            return false;
        }
    }
    case AnimatorParameterType::Int: {
        if (!std::holds_alternative<int32_t>(paramValue)) return false;
        const int32_t value = std::get<int32_t>(paramValue);
        if (!std::holds_alternative<int32_t>(condition.threshold)) return false;
        const int32_t threshold = std::get<int32_t>(condition.threshold);
        switch (condition.op)
        {
        case AnimatorConditionOp::Greater:
            return value > threshold;
        case AnimatorConditionOp::GreaterEqual:
            return value >= threshold;
        case AnimatorConditionOp::Less:
            return value < threshold;
        case AnimatorConditionOp::LessEqual:
            return value <= threshold;
        case AnimatorConditionOp::Equal:
            return value == threshold;
        case AnimatorConditionOp::NotEqual:
            return value != threshold;
        default:
            return false;
        }
    }
    case AnimatorParameterType::Trigger: {
        if (!std::holds_alternative<bool>(paramValue)) return false;
        const bool value = std::get<bool>(paramValue);
        switch (condition.op)
        {
        case AnimatorConditionOp::IsTriggered:
            return value;
        default:
            return false;
        }
    }
    default:
        return false;
    }
    return false;
}

void AnimationComponent::ConsumeTriggersUsedBy(
    const AnimatorTransitionDefinition& transition) {
    for (const auto& condition : transition.conditions) {
        const auto* paramDef = m_controller->GetParameter(condition.parameterId);
        if (!paramDef) continue;
        if (paramDef->type == AnimatorParameterType::Trigger &&
            condition.op == AnimatorConditionOp::IsTriggered) {
            m_parameterValues[condition.parameterId] = false;
        }
    }
}

const TransformTrack* AnimationComponent::FindTrack(const AnimationClip* clip, 
    uint32_t nodeIndex) const {
    if (!clip) return nullptr;
    for (const auto& track : clip->tracks)
        if (track.targetNodeIndex == nodeIndex) return &track;
    return nullptr;
}

const AnimationClip* AnimationComponent::FindAnimationClip(
    const std::string& name) const {
    auto it = m_clips.find(name);
    if (it == m_clips.end()) return nullptr;
    return it->second.get();
}

LocalTransform AnimationComponent::SampleLocalTransform(const AnimationClip* clip, 
    uint32_t nodeIndex, float time) const {
    const glm::mat4& bind = m_model->GetNodes()[nodeIndex].localTransform;
    glm::vec3 scale{1.0f}, translation{0.0f}, skew;
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec4 perspective;
    if (!glm::decompose(bind, scale, rotation, translation, skew, perspective))
        return LocalTransform{};

    const TransformTrack* track = FindTrack(clip, nodeIndex);
    rotation = glm::normalize(rotation);
    if (!track) return LocalTransform{translation, rotation, scale};

    if (!track->positions.empty()) translation = Interpolate(track->positions, time);
    if (!track->rotations.empty()) rotation = Interpolate(track->rotations, time);
    if (!track->scales.empty()) scale = Interpolate(track->scales, time);
    return LocalTransform{translation, rotation, scale};
}

AnimationPose AnimationComponent::SampleLocalPose(
    const AnimatorMotionData& motionData, float normalizedTime) const {
    AnimationPose pose{};
    std::visit([this, &pose, normalizedTime](const auto& value) {
        using ValueType = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<ValueType, ClipMotionDefinition>) {
            pose = SampleLocalPose(value, normalizedTime);
        } else if constexpr (std::is_same_v<ValueType, BlendTree1DDefinition>) {
            pose = SampleLocalPose(value, normalizedTime);
        }
    }, motionData);
    return pose;
}

AnimationPose AnimationComponent::SampleLocalPose(
    const ClipMotionDefinition& clipDef, float normalizedTime) const {
    AnimationPose pose{};
    const auto* clip = FindAnimationClip(clipDef.clipName);
    if (!clip) return pose;

    pose.localTransforms.resize(m_model->GetNodes().size());
    for (std::size_t i = 0; i < m_model->GetNodes().size(); ++i) {
        pose.localTransforms[i] =
            SampleLocalTransform(clip, i, normalizedTime * clip->duration);
    }
    return pose;
}

AnimationPose AnimationComponent::SampleLocalPose(
    const BlendTree1DDefinition& blendTree, float normalizedTime) const {
    AnimationPose localPose{};
    if (!m_controller || blendTree.parameterId >= m_parameterValues.size())
        return localPose;
    const AnimatorParameterValue& paramValue = m_parameterValues[blendTree.parameterId];
    if (!std::holds_alternative<float>(paramValue)) {
        std::cout << "AnimationComponent::SampleLocalPose: "
            "parameter type should be float\n";
        return localPose;
    }

    float value = std::get<float>(paramValue);
    const auto& children = blendTree.children;

    if (children.empty()) return localPose;
    if (value <= children[0].threshold) {
        const auto* motionDef = m_controller->GetMotion(children[0].motionId);
        if (!motionDef) return localPose;
        return SampleLocalPose(motionDef->data, normalizedTime);
    }
    if (value >= children.back().threshold) {
        const auto* motionDef = m_controller->GetMotion(children.back().motionId);
        if (!motionDef) return localPose;
        return SampleLocalPose(motionDef->data, normalizedTime);
    }

    const auto right = std::lower_bound(children.begin() + 1, children.end(), value,
        [](const BlendTree1DChild& child, float input) {
            return child.threshold < input;
        });
    if (value == right->threshold) {
        const auto* motion = m_controller->GetMotion(right->motionId);
        return motion ? SampleLocalPose(motion->data, normalizedTime) : localPose;
    }
    const auto left = right - 1;
    const auto* startMotionDef = m_controller->GetMotion(left->motionId);
    const auto* endMotionDef = m_controller->GetMotion(right->motionId);
    if (!startMotionDef || !endMotionDef) return localPose;
    const float span = right->threshold - left->threshold;
    if (span <= 0.0f) return localPose;
    const AnimationPose startPose =
        SampleLocalPose(startMotionDef->data, normalizedTime);
    const AnimationPose endPose =
        SampleLocalPose(endMotionDef->data, normalizedTime);
    localPose = Mix(startPose, endPose, glm::clamp(
        (value - left->threshold) / span, 0.0f, 1.0f));
    return localPose;
}

AnimationPose AnimationComponent::SampleLocalPose(const MotionPlayback& playback) const {
    if (!m_controller) return AnimationPose{};
    AnimatorMotionId motionId = playback.motionId;
    const auto* motionDef = m_controller->GetMotion(motionId);
    if (!motionDef) {
        return AnimationPose{};
    }
    return SampleLocalPose(motionDef->data, playback.normalizedTime);
}

AnimationPose AnimationComponent::SampleLocalPose(const AnimationBlendTransition& transition) const {
    const AnimationPose& sourcePose = transition.sourcePose;
    const AnimationPose destinationPose = SampleLocalPose(transition.destination);
    AnimationPose blendedPose;
    blendedPose.localTransforms.resize(m_model->GetNodes().size());
    if (sourcePose.localTransforms.size() != m_model->GetNodes().size() ||
        transition.duration <= 0.0f) {
        return destinationPose;
    }
    const float t = glm::clamp(
        transition.elapsed / transition.duration, 0.0f, 1.0f);
    for (std::size_t i = 0; i < m_model->GetNodes().size(); ++i) {
        blendedPose.localTransforms[i].translation = glm::mix(
            sourcePose.localTransforms[i].translation,
            destinationPose.localTransforms[i].translation, t);
        blendedPose.localTransforms[i].rotation = glm::normalize(glm::slerp(
            sourcePose.localTransforms[i].rotation,
            destinationPose.localTransforms[i].rotation, t));
        blendedPose.localTransforms[i].scale = glm::mix(
            sourcePose.localTransforms[i].scale,
            destinationPose.localTransforms[i].scale, t);
    }
    return blendedPose;
}

bool AnimationComponent::ValidateControllerClipName(
    const AnimatorController& controller) const {
    for (const auto& state : controller.GetStates()) {
        const auto* motionDef = controller.GetMotion(state.motionId);
        if (!motionDef) return false;
        if (!ValidateMotionClipName(controller, motionDef->data)) return false;
    }
    return true;
}

bool AnimationComponent::ValidateMotionClipName(
    const AnimatorController& controller,
    const AnimatorMotionData& motion) const {
    bool valid = true;
    std::visit([this, &controller, &valid](const auto& value) {
        using ValueType = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<ValueType, ClipMotionDefinition>) {
            valid = ValidateClipMotionClipName(value);
        } else if constexpr (std::is_same_v<ValueType, BlendTree1DDefinition>) {
            valid = ValidateBlendTree1DClipName(controller, value);
        }
    }, motion);
    return valid;
}

bool AnimationComponent::ValidateClipMotionClipName(
    const ClipMotionDefinition& clipDef) const {
    const auto* clip = FindAnimationClip(clipDef.clipName);
    return clip && std::isfinite(clip->duration) && clip->duration > 0.0f;
}

bool AnimationComponent::ValidateBlendTree1DClipName(
    const AnimatorController& controller,
    const BlendTree1DDefinition& blendTreeDef) const {
    for (const auto& child : blendTreeDef.children) {
        const auto* motionDef = controller.GetMotion(child.motionId);
        if (!motionDef) return false;
        const auto* clipMotion =
            std::get_if<ClipMotionDefinition>(&motionDef->data);
        if (!clipMotion || !ValidateClipMotionClipName(*clipMotion)) return false;
    }
    return true;
}


} // namespace eng
