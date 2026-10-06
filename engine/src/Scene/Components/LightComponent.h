#ifndef O_LIGHT_COMPONENT
#define O_LIGHT_COMPONENT

#include "Scene/Component.h"
#include <glm/glm.hpp>

namespace eng
{

enum class LightType {
    Directional,
    Point,
    Spot
};
    
class LightComponent : public Component
{
public:
    ENG_COMPONENT_TYPE(LightComponent);

    LightComponent(const glm::vec3& color = glm::vec3{1.0f}, 
        float intensity = 1.0f);
    
    const glm::vec3& GetColor() const;
    void SetColor(const glm::vec3& color);
    float GetIntensity() const;
    void SetIntensity(float intensity);

private:
    glm::vec3 m_color{1.0f};
    float m_intensity = 1.0f;
};




} // namespace eng


#endif // O_LIGHT_COMPONENT