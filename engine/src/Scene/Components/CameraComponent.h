#ifndef O_CAMERA_COMPONENTS
#define O_CAMERA_COMPONENTS

#include "Scene/Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace eng {


class CameraComponent : public Component
{
public:
    ENG_COMPONENT_TYPE(CameraComponent);

    CameraComponent(float fov = 45.0f, float znear = 0.01f, float zfar = 100.0f);
    
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;

private:
    float m_fov = 45.0f;
    float m_znear = 0.01f;
    float m_zfar = 100.0f;
};




} // namespace eng


#endif // O_CAMERA_COMPONENTS