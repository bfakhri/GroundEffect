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
    float x, y, z = 0.0f; // [m] 3D Star position coordinates in world space
    float brightness;     // [0.0 - 1.0, dimensionless] Brightness intensity
};

class Game {
public:
    Game();
    ~Game();

    bool init(int width = 1280, int height = 720); // width [px], height [px]
    void run();
    void stepFrame(float dt); // dt in [s] - single-frame step for deterministic testing
    void shutdown();

    static constexpr float PHYSICS_TICK_RATE = 256.0f;            // [Hz] Fixed physics engine tick rate
    static constexpr float PHYSICS_DT        = 1.0f / 256.0f;     // [s] Fixed physics timestep (3.90625 ms)

    // Inspection & Test Accessors
    Battleship& getBattleship() { return m_battleship; }
    const Battleship& getBattleship() const { return m_battleship; }
    Terrain& getTerrain() { return m_terrain; }
    const Terrain& getTerrain() const { return m_terrain; }
    Camera2D& getCamera() { return m_camera; }
    const Camera2D& getCamera() const { return m_camera; }
    InputManager& getInput() { return m_input; }
    SDL_Renderer* getRenderer() const { return m_renderer; }
    int getScore() const { return m_score; }
    int getHighScore() const { return m_highScore; }
    float getGameTime() const { return m_gameTime; }
    int getCoinsInserted() const { return m_coinsInserted; }
    bool isPaused() const { return m_paused; }
    bool isRunning() const { return m_running; }
    bool isThrottleLockMode() const { return m_throttleLockMode; }
    float getCrashVibrationTimer() const { return m_crashVibrationTimer; }
    float getPhysicsFps() const { return m_physicsFps; }
    float getGraphicsFps() const { return m_graphicsFps; }
    uint64_t getTotalPhysicsTicks() const { return m_totalPhysicsTicks; }
    const std::string& getStatusMessage() const { return m_statusMessage; }
    const std::string& getStatusSubMessage() const { return m_statusSubMessage; }

    void cycleCameraMode();

private:
    void handleInput();
    void update(float dt); // dt in [s]
    void render();

    void renderHUD();
    void renderTelemetry();
    void renderNavRadar(float centerX, float centerY, float radius);
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

    // Rocket League Camera_TA state (tracked orientation & right-stick swivel angles)
    float m_camTrackYaw = 0.0f;    // [rad] Smoothed Car-Cam / Target-Cam yaw angle in XZ plane
    float m_camTrackPitch = 0.24f; // [rad] Smoothed Car-Cam / Target-Cam elevation pitch angle
    float m_camSwivelYaw = 0.0f;   // [rad] Right-stick swivel yaw offset (auto-centers to 0 when stick released)
    float m_camSwivelPitch = 0.0f; // [rad] Right-stick swivel pitch offset (auto-centers to 0 when stick released)

    // Fixed 256 Hz physics timestep accumulator & dual FPS telemetry state
    float m_physicsAccumulator = 0.0f;    // [s] Unconsumed time accumulator for 256 Hz fixed physics steps
    bool m_pendingBoost = false;          // [flag] Latched one-shot boost trigger for next physics tick
    bool m_pendingAbort = false;          // [flag] Latched one-shot abort trigger for next physics tick
    uint64_t m_totalPhysicsTicks = 0;     // [count] Cumulative 256 Hz physics ticks executed
    int m_physicsWindowTicks = 0;         // [count] Physics ticks in current measurement window
    int m_graphicsWindowFrames = 0;       // [count] Graphics frames in current measurement window
    float m_fpsWindowTimer = 0.0f;        // [s] Elapsed time in current FPS measurement window
    float m_physicsFps = PHYSICS_TICK_RATE; // [Hz] Measured physics engine tick rate
    float m_graphicsFps = 60.0f;          // [FPS] Measured graphics engine frame rate
};
