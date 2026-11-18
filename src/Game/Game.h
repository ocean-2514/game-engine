#ifndef O_GAME
#define O_GAME

#include "eng.h"
#include "Game/GameObjects/Camera.h"
#include "Game/Entities/Player.h"

class Game : public eng::Application {
public:
    bool Init() override;

    void Update(float deltaTime) override;

    void Render(eng::RenderQueue& queue) override;

    void Destroy() override;

    Player player;

private:
    bool InitSampleScene();
    bool InitMainMenuScene();
    void EnterSampleScene();

    std::unique_ptr<eng::Scene> m_mainMenuScene;
    std::unique_ptr<eng::Scene> m_sampleScene;
    eng::Scene* m_activeScene = nullptr;
};

#endif
