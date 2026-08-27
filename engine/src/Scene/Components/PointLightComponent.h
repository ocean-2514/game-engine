#ifndef O_POINT_LIGHT  
#define O_POINT_LIGHT  

#include "Scene/Components/LightComponent.h"


namespace eng
{
    
class PointLightComponent : public LightComponent
{
public:
    ENG_COMPONENT_TYPE(PointLightComponent);

    PointLightComponent(const glm::vec3& color = glm::vec3{1.0f}, 
        float intensity = 1.0f, float range = 10.0f);
    
    float GetRange() const;
    void SetRange(float range);

private:
    float m_range = 10.0f;
};





} // namespace eng


#endif // O_POINT_LIGHT