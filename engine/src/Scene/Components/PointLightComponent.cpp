#include "Scene/Components/PointLightComponent.h"
#include "Scene/Scene.h"
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

void PointLightComponent::OnAttach(Scene& scene) {
    LightComponent::OnAttach(scene);
    scene.RegisterLight(LightType::Point, this);
}

void PointLightComponent::OnDetach(Scene& scene) {
    scene.UnregisterLight(this);
    LightComponent::OnDetach(scene);
}

} // namespace eng
