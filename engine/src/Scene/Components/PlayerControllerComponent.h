#ifndef O_PLAYER_CONTROLLER_COMPONENT
#define O_PLAYER_CONTROLLER_COMPONENT

#include "Scene/Component.h"
#include <glm/vec3.hpp>

namespace eng {

class InputManager;

class PlayerControllerComponent : public Component
{
public:
    ENG_COMPONENT_TYPE(PlayerControllerComponent);

    PlayerControllerComponent(InputManager* inputManager,
        float sensitivity = 0.1f, float moveSpeed = 1.5f);
    
    void SetWorldUp(const glm::vec3& worldUp);
    const glm::vec3& GetWorldUp() const;
    void SetMoveHorizontally(bool horizontal);

protected:
    void OnUpdate(float deltaTime) override;

private:
    InputManager* m_inputManager = nullptr;
    glm::vec3 m_worldUp{0.0f, 1.0f, 0.0f};
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_sensitivity;
    float m_moveSpeed;
    bool m_moveHorizontally = true;
};



} // namespace eng


#endif // O_PLAYER_CONTROLLER_COMPONENT
