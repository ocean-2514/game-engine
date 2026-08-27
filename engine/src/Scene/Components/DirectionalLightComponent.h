#ifndef O_DIRECTIONAL_LIGHT
#define O_DIRECTIONAL_LIGHT

#include "Scene/Components/LightComponent.h"

namespace eng
{
    
class DirectionalLightComponent : public LightComponent
{
public:
    ENG_COMPONENT_TYPE(DirectionalLightComponent);

    DirectionalLightComponent(const glm::vec3& color = glm::vec3{1.0f}, 
        float intensity = 1.0f);

    glm::vec3 GetDirection() const;
};


} // namespace eng


#endif // O_DIRECTIONAL_LIGHT