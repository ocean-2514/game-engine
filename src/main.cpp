#include <iostream>
#include "Game/Game.h"

int main() {
    Game* game = new Game();
    if (eng::Engine::GetInstance().Init(game, 1920, 1080)) {
        eng::Engine::GetInstance().Run();
    }
    
    return 0;
}