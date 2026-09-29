#include "Renderer/AnimatorController.h"

#include <cmath>
#include <iostream>
#include <utility>
#include <algorithm>

namespace eng {

namespace {

bool IsFinite(float value) {
    return std::isfinite(value);
}

void SetError(std::string* error, const std::string& value) {
    if (error) *error = value;
}

} // namespace

AnimatorParameterId AnimatorController::AddFloat(
    std::string name, float defaultValue) {
    if (name.empty() || !IsFinite(defaultValue) ||
        FindParameter(name) != InvalidAnimatorId) {
        std::cout << "AnimatorController::AddFloat: invalid or duplicate parameter: "
                  << name << '\n';
        return InvalidAnimatorId;
    }
    m_parameters.push_back({std::move(name),
        AnimatorParameterType::Float, defaultValue});
    return static_cast<AnimatorParameterId>(m_parameters.size() - 1);
}

AnimatorParameterId AnimatorController::AddBool(
    std::string name, bool defaultValue) {
    if (name.empty() || FindParameter(name) != InvalidAnimatorId) {
        std::cout << "AnimatorController::AddBool: invalid or duplicate parameter: "
                  << name << '\n';
        return InvalidAnimatorId;
    }
    m_parameters.push_back({std::move(name),
        AnimatorParameterType::Bool, defaultValue});
    return static_cast<AnimatorParameterId>(m_parameters.size() - 1);
}

AnimatorParameterId AnimatorController::AddInt(
    std::string name, int32_t defaultValue) {
    if (name.empty() || FindParameter(name) != InvalidAnimatorId) {
        std::cout << "AnimatorController::AddInt: invalid or duplicate parameter: "
                  << name << '\n';
        return InvalidAnimatorId;
    }
    m_parameters.push_back({std::move(name),
        AnimatorParameterType::Int, defaultValue});
    return static_cast<AnimatorParameterId>(m_parameters.size() - 1);
}

AnimatorParameterId AnimatorController::AddTrigger(std::string name) {
    if (name.empty() || FindParameter(name) != InvalidAnimatorId) {
        std::cout << "AnimatorController::AddTrigger: invalid or duplicate parameter: "
                  << name << '\n';
        return InvalidAnimatorId;
    }
    m_parameters.push_back({std::move(name),
        AnimatorParameterType::Trigger, false});
    return static_cast<AnimatorParameterId>(m_parameters.size() - 1);
}

AnimatorMotionId AnimatorController::AddClipMotion(
    std::string name, std::string clipName) {
    if (name.empty() || clipName.empty() || 
        FindMotion(name) != InvalidAnimatorId) {
        std::cout << "AnimatorController::AddClipMotion: invalid or duplicate motion: "
            << name << '\n';
        return InvalidAnimatorId;
    }
    m_motions.push_back({
        std::move(name), ClipMotionDefinition{std::move(clipName)}
    });
    return static_cast<AnimatorMotionId>(m_motions.size() - 1);
}

AnimatorMotionId AnimatorController::AddBlendTree1D(std::string name, 
    AnimatorParameterId parameter, std::vector<BlendTree1DChild> children) {
    if (name.empty() || children.empty() || 
        FindMotion(name) != InvalidAnimatorId) {
        std::cout << "AnimatorController::AddBlendTree1D: invalid or duplicate motion: "
            << name << '\n';
        return InvalidAnimatorId;
    }
    const auto* parameterDefinition = GetParameter(parameter);
    if (!parameterDefinition ||
        parameterDefinition->type != AnimatorParameterType::Float) {
        std::cout << "AnimatorController::AddBlendTree1D: parameter must be float\n";
        return InvalidAnimatorId;
    }
    if (!ValidateBlendTree1DChildren(children)) {
        std::cout << "AnimatorController::AddBlendTree1D: invalid BlendTree1D children" 
            << '\n';
        return InvalidAnimatorId;
    }
    m_motions.push_back({
        std::move(name), 
        BlendTree1DDefinition{
            parameter, std::move(children)
        }
    });
    return static_cast<AnimatorMotionId>(m_motions.size() - 1);
}

AnimatorStateId AnimatorController::AddState(AnimationStateDefinition state) {
    if (state.name.empty() || !IsFinite(state.speed)
        || !IsValidMotionId(state.motionId)
        || FindState(state.name) != InvalidAnimatorId) {
        std::cout << "AnimatorController::AddState: invalid or duplicate state: "
                  << state.name << '\n';
        return InvalidAnimatorId;
    }
    m_states.push_back(std::move(state));
    return static_cast<AnimatorStateId>(m_states.size() - 1);
}

bool AnimatorController::AddTransition(
    AnimatorTransitionDefinition transition) {
    if (!IsValidTransition(transition)) return false;
    m_transitions.push_back(std::move(transition));
    return true;
}

bool AnimatorController::SetDefaultState(AnimatorStateId state) {
    if (!IsValidStateId(state)) return false;
    m_defaultStateId = state;
    return true;
}

AnimatorMotionId AnimatorController::FindMotion(
    std::string_view name) const {
    for (std::size_t i = 0; i < m_motions.size(); ++i) {
        if (m_motions[i].name == name) {
            return static_cast<AnimatorMotionId>(i);
        }
    }
    return InvalidAnimatorId;
}

AnimatorMotionId AnimatorController::FindClipMotionByClipName(
    std::string_view name) const {
    for (std::size_t i = 0; i < m_motions.size(); ++i) {
        if (std::holds_alternative<ClipMotionDefinition>(m_motions[i].data) && 
            std::get<ClipMotionDefinition>(m_motions[i].data).clipName == name) {
            return static_cast<AnimatorMotionId>(i);
        }
    }
    return InvalidAnimatorId;
}

AnimatorParameterId AnimatorController::FindParameter(
    std::string_view name) const {
    for (std::size_t i = 0; i < m_parameters.size(); ++i) {
        if (m_parameters[i].name == name)
            return static_cast<AnimatorParameterId>(i);
    }
    return InvalidAnimatorId;
}

AnimatorStateId AnimatorController::FindState(std::string_view name) const {
    for (std::size_t i = 0; i < m_states.size(); ++i) {
        if (m_states[i].name == name)
            return static_cast<AnimatorStateId>(i);
    }
    return InvalidAnimatorId;
}

const AnimatorParameterDefinition* AnimatorController::GetParameter(
    AnimatorParameterId id) const {
    return IsValidParameterId(id) ? &m_parameters[id] : nullptr;
}

const AnimatorMotionDefinition* AnimatorController::GetMotion(
    AnimatorMotionId id) const {
    return IsValidMotionId(id) ? &m_motions[id] : nullptr;
}

const AnimationStateDefinition* AnimatorController::GetState(
    AnimatorStateId id) const {
    return IsValidStateId(id) ? &m_states[id] : nullptr;
}

const std::vector<AnimationStateDefinition>& AnimatorController::GetStates() const {
    return m_states;
}

const std::vector<AnimatorMotionDefinition>& 
    AnimatorController::GetMotions() const {
    return m_motions;
}

const std::vector<AnimatorParameterDefinition>&
    AnimatorController::GetParameters() const {
    return m_parameters;
}

std::vector<AnimatorTransitionDefinition>
AnimatorController::GetTransitionsFrom(AnimatorStateId state) const {
    std::vector<AnimatorTransitionDefinition> result;
    if (!IsValidStateId(state)) return result;
    for (const auto& transition : m_transitions) {
        if (transition.sourceState == state) result.push_back(transition);
    }
    return result;
}

AnimatorStateId AnimatorController::GetDefaultStateId() const {
    return m_defaultStateId;
}

bool AnimatorController::Validate(std::string* error) const {
    if (error) error->clear();
    if (!IsValidStateId(m_defaultStateId)) {
        SetError(error, "default state is invalid");
        return false;
    }
    for (const auto& parameter : m_parameters) {
        const bool typeMatches =
            ((parameter.type == AnimatorParameterType::Float) &&
                std::holds_alternative<float>(parameter.defaultValue)) ||
            ((parameter.type == AnimatorParameterType::Int) &&
                std::holds_alternative<int32_t>(parameter.defaultValue)) ||
            ((parameter.type == AnimatorParameterType::Bool ||
              parameter.type == AnimatorParameterType::Trigger) &&
                std::holds_alternative<bool>(parameter.defaultValue));
        if (parameter.name.empty() || !typeMatches) {
            SetError(error, "parameter definition is invalid: " + parameter.name);
            return false;
        }
        if (parameter.type == AnimatorParameterType::Float &&
            !IsFinite(std::get<float>(parameter.defaultValue))) {
            SetError(error, "float parameter default is not finite: " + parameter.name);
            return false;
        }
    }
    for (const auto& state : m_states) {
        if (state.name.empty() || !IsFinite(state.speed) ||
            !IsValidMotionId(state.motionId)) {
            SetError(error, "state definition is invalid: " + state.name);
            return false;
        }
        const auto* motion = GetMotion(state.motionId);
        if (motion && std::holds_alternative<BlendTree1DDefinition>(motion->data) &&
            !state.looping) {
            SetError(error, "1D blend tree state must be looping: " + state.name);
            return false;
        }
    }
    for (const auto& transition : m_transitions) {
        if (!IsValidTransition(transition)) {
            SetError(error, "transition definition is invalid");
            return false;
        }
    }

    for (const auto& motion : m_motions) {
        if (motion.name.empty()) {
            SetError(error, "motion name is empty");
            return false;
        }
        if (const auto* clip = std::get_if<ClipMotionDefinition>(&motion.data)) {
            if (clip->clipName.empty()) {
                SetError(error, "clip motion has an empty clip name: " + motion.name);
                return false;
            }
            continue;
        }

        const auto& tree = std::get<BlendTree1DDefinition>(motion.data);
        const auto* parameter = GetParameter(tree.parameterId);
        if (!parameter || parameter->type != AnimatorParameterType::Float ||
            tree.children.empty()) {
            SetError(error, "1D blend tree definition is invalid: " + motion.name);
            return false;
        }
        float previousThreshold = 0.0f;
        bool first = true;
        for (const auto& child : tree.children) {
            const auto* childMotion = GetMotion(child.motionId);
            if (!IsFinite(child.threshold) || !childMotion ||
                (!first && child.threshold <= previousThreshold)) {
                SetError(error, "1D blend tree child is invalid: " + motion.name);
                return false;
            }
            first = false;
            previousThreshold = child.threshold;
        }
    }

    std::vector<uint8_t> visitStates(m_motions.size(), 0);
    for (AnimatorMotionId motionId = 0; motionId < m_motions.size(); ++motionId) {
        if (!ValidateMotionGraph(motionId, visitStates, error)) return false;
    }
    
    return true;
}

bool AnimatorController::ValidateBlendTree1DChildren(
    std::vector<BlendTree1DChild>& children) {
    for (const auto& child : children) {
        const auto* motion = GetMotion(child.motionId);
        if (!IsFinite(child.threshold) || !motion) {
            return false;
        }
    }
    std::sort(children.begin(), children.end(), 
        [](const BlendTree1DChild& a, const BlendTree1DChild& b) {
        return a.threshold < b.threshold;
    });
    for (std::size_t i = 1; i < children.size(); ++i) {
        if (children[i].threshold <= children[i - 1].threshold) return false;
    }
    return true;
}

bool AnimatorController::IsValidTransition(
    const AnimatorTransitionDefinition& transition) const {
    if (!IsValidStateId(transition.sourceState) ||
        !IsValidStateId(transition.destinationState) ||
        !IsFinite(transition.blendDuration) || transition.blendDuration < 0.0f ||
        !IsFinite(transition.exitTimeNormalized) ||
        transition.exitTimeNormalized < 0.0f ||
        transition.exitTimeNormalized > 1.0f ||
        !IsFinite(transition.destinationStartNormalized) ||
        transition.destinationStartNormalized < 0.0f ||
        transition.destinationStartNormalized > 1.0f) {
        return false;
    }
    for (const auto& condition : transition.conditions) {
        if (!IsValidCondition(condition)) return false;
    }
    return true;
}

bool AnimatorController::IsValidCondition(
    const AnimatorCondition& condition) const {
    const auto* parameter = GetParameter(condition.parameterId);
    if (!parameter) return false;
    switch (parameter->type) {
        case AnimatorParameterType::Float:
            return std::holds_alternative<float>(condition.threshold) &&
                IsFinite(std::get<float>(condition.threshold)) &&
                (condition.op == AnimatorConditionOp::Greater ||
                 condition.op == AnimatorConditionOp::GreaterEqual ||
                 condition.op == AnimatorConditionOp::Less ||
                 condition.op == AnimatorConditionOp::LessEqual ||
                 condition.op == AnimatorConditionOp::Equal ||
                 condition.op == AnimatorConditionOp::NotEqual);
        case AnimatorParameterType::Int:
            return std::holds_alternative<int32_t>(condition.threshold) &&
                (condition.op == AnimatorConditionOp::Greater ||
                 condition.op == AnimatorConditionOp::GreaterEqual ||
                 condition.op == AnimatorConditionOp::Less ||
                 condition.op == AnimatorConditionOp::LessEqual ||
                 condition.op == AnimatorConditionOp::Equal ||
                 condition.op == AnimatorConditionOp::NotEqual);
        case AnimatorParameterType::Bool:
            return condition.op == AnimatorConditionOp::IsTrue ||
                   condition.op == AnimatorConditionOp::IsFalse;
        case AnimatorParameterType::Trigger:
            return condition.op == AnimatorConditionOp::IsTriggered;
    }
    return false;
}

bool AnimatorController::ValidateMotionGraph(AnimatorMotionId motionId,
    std::vector<uint8_t>& visitStates, std::string* error) const {
    if (!IsValidMotionId(motionId)) {
        SetError(error, "motion graph contains an invalid motion id");
        return false;
    }
    // 1 means this node is on the current DFS stack; 2 means fully validated.
    if (visitStates[motionId] == 1) {
        SetError(error, "motion graph contains a cycle at: " +
            m_motions[motionId].name);
        return false;
    }
    if (visitStates[motionId] == 2) return true;

    visitStates[motionId] = 1;
    if (const auto* tree =
            std::get_if<BlendTree1DDefinition>(&m_motions[motionId].data)) {
        for (const auto& child : tree->children) {
            if (!ValidateMotionGraph(child.motionId, visitStates, error))
                return false;
        }
    }
    visitStates[motionId] = 2;
    return true;
}

bool AnimatorController::IsValidMotionId(AnimatorMotionId id) const {
    return id != InvalidAnimatorId && id < m_motions.size();
}

bool AnimatorController::IsValidParameterId(AnimatorParameterId id) const {
    return id != InvalidAnimatorId && id < m_parameters.size();
}

bool AnimatorController::IsValidStateId(AnimatorStateId id) const {
    return id != InvalidAnimatorId && id < m_states.size();
}

} // namespace eng
