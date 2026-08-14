#include "Scene/Components/CameraComponent.h"
#include "Scene/GameObject.h"
#include <glm/gtx/quaternion.hpp>

namespace eng {
    

CameraComponent::CameraComponent(float fov, float znear, float zfar)
    : m_fov(fov), m_znear(znear), m_zfar(zfar) {}

glm::mat4 CameraComponent::GetViewMatrix() const {
    auto pos = m_owner->GetWorldPosition();
    auto up = m_owner->GetWorldUpward();
    auto front = m_owner->GetWorldForward();
    return glm::lookAt(pos, pos + front, up);
}

glm::mat4 CameraComponent::GetProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(m_fov), aspect, m_znear, m_zfar);
}




} // namespace eng
