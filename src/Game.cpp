#include "Game.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

// Conversion factors between Imperial and SI units
static constexpr float FT2M = 0.3048f;           // [m/ft] Length conversion factor (feet to meters)
static constexpr float LB2KG = 0.45359237f;      // [kg/lb] Mass conversion factor (pounds to kilograms)

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

    m_window = SDL_CreateWindow("GroundEffect - Xbox 360 Edition",
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
        m_lander.resetLanded(startX, startY);
    }

    m_camera.screenWidth = m_screenWidth;   // [px]
    m_camera.screenHeight = m_screenHeight; // [px]
    m_camera.x = m_lander.getX();           // [m]
    m_camera.y = m_lander.getY();           // [m]
    m_camera.zoom = 2.10f / FT2M;           // [px/m] (high-magnification closeup view when landed)

    initStars();

    m_running = true;
    m_statusMessage = "READY FOR LAUNCH";
    m_statusSubMessage = "APPLY THROTTLE TO TAKE OFF";
    m_messageTimer = 3.5f; // [s]
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
    for (int i = 0; i < 90; ++i) {
        Star s;
        s.x = (float)(std::rand() % 4800) * FT2M; // [m] Star position across world width
        s.y = (float)(std::rand() % 1600) * FT2M; // [m] Star position across world height
        s.brightness = 0.3f + ((std::rand() % 70) / 100.0f); // [0.0 - 1.0, dimensionless]
        m_stars.push_back(s);
    }
}

void Game::run() {
    Uint64 prevTime = SDL_GetPerformanceCounter();
    const float maxDt = 0.05f; // [s]

    while (m_running) {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float dt = (float)(currentTime - prevTime) / (float)SDL_GetPerformanceFrequency(); // [s]
        prevTime = currentTime;

        if (dt > maxDt) dt = maxDt;

        handleInput();

        if (!m_paused) {
            update(dt);
        }

        render();
    }
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
        m_sound.playBeep(600.0f, 0.05f); // [Hz], [s]
    }

    if (state.toggleGlowPressed) {
        m_crtGlow = (m_crtGlow > 0.5f) ? 0.0f : 1.0f;
        m_showScanlines = (m_crtGlow > 0.5f);
        m_sound.playBeep(900.0f, 0.05f); // [Hz], [s]
    }

    // Insert Coin / Add Fuel
    if (state.addFuelPressed) {
        m_lander.addFuel(500.0f * LB2KG); // [kg] Adds 226.8 kg of propellant (converted from 500.0 lb)
        m_coinsInserted++;
        m_sound.playCoinSound();
        m_input.setRumble(0.0f, 0.4f, 150); // [ms]
        m_statusMessage = "COIN INSERTED: +227 KG FUEL";
        m_statusSubMessage = "";
        m_messageTimer = 2.0f; // [s]
    }

    // Toggle Unlimited Fuel (Testing Mode)
    if (state.toggleUnlimitedFuelPressed) {
        m_lander.toggleUnlimitedFuel();
        m_sound.playBeep(1100.0f, 0.08f); // [Hz], [s]
        m_statusMessage = m_lander.isUnlimitedFuel() ? "UNLIMITED FUEL: ACTIVE" : "UNLIMITED FUEL: OFF";
        m_statusSubMessage = "PRESS [U] TO TOGGLE TESTING FUEL";
        m_messageTimer = 2.5f; // [s]
    }

    // Toggle Throttle Lock Mode (T key / LS click)
    if (state.toggleThrottleLockPressed) {
        m_throttleLockMode = !m_throttleLockMode;
        m_sound.playBeep(m_throttleLockMode ? 1300.0f : 900.0f, 0.07f); // [Hz], [s]
        m_statusMessage = m_throttleLockMode ? "THROTTLE: LOCKED (BOTH ENGINES LINKED)" : "THROTTLE: INDEPENDENT (L/R TRIGGERS)";
        m_statusSubMessage = m_throttleLockMode ? "EITHER TRIGGER NOW CONTROLS BOTH ENGINES" : "LEFT TRIGGER = L.ENG  RIGHT TRIGGER = R.ENG";
        m_messageTimer = 2.5f; // [s]
    }

    // Cycle Landing Site
    if (state.nextSitePressed) {
        m_terrain.cycleSite();
        m_sound.playBeep(750.0f, 0.08f); // [Hz], [s]
        const auto& pads = m_terrain.getLandingPads();
        if (!pads.empty()) {
            float padCenterX = (pads[0].x1 + pads[0].x2) * 0.5f;
            float padSurfaceY = pads[0].y - 12.0f * FT2M;
            m_lander.resetLanded(padCenterX, padSurfaceY, std::max(m_lander.getFuel(), 600.0f * LB2KG));
        }
        m_camera.x = m_lander.getX();
        m_camera.y = m_lander.getY();
        m_camera.zoom = 2.10f / FT2M;
        m_statusMessage = "MISSION: " + m_terrain.getSiteName();
        m_statusSubMessage = "LANDED ON PAD - READY FOR LAUNCH";
        m_messageTimer = 3.0f; // [s]
    }

    // Restart mission
    if (state.restartPressed) {
        if (m_lander.getState() == LanderState::CRASHED || (m_lander.getState() == LanderState::LANDED && state.manualRestartPressed)) {
            float nextFuel = (m_lander.getFuel() < 250.0f * LB2KG) ? 750.0f * LB2KG : m_lander.getFuel(); // [kg]
            const auto& pads = m_terrain.getLandingPads();
            if (!pads.empty()) {
                float padCenterX = (pads[0].x1 + pads[0].x2) * 0.5f;
                float padSurfaceY = pads[0].y - 12.0f * FT2M;
                m_lander.resetLanded(padCenterX, padSurfaceY, nextFuel);
            }
            m_camera.x = m_lander.getX();
            m_camera.y = m_lander.getY();
            m_camera.zoom = 2.10f / FT2M;
            m_statusMessage = "READY FOR LAUNCH";
            m_statusSubMessage = "APPLY THROTTLE TO TAKE OFF";
            m_messageTimer = 2.5f; // [s]
            m_sound.playBeep(1000.0f, 0.1f); // [Hz], [s]
            m_scoredThisTouchdown = true;
        }
    }
}

void Game::update(float dt) {
    const auto& state = m_input.getState();

    m_gameTime += dt;

    // Apply throttle lock: whichever trigger is pressed further drives both engines
    float tLeft = state.throttleLeft;
    float tRight = state.throttleRight;
    if (m_throttleLockMode) {
        float linked = std::max(tLeft, tRight);
        tLeft = linked;
        tRight = linked;
    }

    // Update Lander with twin thruster inputs
    m_lander.update(dt, tLeft, tRight, state.rotateAxis, state.abortPressed, state.boostPressed, m_terrain, m_sound);

    // If Impulse Thruster fired, give powerful rumble and visual telemetry notification
    if (m_lander.wasImpulseFired()) {
        m_input.setRumble(0.85f, 0.90f, 180);
        m_impactRumbleHoldTimer = 0.18f;
        m_statusMessage = "IMPULSE THRUSTER!";
        m_statusSubMessage = "ONE-FRAME RETRO BOOST APPLIED";
        m_messageTimer = 1.2f;
    }

    // Process vehicle terrain impact events (vibrate controller and show damage alert)
    auto impact = m_lander.getAndClearImpactEvent();
    if (impact.occurred) {
        float speed = impact.speed; // [m/s]
        // Vibration intensity proportional to impact velocity (0.35 to 1.0)
        float intensity = std::clamp(speed / (40.0f * FT2M), 0.35f, 1.0f);
        // Duration proportional to impact velocity (140ms to 360ms)
        uint32_t durationMs = (uint32_t)(140 + std::clamp(speed * (4.5f / FT2M), 0.0f, 220.0f)); // [ms]
        m_input.setRumble(intensity, intensity * 0.9f, durationMs);
        m_impactRumbleHoldTimer = (float)durationMs / 1000.0f; // [s]

        // Visual alert on HUD
        std::ostringstream ss;
        ss << "IMPACT! +" << (int)std::round(impact.damageInflicted) 
           << "% DMG  [HULL " << (int)std::round(m_lander.getHull()) << "%]";
        m_statusMessage = ss.str();
        m_statusSubMessage = (m_lander.getHull() < 30.0f) ? "CRITICAL STRUCTURAL DAMAGE!" : "CAUTION: STRUCTURAL INTEGRITY WEAKENED";
        m_messageTimer = 1.8f; // [s]
    }

    // Controller haptic rumble based on dual thrusters and events
    if (m_lander.getState() == LanderState::FLYING) {
        if (m_impactRumbleHoldTimer > 0.0f) {
            m_impactRumbleHoldTimer -= dt; // [s]
        } else {
            float tL = m_lander.getThrottleLeft();
            float tR = m_lander.getThrottleRight();
            if (tL > 0.05f || tR > 0.05f) {
                // Proportional dual rumble: left trigger fires left low-freq motor, right trigger fires right high-freq motor
                m_input.setRumble(tL * 0.40f, tR * 0.40f, 60); // [ms]
            }
        }
    } else if (m_lander.getState() == LanderState::CRASHED) {
        // Strong crash shake
        m_input.setRumble(0.85f, 0.95f, 120); // [ms]
    }

    // Calculate altitude above surface [m]
    float surfaceY = m_terrain.getSurfaceHeightAt(m_lander.getX()); // [m]
    float altitude = std::max(0.0f, surfaceY - (m_lander.getY() + 12.0f * FT2M)); // [m] (3.66 m clearance to footpads)

    // Warning sound when low altitude and descent rate is dangerous
    if (m_lander.getState() == LanderState::FLYING && altitude < 250.0f * FT2M) { // [m] Below 76.2 m
        if (!m_lander.isSafeSpeed() || !m_lander.isSafeAngle()) {
            m_warningBeepCooldown -= dt; // [s]
            if (m_warningBeepCooldown <= 0.0f) {
                m_sound.playBeep(1400.0f, 0.06f); // [Hz], [s]
                m_warningBeepCooldown = 0.25f;    // [s]
            }
        }
    }

    // Check status changes
    if (m_lander.getState() == LanderState::LANDED) {
        if (!m_scoredThisTouchdown) {
            int mult = m_lander.getLastPadMultiplier();
            int points = 50 * mult + (int)((m_lander.getFuel() / LB2KG) * 0.25f) * mult;
            m_score += points;
            if (m_score > m_highScore) m_highScore = m_score;

            std::ostringstream ss;
            ss << "PERFECT TOUCHDOWN! +" << points << " PTS (" << mult << "X PAD)";
            m_statusMessage = ss.str();
            m_statusSubMessage = "APPLY THROTTLE TO TAKE OFF AGAIN";
            m_messageTimer = 4.0f; // [s]
            m_input.setRumble(0.2f, 0.5f, 250); // [ms]
            m_scoredThisTouchdown = true;
        }
    } else {
        m_scoredThisTouchdown = false;
    }

    if (m_lander.getState() == LanderState::CRASHED) {
        if (m_statusMessage.find("CRASHED") == std::string::npos) {
            m_statusMessage = "CRASHED! HULL DESTROYED";
            m_statusSubMessage = "PRESS [A] OR [R] TO TRY AGAIN";
            m_messageTimer = 999.0f; // [s]
        }
    }

    if (m_messageTimer > 0.0f) {
        m_messageTimer -= dt; // [s]
    }

    // 1979 Dual Camera Zoom:
    // Smooth transition from wide view down to high magnification close-up
    // Base conversion factor: 1.0 px/ft = (1.0 / FT2M) = 3.28084 px/m
    static constexpr float BASE_ZOOM_PX_PER_M = 1.0f / FT2M; // [px/m]
    float targetZoom = 0.55f * BASE_ZOOM_PX_PER_M;           // [px/m] Wide view (1.80 px/m)
    if (altitude < 550.0f * FT2M) {                          // [m] Below 167.6 m altitude
        float factor = 1.0f - (altitude / (550.0f * FT2M));  // [0.0 - 1.0, dimensionless]
        targetZoom = (0.55f + factor * 1.55f) * BASE_ZOOM_PX_PER_M; // [px/m] Up to 2.10x (6.89 px/m)
    }
    m_camera.zoom += (targetZoom - m_camera.zoom) * (dt * 4.0f);

    // Camera Center follows Lander with smooth lerp
    float targetCamX = m_lander.getX(); // [m]
    float targetCamY = m_lander.getY(); // [m]
    m_camera.x += (targetCamX - m_camera.x) * (dt * 6.0f); // [m]
    m_camera.y += (targetCamY - m_camera.y) * (dt * 6.0f); // [m]

    // Keep camera within terrain bounds
    m_camera.x = std::clamp(m_camera.x, 300.0f * FT2M, m_terrain.getWorldWidth() - 300.0f * FT2M);   // [m]
    m_camera.y = std::clamp(m_camera.y, 200.0f * FT2M, m_terrain.getWorldHeight() - 100.0f * FT2M);  // [m]
}

void Game::render() {
    // Clear screen to deep space black
    SDL_SetRenderDrawColor(m_renderer, 3, 5, 8, 255);
    SDL_RenderClear(m_renderer);

    // 1. Background Stars
    SDL_Color starColor = {180, 200, 220, 255};
    for (const auto& star : m_stars) {
        float sx, sy;
        m_camera.worldToScreen(star.x, star.y, sx, sy);
        if (sx >= 0 && sx < m_screenWidth && sy >= 0 && sy < m_screenHeight) {
            Uint8 a = (Uint8)(star.brightness * 200.0f);
            SDL_SetRenderDrawColor(m_renderer, starColor.r, starColor.g, starColor.b, a);
            SDL_RenderDrawPoint(m_renderer, (int)sx, (int)sy);
        }
    }

    // 2. Terrain & Multiplier Pads
    m_terrain.render(m_renderer, m_camera, m_crtGlow);

    // 3. Lander & Thrust Plume
    m_lander.render(m_renderer, m_camera, m_crtGlow);

    // 4. Vector HUD & Telemetry
    renderHUD();
    renderTelemetry();
    renderFlightStatus();
    renderControllerGuide();

    // 5. CRT Phosphor Scanlines
    if (m_showScanlines) {
        VectorRenderer::drawScanlines(m_renderer, m_screenWidth, m_screenHeight, 22);
    }

    SDL_RenderPresent(m_renderer);
}

void Game::renderHUD() {
    SDL_Color hudColor = {240, 255, 240, 255};
    float scale = 2.4f;

    // Top Header: SCORE | TIME | HULL | FUEL | SITE
    std::ostringstream ssScore;
    ssScore << "SCORE " << std::setfill('0') << std::setw(4) << m_score;
    VectorFont::drawText(m_renderer, ssScore.str(), 30.0f, 25.0f, scale, hudColor, m_crtGlow);

    std::ostringstream ssTime;
    ssTime << "TIME " << std::setfill('0') << std::setw(3) << (int)m_gameTime;
    VectorFont::drawText(m_renderer, ssTime.str(), 260.0f, 25.0f, scale, hudColor, m_crtGlow);

    float hull = m_lander.getHull(); // [%]
    std::ostringstream ssHull;
    ssHull << "HULL " << std::setfill('0') << std::setw(3) << (int)std::round(hull) << "%";
    SDL_Color hullColor = (hull > 60.0f) ? SDL_Color{100, 255, 120, 255} :
                         ((hull > 30.0f) ? SDL_Color{255, 200, 60, 255} : SDL_Color{255, 70, 70, 255});
    VectorFont::drawText(m_renderer, ssHull.str(), 460.0f, 25.0f, scale, hullColor, m_crtGlow);

    std::ostringstream ssFuel;
    if (m_lander.isUnlimitedFuel()) {
        ssFuel << "FUEL UNLTD";
    } else {
        ssFuel << "FUEL " << std::setfill('0') << std::setw(4) << (int)std::round(m_lander.getFuel()) << " KG";
    }
    SDL_Color fuelColor = m_lander.isUnlimitedFuel() ? SDL_Color{100, 255, 140, 255} :
                         ((m_lander.getFuel() < 150.0f * LB2KG) ? SDL_Color{255, 70, 70, 255} : hudColor); // Low fuel < 68 kg
    VectorFont::drawText(m_renderer, ssFuel.str(), 670.0f, 25.0f, scale, fuelColor, m_crtGlow);

    // Site name
    std::string siteStr = "SITE: " + m_terrain.getSiteName();
    VectorFont::drawText(m_renderer, siteStr, 970.0f, 25.0f, 1.8f, {180, 230, 255, 255}, m_crtGlow);

    // Border line under top HUD [px]
    VectorRenderer::drawLineScreen(m_renderer, 20.0f, 52.0f, m_screenWidth - 20.0f, 52.0f, 
                                  {80, 140, 90, 180}, m_crtGlow);
}

void Game::renderTelemetry() {
    float baseX = m_screenWidth - 340.0f; // [px]
    float baseY = 80.0f;                  // [px]
    float scale = 1.8f;                   // font scale

    SDL_Color greenSafe = {100, 255, 120, 255};
    SDL_Color redDanger = {255, 80, 80, 255};
    SDL_Color whiteText = {220, 240, 230, 255};

    float surfaceY = m_terrain.getSurfaceHeightAt(m_lander.getX()); // [m]
    float altitude = std::max(0.0f, surfaceY - (m_lander.getY() + 12.0f * FT2M)); // [m]

    // 1. ALTITUDE [m]
    std::ostringstream ssAlt;
    ssAlt << "ALTITUDE: " << std::setfill('0') << std::setw(4) << (int)std::round(altitude) << " M";
    VectorFont::drawText(m_renderer, ssAlt.str(), baseX, baseY, scale, whiteText, m_crtGlow);

    // 2. HORIZONTAL VELOCITY [m/s]
    float vx = m_lander.getVx(); // [m/s]
    bool hSafe = std::abs(vx) <= Lander::SAFE_MAX_VX;
    std::ostringstream ssH;
    ssH << "H.SPEED : " << (vx < 0 ? "< " : "> ") << std::setfill('0') << std::setw(2) << std::abs((int)std::round(vx)) << " M/S";
    VectorFont::drawText(m_renderer, ssH.str(), baseX, baseY + 28.0f, scale, 
                         hSafe ? greenSafe : redDanger, m_crtGlow);

    // 3. VERTICAL VELOCITY [m/s]
    float vy = m_lander.getVy(); // [m/s]
    bool vSafe = vy <= Lander::SAFE_MAX_VY;
    std::ostringstream ssV;
    ssV << "V.SPEED : " << (vy > 0 ? "v " : "^ ") << std::setfill('0') << std::setw(2) << std::abs((int)std::round(vy)) << " M/S";
    VectorFont::drawText(m_renderer, ssV.str(), baseX, baseY + 56.0f, scale, 
                         vSafe ? greenSafe : redDanger, m_crtGlow);

    // 4. TILT ANGLE [deg]
    float deg = std::abs(m_lander.getAngle() * 180.0f / 3.14159265f); // [deg]
    bool angleSafe = deg <= Lander::SAFE_MAX_ANGLE_DEG;
    std::ostringstream ssA;
    ssA << "TILT    : " << (m_lander.getAngle() < 0 ? "L " : "R ") << std::setw(2) << (int)deg << "*";
    VectorFont::drawText(m_renderer, ssA.str(), baseX, baseY + 84.0f, scale, 
                         angleSafe ? greenSafe : redDanger, m_crtGlow);

    // 5. HULL INTEGRITY METER [%]
    float hull = m_lander.getHull(); // [%]
    SDL_Color hullCol = (hull > 60.0f) ? greenSafe : ((hull > 30.0f) ? SDL_Color{255, 200, 60, 255} : redDanger);
    std::ostringstream ssHul;
    ssHul << "HULL    : " << std::setw(3) << (int)std::round(hull) << "%";
    VectorFont::drawText(m_renderer, ssHul.str(), baseX, baseY + 112.0f, 1.6f, hullCol, m_crtGlow);

    float hMeterX = baseX + 145.0f; // [px]
    float hMeterY = baseY + 110.0f; // [px]
    float hMeterW = 150.0f;         // [px]
    float hMeterH = 10.0f;          // [px]
    VectorRenderer::drawRectScreen(m_renderer, hMeterX, hMeterY, hMeterW, hMeterH, {100, 160, 120, 255}, m_crtGlow);
    if (hull > 0.0f) {
        float fillW = hMeterW * (hull / 100.0f); // [px]
        for (int line = 1; line < (int)hMeterH - 1; ++line) {
            VectorRenderer::drawLineScreen(m_renderer, hMeterX + 1, hMeterY + line, 
                                          hMeterX + fillW - 1, hMeterY + line, hullCol, m_crtGlow * 0.7f);
        }
    }

    // 6. DUAL ANALOG THRUST METERS (Independent Left & Right Trigger readouts)
    float tL = m_lander.getThrottleLeft();  // [0.0 - 1.0, dimensionless]
    float tR = m_lander.getThrottleRight(); // [0.0 - 1.0, dimensionless]

    VectorFont::drawText(m_renderer, "L.ENG [LT]:", baseX, baseY + 136.0f, 1.6f, whiteText, m_crtGlow);
    float meterX = baseX + 145.0f;  // [px]
    float meterY1 = baseY + 134.0f; // [px]
    float meterW = 150.0f;          // [px]
    float meterH = 10.0f;           // [px]

    // Draw Left meter
    VectorRenderer::drawRectScreen(m_renderer, meterX, meterY1, meterW, meterH, {120, 180, 140, 255}, m_crtGlow);
    if (tL > 0.01f) {
        float fillW = meterW * tL; // [px]
        SDL_Color barCol = (tL > 0.85f) ? SDL_Color{255, 160, 50, 255} : SDL_Color{100, 255, 120, 255};
        for (int line = 1; line < (int)meterH - 1; ++line) {
            VectorRenderer::drawLineScreen(m_renderer, meterX + 1, meterY1 + line, 
                                          meterX + fillW - 1, meterY1 + line, barCol, m_crtGlow * 0.7f);
        }
    }

    // Draw Right meter
    VectorFont::drawText(m_renderer, "R.ENG [RT]:", baseX, baseY + 156.0f, 1.6f, whiteText, m_crtGlow);
    float meterY2 = baseY + 154.0f; // [px]
    VectorRenderer::drawRectScreen(m_renderer, meterX, meterY2, meterW, meterH, {120, 180, 140, 255}, m_crtGlow);
    if (tR > 0.01f) {
        float fillW = meterW * tR; // [px]
        SDL_Color barCol = (tR > 0.85f) ? SDL_Color{255, 160, 50, 255} : SDL_Color{100, 255, 120, 255};
        for (int line = 1; line < (int)meterH - 1; ++line) {
            VectorRenderer::drawLineScreen(m_renderer, meterX + 1, meterY2 + line, 
                                          meterX + fillW - 1, meterY2 + line, barCol, m_crtGlow * 0.7f);
        }
    }

    // 7. THROTTLE LOCK MODE INDICATOR
    SDL_Color lockCol = m_throttleLockMode ? SDL_Color{255, 230, 80, 255} : SDL_Color{140, 200, 160, 255};
    std::string lockStr = m_throttleLockMode ? "THR: LOCKED" : "THR: INDEP.";
    VectorFont::drawText(m_renderer, lockStr, baseX, baseY + 175.0f, 1.6f, lockCol, m_crtGlow);

    // 8. GROUND EFFECT INDICATOR
    float ge = m_lander.getGroundEffectMagnitude(); // [0.0 - 1.0, dimensionless]
    SDL_Color geTextCol = (ge > 0.05f) ? SDL_Color{80, 230, 255, 255} : SDL_Color{100, 140, 120, 255};
    VectorFont::drawText(m_renderer, "G.EFFECT:", baseX, baseY + 195.0f, 1.6f, geTextCol, m_crtGlow);
    float geMeterX = baseX + 120.0f; // [px]
    float geMeterY = baseY + 193.0f; // [px]
    float geMeterW = 180.0f;         // [px]
    float geMeterH = 10.0f;          // [px]
    VectorRenderer::drawRectScreen(m_renderer, geMeterX, geMeterY, geMeterW, geMeterH,
                                   {60, 130, 150, 255}, m_crtGlow);
    if (ge > 0.01f) {
        float fillW = geMeterW * ge; // [px]
        for (int line = 1; line < (int)geMeterH - 1; ++line) {
            VectorRenderer::drawLineScreen(m_renderer, geMeterX + 1, geMeterY + line,
                                           geMeterX + fillW - 1, geMeterY + line,
                                           geTextCol, m_crtGlow * 0.75f);
        }
    }
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

    if (m_messageTimer > 0.0f || m_lander.getState() != LanderState::FLYING) {
        SDL_Color msgColor = {255, 255, 150, 255};
        if (m_lander.getState() == LanderState::CRASHED) {
            msgColor = {255, 90, 90, 255};
        } else if (m_lander.getState() == LanderState::LANDED) {
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
    float scale = 1.6f;
    SDL_Color guideColor = {160, 200, 180, 220};

    // Controller Status Badge
    std::string ctrlName = m_input.isControllerConnected() ? 
                           ("[CONTROLLER: " + m_input.getControllerName() + "]") : 
                           "[CONTROLLER: NOT CONNECTED (KEYBOARD ACTIVE)]";
    VectorFont::drawText(m_renderer, ctrlName, 25.0f, baseY - 24.0f, 1.4f, 
                         m_input.isControllerConnected() ? SDL_Color{120, 255, 140, 255} : SDL_Color{255, 200, 100, 255}, 
                         m_crtGlow);

    // Control Mapping Banner
    std::string controls = m_throttleLockMode
        ? "[LT/RT] LINKED THRUST  [LS] ROTATE  [A/SHIFT] IMPULSE BOOST  [B] ABORT  [U] FUEL"
        : "[LT] L.THRUST  [RT] R.THRUST  [LS] ROTATE  [A/SHIFT] IMPULSE BOOST  [B] ABORT  [U] FUEL";
    VectorFont::drawText(m_renderer, controls, 25.0f, baseY, scale, guideColor, m_crtGlow);
}
