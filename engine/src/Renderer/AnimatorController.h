#ifndef O_ANIMATOR_CONTROLLER
#define O_ANIMATOR_CONTROLLER

#include "Renderer/Animation.h"
#include "Renderer/AnimatorTypes.h"

#include <vector>
#include <variant>
#include <limits>
#include <string_view>

namespace eng
{
    
class AnimatorController
{
public:
    AnimatorParameterId AddFloat(std::string name,
        float defaultValue = 0.0f);
    AnimatorParameterId AddBool(std::string name,
        bool defaultValue = false);
    AnimatorParameterId AddInt(std::string name,
        int32_t defaultValue = 0);
    AnimatorParameterId AddTrigger(std::string name);

    AnimatorMotionId AddClipMotion(std::string name, 
        std::string clipName);
    AnimatorMotionId AddBlendTree1D(std::string name, 
        AnimatorParameterId parameter,
        std::vector<BlendTree1DChild> children);
    AnimatorStateId AddState(AnimationStateDefinition state);
    bool AddTransition(AnimatorTransitionDefinition transition);
    bool SetDefaultState(AnimatorStateId state);

    AnimatorMotionId FindMotion(std::string_view name) const;
    AnimatorMotionId FindClipMotionByClipName(std::string_view name) const;
    AnimatorParameterId FindParameter(std::string_view name) const;
    AnimatorStateId FindState(std::string_view name) const;

    const AnimatorParameterDefinition* GetParameter(
        AnimatorParameterId id) const;
    const AnimatorMotionDefinition* GetMotion(
        AnimatorMotionId id) const;
    const AnimationStateDefinition* GetState(
        AnimatorStateId id) const;
    const std::vector<AnimationStateDefinition>& 
        GetStates() const;
    const std::vector<AnimatorMotionDefinition>& 
        GetMotions() const;
    const std::vector<AnimatorParameterDefinition>& 
        GetParameters() const;
    std::vector<AnimatorTransitionDefinition>
        GetTransitionsFrom(AnimatorStateId state) const;
    AnimatorStateId GetDefaultStateId() const;

    bool Validate(std::string* error = nullptr) const;

private:
    bool ValidateBlendTree1DChildren(
        std::vector<BlendTree1DChild>& children);
    bool IsValidTransition(
        const AnimatorTransitionDefinition& transition) const;
    bool IsValidCondition(const AnimatorCondition& condition) const;
    bool ValidateMotionGraph(AnimatorMotionId motionId,
        std::vector<uint8_t>& visitStates, std::string* error) const;
    bool IsValidMotionId(AnimatorMotionId id) const;
    bool IsValidParameterId(AnimatorParameterId id) const;
    bool IsValidStateId(AnimatorStateId id) const;

    AnimatorStateId m_defaultStateId = InvalidAnimatorId;
    std::vector<AnimatorMotionDefinition> m_motions;
    std::vector<AnimationStateDefinition> m_states;
    std::vector<AnimatorParameterDefinition> m_parameters;
    std::vector<AnimatorTransitionDefinition> m_transitions;
};



} // namespace eng


#endif // O_ANIMATOR_CONTROLLER
