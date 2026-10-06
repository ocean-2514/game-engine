#include "Scene/Components/DirectionalLightComponent.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"

namespace eng
{
    
DirectionalLightComponent::DirectionalLightComponent(
    const glm::vec3& color, float intensity)
    : LightComponent(color, intensity) {}

glm::vec3 DirectionalLightComponent::GetDirection() const {
    return m_owner->GetWorldForward();
}

void DirectionalLightComponent::OnAttach(Scene& scene) {
    LightComponent::OnAttach(scene);
    scene.RegisterLight(LightType::Directional, this);
}

void DirectionalLightComponent::OnDetach(Scene& scene) {
    scene.UnregisterLight(this);
    LightComponent::OnDetach(scene);
}

} // namespace eng
