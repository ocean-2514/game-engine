#ifndef O_FIRST_PERSPECTIVE_CONTROLLER
#define O_FIRST_PERSPECTIVE_CONTROLLER

#include "eng.h"
#include "Game/Entities/Player.h"


// Attach the component to the player root node,
// the root node must have a camera object
// We haven't implemented the script component yet
class ThirdPerspectiveController : public eng::Component {
public:
    ENG_COMPONENT_TYPE(ThirdPerspectiveController);
    ThirdPerspectiveController(Player* player,
        eng::GameObject* camera, 
        float cameraPlayerDistance = 3.0f, 
        float sensitivity = 0.1f);
    
    void SetAnimationComponent(eng::AnimationComponent* comp);

protected:
    void OnUpdate(float deltaTime) override;

private:
    void ComputeModelYaw(const glm::vec3& moveDirection, float deltaTime);

    Player* m_player = nullptr;
    eng::GameObject* m_camera = nullptr;
    eng::AnimationComponent* m_animationComponent = nullptr;
    glm::vec3 m_worldUp{0.0f, 1.0f, 0.0f};
    float m_distance = 3.0f;
    float m_sensitivity = 0.1f;
    float m_cameraYawWorld = 0.0f;
    float m_modelTargetYaw = 0.0f;
    float m_modelCurrentYaw = 0.0f;
    float m_pitch = 0.0f;

    bool m_spacePressed = false;
    bool m_rightMousePressed = false;
    bool m_shouldRunning = false;
};

#endif // O_FIRST_PERSPECTIVE_CONTROLLER