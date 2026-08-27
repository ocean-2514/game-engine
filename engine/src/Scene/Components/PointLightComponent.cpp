#include "Scene/Components/PointLightComponent.h"
#include <algorithm>


namespace eng
{
    
PointLightComponent::PointLightComponent(const glm::vec3& color, 
    float intensity, float range)
    : LightComponent(color, intensity),
      m_range(std::max(range, 0.001f)) {}

float PointLightComponent::GetRange() const {
    return m_range;
}

void PointLightComponent::SetRange(float range) {
    m_range = std::max(range, 0.001f);
}



} // namespace eng
