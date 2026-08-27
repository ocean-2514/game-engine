#include "Scene/Components/LightComponent.h"
#include "Scene/GameObject.h"
#include <algorithm>

namespace eng
{
    
LightComponent::LightComponent(const glm::vec3& color, float intensity) 
    : m_color(glm::max(color, glm::vec3(0.0f))),
      m_intensity(std::max(intensity, 0.0f)) {}

const glm::vec3& LightComponent::GetColor() const {
    return m_color;
}

float LightComponent::GetIntensity() const {
    return m_intensity;
}

void LightComponent::SetColor(const glm::vec3& color) {
    m_color = glm::max(color, glm::vec3(0.0f));
}

void LightComponent::SetIntensity(float intensity) {
    m_intensity = std::max(intensity, 0.0f);
}



} // namespace eng
