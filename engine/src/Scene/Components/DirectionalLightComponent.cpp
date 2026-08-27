#include "Scene/Components/DirectionalLightComponent.h"
#include "Scene/GameObject.h"

namespace eng
{
    
DirectionalLightComponent::DirectionalLightComponent(
    const glm::vec3& color, float intensity)
    : LightComponent(color, intensity) {}

glm::vec3 DirectionalLightComponent::GetDirection() const {
    return m_owner->GetWorldForward();
}


} // namespace eng
