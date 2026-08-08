#include <iostream>
#include "Game.h"

int main() {
    Game* game = new Game();
    if (eng::Engine::GetInstance().Init(game)) {
        eng::Engine::GetInstance().Run();
    }
    
    return 0;
}