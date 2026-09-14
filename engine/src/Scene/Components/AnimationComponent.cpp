#include "Scene/Components/AnimationComponent.h"
#include "Renderer/Model.h"
#include "Scene/GameObject.h"

#include <iostream>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace eng {

AnimationComponent::AnimationComponent(std::shared_ptr<const Model> model)
    : m_model(std::move(model)), m_skeletonPose(std::make_shared<SkeletonPose>()) {
    if (!m_model) return;
    m_skeletonPose->skinMatrices.resize(m_model->GetBones().size(), glm::mat4{1.0f});
    for (const auto& clip : m_model->GetAnimationClips()) Register(clip);
    if (!m_model->GetAnimationClips().empty()) {
        m_playback.clip = m_model->GetAnimationClips().front();
        m_playback.looping = m_playback.clip->looping;
    }
    m_currentPose = SampleLocalPose(m_playback);
    BuildSkinningPalette();
}

const AnimationClip* AnimationComponent::GetCurrentClip() const {
    if (m_transition.active) return m_transition.destination.clip.get();
    else return m_playback.clip.get();
}

const std::shared_ptr<const Model>& AnimationComponent::GetModel() const {
    return m_model;
}

const std::shared_ptr<SkeletonPose>& AnimationComponent::GetPose() const {
    return m_skeletonPose;
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
    if (m_transition.active) return m_playback.playing;
    else return m_playback.playing;
}

void AnimationComponent::SetPlaying(bool playing) { 
    if (m_transition.active) {
        m_playback.playing = playing;
        m_transition.destination.playing = playing;
    }
    else m_playback.playing = playing;
}
float AnimationComponent::GetTime() const { 
    if (m_transition.active) return m_transition.destination.time;
    else return m_playback.time;
}

void AnimationComponent::SetTime(float time) {
    if (m_transition.active) {
        m_transition.destination.SetTime(time);
    } else {
        m_playback.SetTime(time);
    }

    Renew();
}

void AnimationComponent::Register(std::shared_ptr<const AnimationClip> clip) {
    if (!clip || clip->name.empty()) return;
    m_clips.insert_or_assign(clip->name, std::move(clip));
}

// cancels any ongoing transition and starts playing the specified clip immediately
bool AnimationComponent::Play(const std::string& name, bool looping) {
    const auto it = m_clips.find(name);
    if (it == m_clips.end()) {
        std::cout << "AnimationComponent::Play: clip not found: " << name << std::endl;
        return false;
    }

    m_transition.active = false;

    m_playback.clip = it->second;
    m_playback.time = 0.0f;
    m_playback.looping = looping;
    m_playback.playing = true;
    BuildBindings();
    Renew();
    return true;
}

bool AnimationComponent::CrossFade(const std::string& name, 
    float duration, bool looping) {
    if (duration <= 0.0f) return Play(name, looping);

    const auto it = m_clips.find(name);
    if (it == m_clips.end()) {
        std::cout << "AnimationComponent::CrossFade: clip not found: " << name << std::endl;
        return false;
    }
    if (m_transition.active &&
        m_transition.destination.clip == it->second) {
        m_playback.playing = true;
        m_transition.destination.looping = looping;
        m_transition.destination.playing = true;
        return true;
    }
    if (!m_transition.active && m_playback.clip == it->second) {
        m_playback.looping = looping;
        m_playback.playing = true;
        return true;
    }

    m_transition.active = true;
    m_playback.playing = true;
    m_transition.sourcePose = m_currentPose;
    m_transition.destination.Reset();
    m_transition.destination.clip = it->second;
    m_transition.destination.time = 0.0f;
    m_transition.destination.looping = looping;
    m_transition.destination.playing = true;
    m_transition.elapsed = 0.0f;
    m_transition.duration = duration;
    return true;
}

void AnimationComponent::OnUpdate(float deltaTime) {
    if (deltaTime <= 0.0f) return;
    if (m_bindings.empty()) BuildBindings();
    if (m_transition.active) {
        if (!m_playback.playing) return;
        m_transition.elapsed += deltaTime;
        m_transition.destination.Update(deltaTime);
        if (m_transition.elapsed >= m_transition.duration) {
            m_playback = m_transition.destination;
            BuildBindings();
            m_transition.active = false;
        }
    } else if (m_playback.playing) {
        m_playback.Update(deltaTime);
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

const TransformTrack* AnimationComponent::FindTrack(const AnimationClip* clip, 
    uint32_t nodeIndex) const {
    if (!clip) return nullptr;
    for (const auto& track : clip->tracks)
        if (track.targetNodeIndex == nodeIndex) return &track;
    return nullptr;
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

AnimationPose AnimationComponent::SampleLocalPose(const AnimationClip* clip, 
    float time) const {
    AnimationPose pose;
    pose.localTransforms.resize(m_model->GetNodes().size());
    for (std::size_t i = 0; i < m_model->GetNodes().size(); ++i) {
        pose.localTransforms[i] = SampleLocalTransform(clip, i, time);
    }
    return pose;
}

AnimationPose AnimationComponent::SampleLocalPose(const AnimationPlayback& playback) const {
    return SampleLocalPose(playback.clip.get(), playback.time);
}

AnimationPose AnimationComponent::SampleLocalPose(const AnimationTransition& transition) const {
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

glm::vec3 AnimationComponent::Interpolate(
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

glm::quat AnimationComponent::Interpolate(
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

} // namespace eng
