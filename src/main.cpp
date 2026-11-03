#include "Game.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    std::cout << "====================================================\n";
    std::cout << "  GROUND EFFECT - VECTOR ARCADE FLIGHT SIMULATOR    \n";
    std::cout << "  Featuring Xbox 360 Controller Proportional Input  \n";
    std::cout << "====================================================\n";

    Game game;
    if (!game.init(1280, 720)) { // [px] 1280x720 window resolution in pixels
        std::cerr << "Failed to initialize GroundEffect game!" << std::endl;
        return 1;
    }

    std::cout << "[System] Initialized successfully. Starting main flight loop...\n";
    game.run();

    std::cout << "[System] Exiting GroundEffect.\n";
    return 0;
}
