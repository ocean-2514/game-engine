#ifndef O_SPOT_LIGHT
#define O_SPOT_LIGHT

#include "Scene/Components/LightComponent.h"

namespace eng
{
    
class SpotLightComponent : public LightComponent
{
public:
    ENG_COMPONENT_TYPE(SpotLightComponent);

    SpotLightComponent(
        const glm::vec3& color = glm::vec3{1.0f}, 
        float intensity = 1.0f, 
        float range = 10.0f, 
        float innerConeAngle = 20.0f, 
        float outerConeAngle = 30.0f
    );
    
    float GetRange() const;
    void SetRange(float range);
    float GetInnerConeAngle() const;
    float GetInnerConeCos() const;
    void SetInnerConeAngle(float angle);
    float GetOuterConeAngle() const;
    float GetOuterConeCos() const;
    void SetOuterConeAngle(float angle);
    glm::vec3 GetDirection() const;

protected:
    void OnAttach(Scene& scene) override;
    void OnDetach(Scene& scene) override;

private:
    float m_range = 10.0f;
    // expressed in degrees
    float m_innerConeAngle = 20.0f;
    // expressed in degrees
    float m_outerConeAngle = 30.0f;
    float m_innerConeCos;
    float m_outerConeCos;
};




} // namespace eng


#endif // O_SPOT_LIGHT