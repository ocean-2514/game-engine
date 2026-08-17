#ifndef O_GAME
#define O_GAME

#include "eng.h"
#include "Camera.h"

class Game : public eng::Application {
public:
    bool Init() override;

    void Update(float deltaTime) override;

    void Render(eng::RenderQueue& queue) override;

    void Destroy() override;

    std::shared_ptr<eng::Mesh> mesh;
    std::shared_ptr<eng::Material> material;
    eng::Scene scene;
    Camera* camera;

private:
    std::unique_ptr<eng::MaterialAssetLoader> m_materialAssetLoader;
    float xoffset = 0.0f;
    float yoffset = 0.0f;
};

#endif
