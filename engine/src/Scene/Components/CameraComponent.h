#ifndef O_CAMERA_COMPONENTS
#define O_CAMERA_COMPONENTS

#include "Scene/Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace eng {

class InputManager;

class CameraComponent : public Component
{
public:
    ENG_COMPONENT_TYPE(CameraComponent);

    CameraComponent(InputManager* inputManager,
        float fov = 45.0f, float znear = 0.01f, float zfar = 100.0f);

    void OnUpdate(float deltaTime) override;
    
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;

private:
    float m_fov = 45.0f;
    float m_znear = 0.01f;
    float m_zfar = 100.0f;
    float m_zoomSpeed = 1.0f;

    InputManager* m_inputManager;
};




} // namespace eng


#endif // O_CAMERA_COMPONENTS