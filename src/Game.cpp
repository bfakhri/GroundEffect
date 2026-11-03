#include "Game.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

// Conversion factors between Imperial and SI units
static constexpr float FT2M = 0.3048f;           // [m/ft] Length conversion factor (feet to meters)
static constexpr float LB2KG = 0.45359237f;      // [kg/lb] Mass conversion factor (pounds to kilograms)
static constexpr float DEG2RAD = 3.1415926535f / 180.0f;

static inline float wrapAnglePi(float a) {
    while (a >  3.14159265f) a -= 6.28318530f;
    while (a < -3.14159265f) a += 6.28318530f;
    return a;
}

static inline float shortestAngleDiff(float from, float to) {
    return wrapAnglePi(to - from);
}

Game::Game() {}

Game::~Game() {
    shutdown();
}

bool Game::init(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    m_window = SDL_CreateWindow("GroundEffect 3D - Vector Arcade Flight Simulator",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                m_screenWidth, m_screenHeight,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1,
                                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!m_renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

    VectorFont::init();
    m_sound.init();
    m_input.init();

    const auto& pads = m_terrain.getLandingPads();
    if (!pads.empty()) {
        float startX = (pads[0].x1 + pads[0].x2) * 0.5f;
        float startY = pads[0].y - 12.0f * FT2M;
        m_battleship.resetLanded(startX, startY, 750.0f * LB2KG, 0.0f);
    }

    m_camera.screenWidth = m_screenWidth;
    m_camera.screenHeight = m_screenHeight;
    float heightOffsetM = m_camera.rlSettings.height * 0.035f; // 100 uu -> 3.5 m local focus offset above ship
    Vec3 focusPos = m_battleship.localToWorldPoint({0.0f, -heightOffsetM, 0.0f});
    m_camera.x = focusPos.x;
    m_camera.y = focusPos.y;
    m_camera.z = focusPos.z;
    m_camera.zoom = m_camera.getFocalLengthPx() / 30.0f; // Close 30m Rocket League chase distance
    m_camera.use3D = true;
    m_camera.viewMode = CameraViewMode::CHASE_3D;
    m_camTrackYaw = 0.0f;
    m_camTrackPitch = 0.22f;
    m_camSwivelYaw = 0.0f;
    m_camSwivelPitch = 0.0f;
    m_camera.camYaw = m_camTrackYaw;
    m_camera.camPitch = m_camTrackPitch;

    initStars();

    m_running = true;
    m_lastBattleshipState = m_battleship.getState();
    m_crashVibrationTimer = 0.0f;
    m_statusMessage = "READY FOR LAUNCH (3D VECTOR MODE)";
    m_statusSubMessage = "APPLY THROTTLE TO TAKE OFF  |  [W/S] PITCH  [A/D] ROLL";
    m_messageTimer = 3.5f;
    m_scoredThisTouchdown = true;

    return true;
}

void Game::shutdown() {
    m_sound.shutdown();
    m_input.shutdown();

    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_Quit();
}

void Game::initStars() {
    m_stars.clear();
    auto randFloat = [](float minVal, float maxVal) {
        float r = (float)std::rand() / (float)RAND_MAX;
        return minVal + r * (maxVal - minVal);
    };

    // 1. Dense local 3D starfield around the terrain and low altitude
    for (int i = 0; i < 600; ++i) {
        Star s;
        s.x = randFloat(-800.0f * FT2M, 5600.0f * FT2M);
        s.y = randFloat(-2200.0f * FT2M, 2200.0f * FT2M);
        s.z = randFloat(0.0f * FT2M, 2600.0f * FT2M);
        s.brightness = randFloat(0.25f, 1.0f);
        m_stars.push_back(s);
    }

    // 2. High-altitude deep space 3D starfield
    for (int i = 0; i < 1400; ++i) {
        Star s;
        s.x = randFloat(-1200.0f * FT2M, 6000.0f * FT2M);
        s.y = randFloat(-200000.0f * FT2M, -2000.0f * FT2M);
        s.z = randFloat(0.0f * FT2M, 4000.0f * FT2M);
        s.brightness = randFloat(0.25f, 1.0f);
        m_stars.push_back(s);
    }
}

void Game::cycleCameraMode() {
    int next = ((int)m_camera.viewMode + 1) % (int)CameraViewMode::COUNT;
    m_camera.viewMode = (CameraViewMode)next;

    switch (m_camera.viewMode) {
        case CameraViewMode::CHASE_3D:
            m_camera.use3D = true;
            m_camera.hasCustomBasis = true;
            m_statusMessage = "CAMERA: 6-DOF SHIP-FRAME CHASE CAM";
            m_statusSubMessage = "1:1 SHIP ROTATION LOCK  |  [RS / J,L,I,O] SWIVEL";
            break;
        case CameraViewMode::ORBIT_3D:
            m_camera.use3D = true;
            m_camera.hasCustomBasis = false;
            m_statusMessage = "CAMERA: TARGET PAD CAM (BALL CAM)";
            m_statusSubMessage = "LOCKS ONTO NEAREST LANDING PAD  |  [RS / J,L,I,O] SWIVEL";
            break;
        case CameraViewMode::ISOMETRIC_3D:
            m_camera.use3D = true;
            m_camera.hasCustomBasis = false;
            m_camTrackYaw = 0.45f;
            m_camTrackPitch = 0.50f;
            m_camera.camYaw = 0.45f;
            m_camera.camPitch = 0.50f;
            m_statusMessage = "CAMERA: 3D TACTICAL ISOMETRIC";
            m_statusSubMessage = "HIGH-ANGLE 3D TACTICAL OVERVIEW";
            break;
        case CameraViewMode::CLASSIC_2D:
            m_camera.use3D = false;
            m_camera.hasCustomBasis = false;
            m_camera.camYaw = 0.0f;
            m_camera.camPitch = 0.0f;
            m_statusMessage = "CAMERA: CLASSIC 1979 2D ORTHO";
            m_statusSubMessage = "PRESS [TAB] OR [M] TO RETURN TO 3D";
            break;
        default:
            break;
    }
    m_messageTimer = 2.5f;
    m_sound.playBeep(1050.0f, 0.07f);
}

void Game::run() {
    Uint64 prevTime = SDL_GetPerformanceCounter();
    const float maxDt = 0.05f;

    while (m_running) {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float dt = (float)(currentTime - prevTime) / (float)SDL_GetPerformanceFrequency();
        prevTime = currentTime;

        if (dt > maxDt) dt = maxDt;

        stepFrame(dt);
    }
}

void Game::stepFrame(float dt) {
    handleInput();

    if (!m_paused) {
        update(dt);
    }

    render();
}

void Game::handleInput() {
    m_input.update();
    const auto& state = m_input.getState();

    if (state.quitRequested) {
        m_running = false;
        return;
    }

    if (state.pausePressed) {
        m_paused = !m_paused;
        m_sound.playBeep(600.0f, 0.05f);
    }

    if (state.toggleGlowPressed) {
        m_crtGlow = (m_crtGlow > 0.5f) ? 0.0f : 1.0f;
        m_showScanlines = (m_crtGlow > 0.5f);
        m_sound.playBeep(900.0f, 0.05f);
    }

    if (state.cycleCameraPressed) {
        cycleCameraMode();
    }

    // Insert Coin / Add Fuel
    if (state.addFuelPressed) {
        m_battleship.addFuel(500.0f * LB2KG);
        m_coinsInserted++;
        m_sound.playCoinSound();
        m_input.setRumble(0.0f, 0.4f, 150);
        m_statusMessage = "COIN INSERTED: +227 KG FUEL";
        m_statusSubMessage = "";
        m_messageTimer = 2.0f;
    }

    // Toggle Unlimited Fuel (Testing Mode)
    if (state.toggleUnlimitedFuelPressed) {
        m_battleship.toggleUnlimitedFuel();
        m_sound.playBeep(1100.0f, 0.08f);
        m_statusMessage = m_battleship.isUnlimitedFuel() ? "UNLIMITED FUEL: ACTIVE" : "UNLIMITED FUEL: OFF";
        m_statusSubMessage = "PRESS [U] TO TOGGLE TESTING FUEL";
        m_messageTimer = 2.5f;
    }

    // Toggle Throttle Lock Mode (T key / LS click)
    if (state.toggleThrottleLockPressed) {
        m_throttleLockMode = !m_throttleLockMode;
        m_sound.playBeep(m_throttleLockMode ? 1300.0f : 900.0f, 0.07f);
        m_statusMessage = m_throttleLockMode ? "THROTTLE: LOCKED (BOTH ENGINES LINKED)" : "THROTTLE: INDEPENDENT (L/R TRIGGERS)";
        m_statusSubMessage = m_throttleLockMode ? "EITHER TRIGGER NOW CONTROLS BOTH ENGINES" : "LEFT TRIGGER = L.ENG  RIGHT TRIGGER = R.ENG";
        m_messageTimer = 2.5f;
    }

    // Toggle Air Brakes (K key / RS click)
    if (state.toggleAirBrakesPressed) {
        m_battleship.toggleAirBrakes();
        m_sound.playBeep(m_battleship.isAirBrakesToggled() ? 820.0f : 640.0f, 0.07f);
        m_statusMessage = m_battleship.isAirBrakesToggled() ? "AIR BRAKES: LOCKED ON" : "AIR BRAKES: RETRACTED";
        m_statusSubMessage = "HOLD [DOWN] OR PRESS [K] TO DEPLOY AIR BRAKES";
        m_messageTimer = 2.2f;
    }

    // Toggle Ground Effect Hover Thrusters (H key / D-Pad Up)
    if (state.toggleHoverPressed) {
        m_battleship.toggleHoverThrusters();
        m_sound.playBeep(m_battleship.isHoverThrustersEnabled() ? 1250.0f : 700.0f, 0.08f);
        m_statusMessage = m_battleship.isHoverThrustersEnabled() ? "GROUND EFFECT HOVER: ON" : "GROUND EFFECT HOVER: OFF";
        m_statusSubMessage = m_battleship.isHoverThrustersEnabled()
            ? "AUTO-MODULATING 3D THRUST TO HOVER & SKATE ACROSS TERRAIN"
            : "PRESS [H] OR [DPAD UP] TO ENABLE HOVER THRUSTERS";
        m_messageTimer = 2.5f;
    }

    // Toggle Force Vector Overlay (V key; ON by default)
    if (state.toggleForceVectorsPressed) {
        m_battleship.toggleForceVectors();
        m_sound.playBeep(m_battleship.isForceVectorsVisible() ? 1150.0f : 680.0f, 0.07f);
        m_statusMessage = m_battleship.isForceVectorsVisible() ? "3D FORCE VECTORS: ON" : "3D FORCE VECTORS: OFF";
        m_statusSubMessage = "PRESS [V] TO TOGGLE HULL FORCE VECTOR OVERLAY";
        m_messageTimer = 2.2f;
    }

    // Cycle Landing Site
    if (state.nextSitePressed) {
        m_terrain.cycleSite();
        m_sound.playBeep(750.0f, 0.08f);
        const auto& pads = m_terrain.getLandingPads();
        if (!pads.empty()) {
            float padCenterX = (pads[0].x1 + pads[0].x2) * 0.5f;
            float padSurfaceY = pads[0].y - 12.0f * FT2M;
            m_battleship.resetLanded(padCenterX, padSurfaceY, std::max(m_battleship.getFuel(), 600.0f * LB2KG), 0.0f);
        }
        float heightOffsetM = m_camera.use3D ? (m_camera.rlSettings.height * 0.035f) : 0.0f;
        Vec3 focusPos = m_battleship.localToWorldPoint({0.0f, -heightOffsetM, 0.0f});
        m_camera.x = focusPos.x;
        m_camera.y = focusPos.y;
        m_camera.z = focusPos.z;
        m_camera.zoom = m_camera.use3D ? (m_camera.getFocalLengthPx() / 30.0f) : (2.10f / FT2M);
        m_camTrackYaw = 0.0f;
        m_camTrackPitch = 0.22f;
        m_camSwivelYaw = 0.0f;
        m_camSwivelPitch = 0.0f;
        m_camera.camYaw = 0.0f;
        m_camera.camPitch = 0.22f;
        m_statusMessage = "MISSION: " + m_terrain.getSiteName();
        m_statusSubMessage = "LANDED ON 3D PAD - READY FOR LAUNCH";
        m_messageTimer = 3.0f;
    }

    // Restart mission
    if (state.restartPressed) {
        if (m_battleship.getState() == BattleshipState::CRASHED ||
            (m_battleship.getState() == BattleshipState::LANDED && state.manualRestartPressed)) {
            float nextFuel = (m_battleship.getFuel() < 250.0f * LB2KG) ? 750.0f * LB2KG : m_battleship.getFuel();
            const auto& pads = m_terrain.getLandingPads();
            if (!pads.empty()) {
                float padCenterX = (pads[0].x1 + pads[0].x2) * 0.5f;
                float padSurfaceY = pads[0].y - 12.0f * FT2M;
                m_battleship.resetLanded(padCenterX, padSurfaceY, nextFuel, 0.0f);
            }
            float heightOffsetM = m_camera.use3D ? (m_camera.rlSettings.height * 0.035f) : 0.0f;
            Vec3 focusPos = m_battleship.localToWorldPoint({0.0f, -heightOffsetM, 0.0f});
            m_camera.x = focusPos.x;
            m_camera.y = focusPos.y;
            m_camera.z = focusPos.z;
            m_camera.zoom = m_camera.use3D ? (m_camera.getFocalLengthPx() / 30.0f) : (2.10f / FT2M);
            m_camTrackYaw = 0.0f;
            m_camTrackPitch = 0.22f;
            m_camSwivelYaw = 0.0f;
            m_camSwivelPitch = 0.0f;
            m_camera.camYaw = 0.0f;
            m_camera.camPitch = 0.22f;
            m_statusMessage = "READY FOR LAUNCH";
            m_statusSubMessage = "APPLY THROTTLE TO TAKE OFF";
            m_messageTimer = 2.5f;
            m_sound.playBeep(1000.0f, 0.1f);
            m_scoredThisTouchdown = true;
            m_crashVibrationTimer = 0.0f;
            m_input.setRumble(0.0f, 0.0f, 0);
        }
    }
}

void Game::update(float dt) {
    const auto& state = m_input.getState();

    m_gameTime += dt;

    float tLeft = state.throttleLeft;
    float tRight = state.throttleRight;
    if (m_throttleLockMode) {
        float linked = std::max(tLeft, tRight);
        tLeft = linked;
        tRight = linked;
    }

    // Update Battleship with 3D twin thruster, roll, pitch, yaw, and air brake inputs
    m_battleship.update(dt, tLeft, tRight, state.rotateAxis, state.abortPressed, state.boostPressed,
                        m_terrain, m_sound, state.airBrakeHeld, state.pitchAxis, state.yawAxis);

    if (m_battleship.wasImpulseFired()) {
        m_input.setRumble(0.85f, 0.90f, 180);
        m_impactRumbleHoldTimer = 0.18f;
        m_statusMessage = "IMPULSE THRUSTER!";
        if (m_battleship.getGroundEffectMagnitude() > 0.20f) {
            m_statusSubMessage = "IMPULSE 3D GROUND EFFECT CUSHION ACTIVE";
        } else {
            m_statusSubMessage = "ONE-FRAME RETRO BOOST APPLIED";
        }
        m_messageTimer = 1.2f;
    }

    auto impact = m_battleship.getAndClearImpactEvent();
    if (impact.occurred) {
        float speed = impact.speed;
        float intensity = std::clamp(speed / (40.0f * FT2M), 0.35f, 1.0f);
        uint32_t durationMs = (uint32_t)(140 + std::clamp(speed * (4.5f / FT2M), 0.0f, 220.0f));
        m_input.setRumble(intensity, intensity * 0.9f, durationMs);
        m_impactRumbleHoldTimer = (float)durationMs / 1000.0f;

        std::ostringstream ss;
        ss << "IMPACT! +" << (int)std::round(impact.damageInflicted)
           << "% DMG  [HULL " << (int)std::round(m_battleship.getHull()) << "%]";
        m_statusMessage = ss.str();
        m_statusSubMessage = (m_battleship.getHull() < 30.0f) ? "CRITICAL STRUCTURAL DAMAGE!" : "CAUTION: STRUCTURAL INTEGRITY WEAKENED";
        m_messageTimer = 1.8f;
    }

    if (m_battleship.getState() == BattleshipState::CRASHED && m_lastBattleshipState != BattleshipState::CRASHED) {
        m_crashVibrationTimer = 2.0f;
    }

    if (m_battleship.getState() == BattleshipState::FLYING) {
        if (m_impactRumbleHoldTimer > 0.0f) {
            m_impactRumbleHoldTimer -= dt;
        } else {
            float tL = m_battleship.getThrottleLeft();
            float tR = m_battleship.getThrottleRight();
            if (tL > 0.05f || tR > 0.05f) {
                m_input.setRumble(tL * 0.40f, tR * 0.40f, 60);
            }
        }
    } else if (m_battleship.getState() == BattleshipState::CRASHED) {
        if (m_crashVibrationTimer > 0.0f) {
            m_crashVibrationTimer -= dt;
            m_input.setRumble(0.85f, 0.95f, 120);
            if (m_crashVibrationTimer <= 0.0f) {
                m_input.setRumble(0.0f, 0.0f, 0);
            }
        }
    }

    float surfaceY = m_terrain.getSurfaceHeightAt(m_battleship.getX(), m_battleship.getZ());
    float altitude = std::max(0.0f, surfaceY - (m_battleship.getY() + 12.0f * FT2M));

    if (m_battleship.getState() == BattleshipState::FLYING && altitude < 250.0f * FT2M) {
        if (!m_battleship.isSafeSpeed() || !m_battleship.isSafeAngle()) {
            m_warningBeepCooldown -= dt;
            if (m_warningBeepCooldown <= 0.0f) {
                m_sound.playBeep(1400.0f, 0.06f);
                m_warningBeepCooldown = 0.25f;
            }
        }
    }

    if (m_battleship.getState() == BattleshipState::LANDED) {
        if (!m_scoredThisTouchdown) {
            int mult = m_battleship.getLastPadMultiplier();
            int points = 50 * mult + (int)((m_battleship.getFuel() / LB2KG) * 0.25f) * mult;
            m_score += points;
            if (m_score > m_highScore) m_highScore = m_score;

            std::ostringstream ss;
            ss << "PERFECT TOUCHDOWN! +" << points << " PTS (" << mult << "X PAD)";
            m_statusMessage = ss.str();
            m_statusSubMessage = "APPLY THROTTLE TO TAKE OFF AGAIN";
            m_messageTimer = 4.0f;
            m_input.setRumble(0.2f, 0.5f, 250);
            m_scoredThisTouchdown = true;
        }
    } else {
        m_scoredThisTouchdown = false;
    }

    if (m_battleship.getState() == BattleshipState::CRASHED) {
        if (m_statusMessage.find("CRASHED") == std::string::npos) {
            m_statusMessage = "CRASHED! HULL DESTROYED";
            m_statusSubMessage = "PRESS [A] OR [R] TO TRY AGAIN";
            m_messageTimer = 999.0f;
        }
    }

    if (m_messageTimer > 0.0f) {
        m_messageTimer -= dt;
    }

    // ── Rocket League 6-DOF Local Ship-Frame Camera Implementation ───────────
    const auto& rl = m_camera.rlSettings;
    float stiffness = std::clamp(rl.stiffness, 0.0f, 1.0f);

    if (m_camera.use3D) {
        // 1. Close Rocket League Distance (~30m behind ship + slight speed stretch, NO altitude zoom-out)
        float baseDistM = rl.distance * (30.0f / 270.0f); // 270 -> 30.0 m close third-person distance
        Vec3 vel3D = m_battleship.getVelocity3D();
        float speed = vel3D.length();
        static constexpr float SUPERSONIC_SPEED = 42.0f; // [m/s] Supersonic threshold speed
        float speedRatio = std::clamp(speed / SUPERSONIC_SPEED, 0.0f, 1.0f);
        float supersonicStretchM = (1.0f - stiffness) * 14.0f * speedRatio;
        float effectiveDistM     = std::max(12.0f, baseDistM + supersonicStretchM);

        float targetZoom = m_camera.getFocalLengthPx() / effectiveDistM;
        float zoomRate = 8.0f + stiffness * 16.0f;
        m_camera.zoom += (targetZoom - m_camera.zoom) * (1.0f - std::exp(-zoomRate * dt));

        // 2. Focus Point in Ship's Local 3D Frame (P_focus = P_ship + localToWorldDir(0, -Height, 0))
        float heightOffsetM = rl.height * 0.035f; // 100 -> 3.5 m above ship center of mass along ship's local Up axis
        Vec3 targetFocus = m_battleship.localToWorldPoint({0.0f, -heightOffsetM, 0.0f});

        float posRate  = 14.0f + stiffness * 26.0f;
        float posAlpha = 1.0f - std::exp(-posRate * dt);
        m_camera.x += (targetFocus.x - m_camera.x) * posAlpha;
        m_camera.y += (targetFocus.y - m_camera.y) * posAlpha;
        m_camera.z += (targetFocus.z - m_camera.z) * posAlpha;

        // 3. Base Downward Pitch & Proportional Right-Stick Swivel (auto-centers when released)
        float basePitchRad = std::clamp(-rl.pitch * DEG2RAD + 0.15f, 0.05f, 0.45f); // ~0.22 rad (~12.6 deg)

        static constexpr float MAX_SWIVEL_YAW   = 2.61799f; // [rad] 150 deg max horizontal swivel
        static constexpr float MAX_SWIVEL_PITCH = 1.22173f; // [rad] 70 deg max vertical swivel
        float targetSwivelYaw   = state.camYawAxis   * MAX_SWIVEL_YAW;
        float targetSwivelPitch = state.camPitchAxis * MAX_SWIVEL_PITCH;

        float swivelRate  = std::clamp(rl.swivelSpeed, 1.0f, 10.0f) * 2.4f;
        float swivelAlpha = 1.0f - std::exp(-swivelRate * dt);
        m_camSwivelYaw   += shortestAngleDiff(m_camSwivelYaw, targetSwivelYaw) * swivelAlpha;
        m_camSwivelPitch += (targetSwivelPitch - m_camSwivelPitch) * swivelAlpha;

        if (m_camera.viewMode == CameraViewMode::CHASE_3D) {
            // Full 6-DOF Local Ship-Frame Camera:
            // Anchored 1:1 to the ship's 3D rotation matrix (rolls, pitches, and yaws 1:1 with the ship)
            float localYaw   = m_camSwivelYaw;
            float localPitch = std::clamp(basePitchRad + m_camSwivelPitch, -1.35f, 1.35f);

            float cosY = std::cos(localYaw);
            float sinY = std::sin(localYaw);
            float cosP = std::cos(localPitch);
            float sinP = std::sin(localPitch);

            Vec3 localForward = { sinY * cosP, sinP, cosY * cosP };
            Vec3 localRight   = { cosY, 0.0f, -sinY };
            Vec3 localCamDown = { -sinP * sinY, cosP, -sinP * cosY };

            m_camera.customForward = m_battleship.localToWorldDir(localForward).normalized();
            m_camera.customRight   = m_battleship.localToWorldDir(localRight).normalized();
            m_camera.customCamDown = m_battleship.localToWorldDir(localCamDown).normalized();
            m_camera.hasCustomBasis = true;

            // Keep camYaw and camPitch synchronized with the world-space forward look vector
            m_camera.camYaw   = std::atan2(m_camera.customForward.x, m_camera.customForward.z);
            m_camera.camPitch = std::asin(std::clamp(m_camera.customForward.y, -1.0f, 1.0f));
            m_camTrackYaw     = m_camera.camYaw;
            m_camTrackPitch   = m_camera.camPitch;
        } else if (m_camera.viewMode == CameraViewMode::ORBIT_3D) {
            // Target Pad Cam (Ball Cam): locks camera onto nearest Landing Pad
            m_camera.hasCustomBasis = false;
            float targetYaw   = m_camTrackYaw;
            float targetPitch = m_camTrackPitch;
            const auto& pads = m_terrain.getLandingPads();
            if (!pads.empty()) {
                Vec3 shipPos = m_battleship.getPosition3D();
                float bestDistSq = 1e18f;
                Vec3 bestPadPos  = shipPos + Vec3{0.0f, 0.0f, 100.0f};
                for (const auto& pad : pads) {
                    Vec3 padCenter{0.5f * (pad.x1 + pad.x2), pad.y, 0.5f * (pad.z1 + pad.z2)};
                    float dSq = (padCenter - shipPos).lengthSq();
                    if (dSq < bestDistSq) {
                        bestDistSq = dSq;
                        bestPadPos = padCenter;
                    }
                }
                Vec3 toPad = bestPadPos - shipPos;
                float horizDist = std::sqrt(toPad.x * toPad.x + toPad.z * toPad.z);
                if (horizDist > 2.5f) {
                    targetYaw = std::atan2(toPad.x, toPad.z);
                    float padPitch = std::atan2(toPad.y, horizDist);
                    targetPitch = std::clamp(basePitchRad + padPitch * 0.45f, -1.25f, 1.25f);
                }
            }
            float rotRate  = (4.0f + stiffness * 11.0f) * std::clamp(rl.transitionSpeed, 0.5f, 2.5f);
            float rotAlpha = 1.0f - std::exp(-rotRate * dt);
            m_camTrackYaw   = wrapAnglePi(m_camTrackYaw + shortestAngleDiff(m_camTrackYaw, targetYaw) * rotAlpha);
            m_camTrackPitch += (targetPitch - m_camTrackPitch) * rotAlpha;
            m_camera.camYaw   = wrapAnglePi(m_camTrackYaw + m_camSwivelYaw);
            m_camera.camPitch = std::clamp(m_camTrackPitch + m_camSwivelPitch, -1.35f, 1.35f);
        } else if (m_camera.viewMode == CameraViewMode::ISOMETRIC_3D) {
            m_camera.hasCustomBasis = false;
            m_camTrackYaw   = 0.45f;
            m_camTrackPitch = 0.50f;
            m_camera.camYaw   = wrapAnglePi(m_camTrackYaw + m_camSwivelYaw);
            m_camera.camPitch = std::clamp(m_camTrackPitch + m_camSwivelPitch, -1.35f, 1.35f);
        }
    } else {
        // Classic 2D Orthographic Mode
        m_camera.hasCustomBasis = false;
        static constexpr float BASE_ZOOM_PX_PER_M = 1.0f / FT2M;
        float targetZoom = 0.55f * BASE_ZOOM_PX_PER_M;
        if (altitude < 550.0f * FT2M) {
            float factor = 1.0f - (altitude / (550.0f * FT2M));
            targetZoom = (0.55f + factor * 1.55f) * BASE_ZOOM_PX_PER_M;
        }
        m_camera.zoom += (targetZoom - m_camera.zoom) * (dt * 4.0f);
        m_camera.x += (m_battleship.getX() - m_camera.x) * (dt * 6.0f);
        m_camera.y += (m_battleship.getY() - m_camera.y) * (dt * 6.0f);
        m_camera.z += (m_battleship.getZ() - m_camera.z) * (dt * 6.0f);
        m_camera.x = std::clamp(m_camera.x, 300.0f * FT2M, m_terrain.getWorldWidth() - 300.0f * FT2M);
        m_camera.y = std::min(m_camera.y, m_terrain.getWorldHeight() - 100.0f * FT2M);
    }

    m_lastBattleshipState = m_battleship.getState();
}

void Game::render() {
    SDL_SetRenderDrawColor(m_renderer, 3, 5, 8, 255);
    SDL_RenderClear(m_renderer);

    // 1. 3D Parallax Background Starfield
    SDL_Color starColor = {190, 215, 240, 255};
    for (const auto& star : m_stars) {
        float sx, sy;
        if (m_camera.worldToScreen3D(star.x, star.y, star.z, sx, sy)) {
            if (sx >= -2 && sx < m_screenWidth + 2 && sy >= -2 && sy < m_screenHeight + 2) {
                Uint8 a = (Uint8)(star.brightness * 220.0f);
                SDL_SetRenderDrawColor(m_renderer, starColor.r, starColor.g, starColor.b, a);
                SDL_RenderDrawPoint(m_renderer, (int)sx, (int)sy);

                if (star.brightness > 0.85f) {
                    Uint8 bloomA = (Uint8)(a * 0.45f);
                    SDL_SetRenderDrawColor(m_renderer, starColor.r, starColor.g, starColor.b, bloomA);
                    SDL_RenderDrawPoint(m_renderer, (int)sx - 1, (int)sy);
                    SDL_RenderDrawPoint(m_renderer, (int)sx + 1, (int)sy);
                    SDL_RenderDrawPoint(m_renderer, (int)sx, (int)sy - 1);
                    SDL_RenderDrawPoint(m_renderer, (int)sx, (int)sy + 1);
                }
            }
        }
    }

    // 2. 3D Terrain Lattice & Multiplier Landing Decks
    m_terrain.render(m_renderer, m_camera, m_crtGlow);

    // 3. 3D Vertical Plumb-Line & Surface Target Reticle
    m_battleship.renderDropShadow(m_renderer, m_camera, m_terrain, m_crtGlow);

    // 4. 3D Battleship Wireframe, Plumes & Force Vectors
    m_battleship.render(m_renderer, m_camera, m_crtGlow);

    // 5. Vector HUD & 3D Telemetry
    renderHUD();
    renderTelemetry();
    renderFlightStatus();
    renderControllerGuide();

    // 6. CRT Phosphor Scanlines
    if (m_showScanlines) {
        VectorRenderer::drawScanlines(m_renderer, m_screenWidth, m_screenHeight, 22);
    }

    SDL_RenderPresent(m_renderer);
}

void Game::renderHUD() {
    SDL_Color hudColor = {240, 255, 240, 255};
    float scale = 2.4f;

    std::ostringstream ssScore;
    ssScore << "SCORE " << std::setfill('0') << std::setw(4) << m_score;
    VectorFont::drawText(m_renderer, ssScore.str(), 30.0f, 25.0f, scale, hudColor, m_crtGlow);

    std::ostringstream ssTime;
    ssTime << "TIME " << std::setfill('0') << std::setw(3) << (int)m_gameTime;
    VectorFont::drawText(m_renderer, ssTime.str(), 260.0f, 25.0f, scale, hudColor, m_crtGlow);

    float hull = m_battleship.getHull();
    std::ostringstream ssHull;
    ssHull << "HULL " << std::setfill('0') << std::setw(3) << (int)std::round(hull) << "%";
    SDL_Color hullColor = (hull > 60.0f) ? SDL_Color{100, 255, 120, 255} :
                         ((hull > 30.0f) ? SDL_Color{255, 200, 60, 255} : SDL_Color{255, 70, 70, 255});
    VectorFont::drawText(m_renderer, ssHull.str(), 460.0f, 25.0f, scale, hullColor, m_crtGlow);

    std::ostringstream ssFuel;
    if (m_battleship.isUnlimitedFuel()) {
        ssFuel << "FUEL UNLTD";
    } else {
        ssFuel << "FUEL " << std::setfill('0') << std::setw(4) << (int)std::round(m_battleship.getFuel()) << " KG";
    }
    SDL_Color fuelColor = m_battleship.isUnlimitedFuel() ? SDL_Color{100, 255, 140, 255} :
                         ((m_battleship.getFuel() < 150.0f * LB2KG) ? SDL_Color{255, 70, 70, 255} : hudColor);
    VectorFont::drawText(m_renderer, ssFuel.str(), 670.0f, 25.0f, scale, fuelColor, m_crtGlow);

    std::string siteStr = "SITE: " + m_terrain.getSiteName();
    VectorFont::drawText(m_renderer, siteStr, 970.0f, 25.0f, 1.8f, {180, 230, 255, 255}, m_crtGlow);

    VectorRenderer::drawLineScreen(m_renderer, 20.0f, 52.0f, m_screenWidth - 20.0f, 52.0f,
                                  {80, 140, 90, 180}, m_crtGlow);
}

void Game::renderTelemetry() {
    float baseX = m_screenWidth - 340.0f;
    float baseY = 76.0f;
    float scale = 1.75f;

    SDL_Color greenSafe = {100, 255, 120, 255};
    SDL_Color redDanger = {255, 80, 80, 255};
    SDL_Color whiteText = {220, 240, 230, 255};

    float surfaceY = m_terrain.getSurfaceHeightAt(m_battleship.getX(), m_battleship.getZ());
    float altitude = std::max(0.0f, surfaceY - (m_battleship.getY() + 12.0f * FT2M));

    // 1. ALTITUDE [m]
    std::ostringstream ssAlt;
    ssAlt << "ALTITUDE: " << std::setfill('0') << std::setw(4) << (int)std::round(altitude) << " M";
    VectorFont::drawText(m_renderer, ssAlt.str(), baseX, baseY, scale, whiteText, m_crtGlow);

    // 2. HORIZONTAL VELOCITY X & Z [m/s]
    float vx = m_battleship.getVx();
    float vz = m_battleship.getVz();
    float hSpeed = std::sqrt(vx * vx + vz * vz);
    bool hSafe = hSpeed <= Battleship::SAFE_MAX_VX;
    std::ostringstream ssH;
    ssH << "X/Z SPD : " << std::setw(2) << (int)std::round(vx) << "/" << std::setw(2) << (int)std::round(vz) << " M/S";
    VectorFont::drawText(m_renderer, ssH.str(), baseX, baseY + 26.0f, scale,
                         hSafe ? greenSafe : redDanger, m_crtGlow);

    // 3. VERTICAL VELOCITY [m/s]
    float vy = m_battleship.getVy();
    bool vSafe = vy <= Battleship::SAFE_MAX_VY;
    std::ostringstream ssV;
    ssV << "V.SPEED : " << (vy > 0 ? "v " : "^ ") << std::setfill('0') << std::setw(2) << std::abs((int)std::round(vy)) << " M/S";
    VectorFont::drawText(m_renderer, ssV.str(), baseX, baseY + 52.0f, scale,
                         vSafe ? greenSafe : redDanger, m_crtGlow);

    // 4. 3D ROLL / PITCH TILT [deg]
    float rollDeg  = m_battleship.getAngle() * 180.0f / 3.14159265f;
    float pitchDeg = m_battleship.getPitch() * 180.0f / 3.14159265f;
    bool angleSafe = m_battleship.isSafeAngle();
    std::ostringstream ssA;
    ssA << "R/P TILT: " << std::setw(2) << (int)std::round(rollDeg) << "/" << std::setw(2) << (int)std::round(pitchDeg) << "*";
    VectorFont::drawText(m_renderer, ssA.str(), baseX, baseY + 78.0f, scale,
                         angleSafe ? greenSafe : redDanger, m_crtGlow);

    // 5. HULL INTEGRITY METER [%]
    float hull = m_battleship.getHull();
    SDL_Color hullCol = (hull > 60.0f) ? greenSafe : ((hull > 30.0f) ? SDL_Color{255, 200, 60, 255} : redDanger);
    std::ostringstream ssHul;
    ssHul << "HULL    : " << std::setw(3) << (int)std::round(hull) << "%";
    VectorFont::drawText(m_renderer, ssHul.str(), baseX, baseY + 104.0f, 1.6f, hullCol, m_crtGlow);

    float hMeterX = baseX + 145.0f;
    float hMeterY = baseY + 102.0f;
    float hMeterW = 150.0f;
    float hMeterH = 10.0f;
    VectorRenderer::drawRectScreen(m_renderer, hMeterX, hMeterY, hMeterW, hMeterH, {100, 160, 120, 255}, m_crtGlow);
    if (hull > 0.0f) {
        float fillW = hMeterW * (hull / 100.0f);
        for (int line = 1; line < (int)hMeterH - 1; ++line) {
            VectorRenderer::drawLineScreen(m_renderer, hMeterX + 1, hMeterY + line,
                                          hMeterX + fillW - 1, hMeterY + line, hullCol, m_crtGlow * 0.7f);
        }
    }

    // 6. DUAL ANALOG THRUST METERS
    float tL = m_battleship.getThrottleLeft();
    float tR = m_battleship.getThrottleRight();

    VectorFont::drawText(m_renderer, "L.ENG [LT]:", baseX, baseY + 128.0f, 1.6f, whiteText, m_crtGlow);
    float meterX = baseX + 145.0f;
    float meterY1 = baseY + 126.0f;
    float meterW = 150.0f;
    float meterH = 10.0f;

    VectorRenderer::drawRectScreen(m_renderer, meterX, meterY1, meterW, meterH, {120, 180, 140, 255}, m_crtGlow);
    if (tL > 0.01f) {
        float fillW = meterW * tL;
        SDL_Color barCol = (tL > 0.85f) ? SDL_Color{255, 160, 50, 255} : SDL_Color{100, 255, 120, 255};
        for (int line = 1; line < (int)meterH - 1; ++line) {
            VectorRenderer::drawLineScreen(m_renderer, meterX + 1, meterY1 + line,
                                          meterX + fillW - 1, meterY1 + line, barCol, m_crtGlow * 0.7f);
        }
    }

    VectorFont::drawText(m_renderer, "R.ENG [RT]:", baseX, baseY + 148.0f, 1.6f, whiteText, m_crtGlow);
    float meterY2 = baseY + 146.0f;
    VectorRenderer::drawRectScreen(m_renderer, meterX, meterY2, meterW, meterH, {120, 180, 140, 255}, m_crtGlow);
    if (tR > 0.01f) {
        float fillW = meterW * tR;
        SDL_Color barCol = (tR > 0.85f) ? SDL_Color{255, 160, 50, 255} : SDL_Color{100, 255, 120, 255};
        for (int line = 1; line < (int)meterH - 1; ++line) {
            VectorRenderer::drawLineScreen(m_renderer, meterX + 1, meterY2 + line,
                                          meterX + fillW - 1, meterY2 + line, barCol, m_crtGlow * 0.7f);
        }
    }

    // 7. THROTTLE LOCK & CAMERA MODE INDICATOR
    SDL_Color lockCol = m_throttleLockMode ? SDL_Color{255, 230, 80, 255} : SDL_Color{140, 200, 160, 255};
    std::string lockStr = m_throttleLockMode ? "THR: LOCKED" : "THR: INDEP.";
    VectorFont::drawText(m_renderer, lockStr, baseX, baseY + 168.0f, 1.5f, lockCol, m_crtGlow);

    std::string camStr = m_camera.use3D ? "CAM[TAB]: 3D" : "CAM[TAB]: 2D";
    VectorFont::drawText(m_renderer, camStr, baseX + 150.0f, baseY + 168.0f, 1.5f, {160, 235, 255, 255}, m_crtGlow);

    // 8. GROUND EFFECT INDICATOR
    float ge = m_battleship.getGroundEffectMagnitude();
    SDL_Color geTextCol = (ge > 0.05f) ? SDL_Color{80, 230, 255, 255} : SDL_Color{100, 140, 120, 255};
    VectorFont::drawText(m_renderer, "G.EFFECT:", baseX, baseY + 188.0f, 1.6f, geTextCol, m_crtGlow);
    float geMeterX = baseX + 120.0f;
    float geMeterY = baseY + 186.0f;
    float geMeterW = 180.0f;
    float geMeterH = 10.0f;
    VectorRenderer::drawRectScreen(m_renderer, geMeterX, geMeterY, geMeterW, geMeterH,
                                   {60, 130, 150, 255}, m_crtGlow);
    if (ge > 0.01f) {
        float fillW = geMeterW * ge;
        for (int line = 1; line < (int)geMeterH - 1; ++line) {
            VectorRenderer::drawLineScreen(m_renderer, geMeterX + 1, geMeterY + line,
                                           geMeterX + fillW - 1, geMeterY + line,
                                           geTextCol, m_crtGlow * 0.75f);
        }
    }

    // 9. GROUND EFFECT HOVER THRUSTERS & AIR BRAKES TELEMETRY
    bool hoverOn = m_battleship.isHoverThrustersEnabled();
    float hoverPct = m_battleship.getHoverThrusterThrottle() * 100.0f;
    SDL_Color hoverCol = hoverOn ? SDL_Color{80, 235, 255, 255} : SDL_Color{110, 150, 130, 255};
    std::ostringstream ssHover;
    ssHover << "HOVER [H]: " << (hoverOn ? "ON (" : "OFF");
    if (hoverOn) {
        ssHover << std::setw(3) << (int)std::round(hoverPct) << "%)";
    }
    VectorFont::drawText(m_renderer, ssHover.str(), baseX, baseY + 208.0f, 1.6f, hoverCol, m_crtGlow);

    bool brakeOn = m_battleship.isAirBrakesActive();
    SDL_Color brakeCol = brakeOn ? SDL_Color{255, 195, 70, 255} : SDL_Color{110, 150, 130, 255};
    std::string brakeStr = brakeOn ? "BRAKE [K]: DEPLOYED" : "BRAKE [K]: OFF";
    VectorFont::drawText(m_renderer, brakeStr, baseX, baseY + 228.0f, 1.6f, brakeCol, m_crtGlow);

    bool vecOn = m_battleship.isForceVectorsVisible();
    SDL_Color vecCol = vecOn ? SDL_Color{150, 240, 190, 255} : SDL_Color{110, 150, 130, 255};
    std::string vecStr = vecOn ? "VECTORS [V]: ON" : "VECTORS [V]: OFF";
    VectorFont::drawText(m_renderer, vecStr, baseX, baseY + 248.0f, 1.6f, vecCol, m_crtGlow);

    // 10. 3D NAV-BALL / PAD ALIGNMENT RADAR WIDGET
    renderNavRadar(baseX + 150.0f, baseY + 335.0f, 52.0f);
}

void Game::renderNavRadar(float centerX, float centerY, float radius) {
    SDL_Color ringCol = {90, 175, 130, 210};
    SDL_Color crossCol = {60, 125, 90, 170};

    // Draw octagonal radar bezel
    const int N = 16;
    for (int i = 0; i < N; ++i) {
        float a1 = (float)i * (6.2831853f / N);
        float a2 = (float)(i + 1) * (6.2831853f / N);
        VectorRenderer::drawLineScreen(m_renderer,
                                       centerX + std::cos(a1) * radius, centerY + std::sin(a1) * radius,
                                       centerX + std::cos(a2) * radius, centerY + std::sin(a2) * radius,
                                       ringCol, m_crtGlow);
    }
    // Crosshairs (X = East/West, Z = North/South)
    VectorRenderer::drawLineScreen(m_renderer, centerX - radius, centerY, centerX + radius, centerY, crossCol, m_crtGlow);
    VectorRenderer::drawLineScreen(m_renderer, centerX, centerY - radius, centerX, centerY + radius, crossCol, m_crtGlow);

    // Nearest Landing Pad indicator (yellow box)
    const auto& pads = m_terrain.getLandingPads();
    if (!pads.empty()) {
        float bestDistSq = 1e12f;
        float bestDx = 0.0f, bestDz = 0.0f;
        for (const auto& pad : pads) {
            float px = 0.5f * (pad.x1 + pad.x2);
            float pz = 0.5f * (pad.z1 + pad.z2);
            float dx = px - m_battleship.getX();
            float dz = pz - m_battleship.getZ();
            float d2 = dx * dx + dz * dz;
            if (d2 < bestDistSq) {
                bestDistSq = d2;
                bestDx = dx;
                bestDz = dz;
            }
        }
        float radarRange = 120.0f; // [m]
        float padRx = std::clamp(bestDx / radarRange, -0.9f, 0.9f) * radius;
        float padRy = std::clamp(-bestDz / radarRange, -0.9f, 0.9f) * radius; // +Z is Up on radar
        VectorRenderer::drawRectScreen(m_renderer, centerX + padRx - 4.0f, centerY + padRy - 4.0f, 8.0f, 8.0f,
                                       {255, 255, 100, 255}, m_crtGlow * 1.3f);
    }

    // 3D Horizontal Velocity Vector (Vx, Vz)
    float vx = m_battleship.getVx();
    float vz = m_battleship.getVz();
    float velScale = radius / 15.0f; // 15 m/s full scale
    float vxPx = std::clamp(vx * velScale, -radius * 0.9f, radius * 0.9f);
    float vzPx = std::clamp(-vz * velScale, -radius * 0.9f, radius * 0.9f);
    SDL_Color vCol = m_battleship.isSafeSpeed() ? SDL_Color{100, 255, 140, 255} : SDL_Color{255, 90, 90, 255};
    VectorRenderer::drawLineScreen(m_renderer, centerX, centerY, centerX + vxPx, centerY + vzPx, vCol, m_crtGlow * 1.4f, 2);

    // 3D Attitude Tilt pip (Roll, Pitch)
    float rollPipX  = std::clamp(m_battleship.getAngle() / 0.5f, -0.9f, 0.9f) * radius;
    float pitchPipY = std::clamp(-m_battleship.getPitch() / 0.5f, -0.9f, 0.9f) * radius;
    SDL_Color tCol = m_battleship.isSafeAngle() ? SDL_Color{100, 230, 255, 255} : SDL_Color{255, 140, 60, 255};
    VectorRenderer::drawLineScreen(m_renderer, centerX + rollPipX - 3.0f, centerY + pitchPipY, centerX + rollPipX + 3.0f, centerY + pitchPipY, tCol, m_crtGlow, 2);
    VectorRenderer::drawLineScreen(m_renderer, centerX + rollPipX, centerY + pitchPipY - 3.0f, centerX + rollPipX, centerY + pitchPipY + 3.0f, tCol, m_crtGlow, 2);

    VectorFont::drawTextCentered(m_renderer, "3D PAD RADAR", centerX, centerY + radius + 12.0f, 1.3f, ringCol, m_crtGlow);
}

void Game::renderFlightStatus() {
    if (m_paused) {
        VectorFont::drawTextCentered(m_renderer, "** PAUSED **",
                                     m_screenWidth * 0.5f, m_screenHeight * 0.38f, 3.5f,
                                     {255, 255, 120, 255}, m_crtGlow * 1.5f);
        VectorFont::drawTextCentered(m_renderer, "PRESS START TO RESUME",
                                     m_screenWidth * 0.5f, m_screenHeight * 0.46f, 2.0f,
                                     {200, 220, 220, 255}, m_crtGlow);
        return;
    }

    if (m_messageTimer > 0.0f || m_battleship.getState() != BattleshipState::FLYING) {
        SDL_Color msgColor = {255, 255, 150, 255};
        if (m_battleship.getState() == BattleshipState::CRASHED) {
            msgColor = {255, 90, 90, 255};
        } else if (m_battleship.getState() == BattleshipState::LANDED) {
            msgColor = {120, 255, 140, 255};
        }

        VectorFont::drawTextCentered(m_renderer, m_statusMessage,
                                     m_screenWidth * 0.5f, m_screenHeight * 0.32f, 2.8f,
                                     msgColor, m_crtGlow * 1.4f);

        if (!m_statusSubMessage.empty()) {
            VectorFont::drawTextCentered(m_renderer, m_statusSubMessage,
                                         m_screenWidth * 0.5f, m_screenHeight * 0.38f, 1.8f,
                                         {220, 230, 220, 255}, m_crtGlow);
        }
    }
}

void Game::renderControllerGuide() {
    float baseY = m_screenHeight - 34.0f;
    float scale = 1.5f;
    SDL_Color guideColor = {160, 200, 180, 220};

    std::string ctrlName = m_input.isControllerConnected() ?
                           ("[CONTROLLER: " + m_input.getControllerName() + "]") :
                           "[CONTROLLER: NOT CONNECTED (KEYBOARD ACTIVE)]";
    VectorFont::drawText(m_renderer, ctrlName, 25.0f, baseY - 24.0f, 1.4f,
                         m_input.isControllerConnected() ? SDL_Color{120, 255, 140, 255} : SDL_Color{255, 200, 100, 255},
                         m_crtGlow);

    std::string controls = m_throttleLockMode
        ? "[LT/RT] LINKED  [LS/WASD] 3D PITCH&ROLL  [RS/JL] ORBIT  [TAB] CAM  [H] HOVER  [K] BRAKE"
        : "[LT/RT] L/R.THR  [LS/WASD] 3D PITCH&ROLL  [RS/JL] ORBIT  [TAB] CAM  [H] HOVER  [K] BRAKE";
    VectorFont::drawText(m_renderer, controls, 25.0f, baseY, scale, guideColor, m_crtGlow);
}
