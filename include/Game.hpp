#pragma once

#include "VectorRenderer.hpp"
#include "VectorFont.hpp"
#include "SoundEngine.hpp"
#include "InputManager.hpp"
#include "Terrain.hpp"
#include "Battleship.hpp"

#include <SDL2/SDL.h>
#include <string>
#include <vector>

struct Star {
    float x, y;       // [m] Star position coordinates in world space
    float brightness; // [0.0 - 1.0, dimensionless] Brightness intensity
};

class Game {
public:
    Game();
    ~Game();

    bool init(int width = 1280, int height = 720); // width [px], height [px]
    void run();
    void shutdown();

private:
    void handleInput();
    void update(float dt); // dt in [s]
    void render();

    void renderHUD();
    void renderTelemetry();
    void renderFlightStatus();
    void renderControllerGuide();
    void initStars();

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    int m_screenWidth = 1280;   // [px] Screen width
    int m_screenHeight = 720;   // [px] Screen height
    bool m_running = false;
    bool m_paused = false;

    Camera2D m_camera;
    Terrain m_terrain;
    Battleship m_battleship;
    SoundEngine m_sound;
    InputManager m_input;

    // Game stats
    int m_score = 0;            // [points] Cumulative session score
    int m_highScore = 0;        // [points] High score
    float m_gameTime = 0.0f;    // [s] Elapsed flight time in seconds
    int m_coinsInserted = 0;    // [count] Coins inserted

    // CRT vector styling
    float m_crtGlow = 1.0f;     // [0.0 - 1.0, dimensionless] CRT phosphor glow intensity
    bool m_showScanlines = true;

    // Background starfield
    std::vector<Star> m_stars;

    // Landing / Crash message state
    std::string m_statusMessage;
    std::string m_statusSubMessage;
    float m_messageTimer = 0.0f; // [s] Message display timer
    bool m_scoredThisTouchdown = false; // Prevents duplicate scoring while resting on pad

    // Warning alert beep timer & rumble hold timer
    float m_warningBeepCooldown = 0.0f;   // [s] Cooldown timer between warning alarm beeps
    float m_impactRumbleHoldTimer = 0.0f; // [s] Sustained duration for impact rumble
    float m_crashVibrationTimer = 0.0f;   // [s] Remaining crash vibration duration (capped at 2.0s)
    BattleshipState m_lastBattleshipState = BattleshipState::LANDED;

    // Throttle coupling mode: true = both triggers mirror each other (whichever is higher wins)
    bool m_throttleLockMode = false;
};
