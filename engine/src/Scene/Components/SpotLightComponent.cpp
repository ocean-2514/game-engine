#include "Scene/Components/SpotLightComponent.h"
#include "Scene/GameObject.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace eng
{
    
SpotLightComponent::SpotLightComponent(const glm::vec3& color, 
    float intensity, float range, 
    float innerConeAngle, float outerConeAngle)
    : LightComponent(color, intensity),
      m_range(std::max(range, 0.001f)) {
    m_innerConeAngle = std::clamp(innerConeAngle, 0.0f, 89.9f);
    m_outerConeAngle = std::clamp(outerConeAngle, m_innerConeAngle, 89.9f);
    m_innerConeCos = std::cos(glm::radians(m_innerConeAngle));
    m_outerConeCos = std::cos(glm::radians(m_outerConeAngle));
}

float SpotLightComponent::GetRange() const {
    return m_range;
}

void SpotLightComponent::SetRange(float range) {
    m_range = std::max(range, 0.001f);
}

float SpotLightComponent::GetInnerConeAngle() const {
    return m_innerConeAngle;
}

void SpotLightComponent::SetInnerConeAngle(float angle) {
    m_innerConeAngle = std::clamp(angle, 0.0f, m_outerConeAngle);
    m_innerConeCos = std::cos(glm::radians(m_innerConeAngle));
}

float SpotLightComponent::GetInnerConeCos() const {
    return m_innerConeCos;
}

float SpotLightComponent::GetOuterConeAngle() const {
    return m_outerConeAngle;
}

void SpotLightComponent::SetOuterConeAngle(float angle) {
    m_outerConeAngle = std::clamp(angle, m_innerConeAngle, 89.9f);
    m_outerConeCos = std::cos(glm::radians(m_outerConeAngle));
}

float SpotLightComponent::GetOuterConeCos() const {
    return m_outerConeCos;
}

glm::vec3 SpotLightComponent::GetDirection() const {
    return m_owner->GetWorldForward();
}



} // namespace eng
