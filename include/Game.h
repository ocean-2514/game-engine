#ifndef O_GAME
#define O_GAME

#include "eng.h"

class Game : public eng::Application {
public:
    bool Init() override;

    void Update(float deltaTime) override;

    void Destroy() override;
};

#endif