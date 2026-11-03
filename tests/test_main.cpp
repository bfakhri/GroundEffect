#include "Game.hpp"
#include "Battleship.hpp"
#include "Terrain.hpp"
#include "SoundEngine.hpp"
#include "InputManager.hpp"
#include "VectorRenderer.hpp"
#include "VectorFont.hpp"

#include <SDL2/SDL.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <functional>

namespace {

static constexpr float FT2M = 0.3048f;
static constexpr float LB2KG = 0.45359237f;
static constexpr float DT = 1.0f / 60.0f;

int g_testsRun = 0;
int g_testsFailed = 0;
int g_checksRun = 0;

#define CHECK(cond) do { \
    ++g_checksRun; \
    if (!(cond)) { \
        std::cerr << "  [FAIL] " << __FILE__ << ":" << __LINE__ << " -> CHECK(" #cond ") failed\n"; \
        return false; \
    } \
} while (0)

#define CHECK_NEAR(val, expected, tol) do { \
    ++g_checksRun; \
    float _v = (val); \
    float _e = (expected); \
    float _t = (tol); \
    if (std::abs(_v - _e) > _t || !std::isfinite(_v)) { \
        std::cerr << "  [FAIL] " << __FILE__ << ":" << __LINE__ \
                  << " -> CHECK_NEAR(" #val ", " #expected ", " #tol ") failed: got " \
                  << _v << ", expected " << _e << " (+/- " << _t << ")\n"; \
        return false; \
    } \
} while (0)

struct TestCase {
    std::string name;
    std::function<bool()> fn;
};

void pushKeyEvent(Uint32 type, SDL_Keycode sym) {
    SDL_Event ev;
    SDL_zero(ev);
    ev.type = type;
    ev.key.state = (type == SDL_KEYDOWN) ? SDL_PRESSED : SDL_RELEASED;
    ev.key.repeat = 0;
    ev.key.keysym.sym = sym;
    SDL_PushEvent(&ev);
}

// ── 1. Terrain & Raycasting Tests ───────────────────────────────────────────
bool testTerrainSitesAndCollision() {
    Terrain terrain;
    CHECK(terrain.getCurrentSite() == MissionSite::TRANQUILITY_BASE);
    CHECK(terrain.getSiteName() == "TRANQUILITY BASE");
    CHECK(terrain.getWorldWidth() > 1400.0f);
    CHECK(terrain.getWorldHeight() > 600.0f);

    // Verify all 4 sites load valid pads and surface heights
    for (int i = 0; i < (int)MissionSite::COUNT; ++i) {
        const auto& pads = terrain.getLandingPads();
        CHECK(pads.size() >= 4);
        for (const auto& pad : pads) {
            CHECK(pad.x2 > pad.x1);
            CHECK(pad.multiplier >= 2 && pad.multiplier <= 5);
            float midX = (pad.x1 + pad.x2) * 0.5f;
            CHECK_NEAR(terrain.getSurfaceHeightAt(midX), pad.y, 0.05f);

            // Vertical segment intersecting the pad should register a pad collision
            bool isPad = false;
            int mult = 0;
            float hitY = 0.0f;
            bool hit = terrain.checkCollision(midX, pad.y - 5.0f, midX, pad.y + 5.0f, isPad, mult, hitY);
            CHECK(hit);
            CHECK(isPad);
            CHECK(mult == pad.multiplier);
            CHECK_NEAR(hitY, pad.y, 0.1f);
        }
        terrain.cycleSite();
    }
    // After 4 cycles, back at Tranquility Base
    CHECK(terrain.getCurrentSite() == MissionSite::TRANQUILITY_BASE);

    // Raycast straight down onto Pad 0 of Tranquility Base
    const auto& pad0 = terrain.getLandingPads()[0];
    float midX = (pad0.x1 + pad0.x2) * 0.5f;
    Terrain::RayHit rayHit;
    bool hit = terrain.castRay(midX, pad0.y - 20.0f, 0.0f, 1.0f, 50.0f, rayHit);
    CHECK(hit);
    CHECK_NEAR(rayHit.dist, 20.0f, 0.05f);
    CHECK_NEAR(rayHit.normalX, 0.0f, 0.02f);
    CHECK_NEAR(rayHit.normalY, -1.0f, 0.02f);

    // Raycast with maxDist shorter than distance to ground should miss
    CHECK(!terrain.castRay(midX, pad0.y - 20.0f, 0.0f, 1.0f, 10.0f, rayHit));
    return true;
}

// ── 2. Battleship Free-Fall, Twin Thrusters & Differential Steering ─────────
bool testPropulsionAndDifferentialSteering() {
    Terrain terrain;
    SoundEngine sound;
    Battleship ship;

    // A. Free-fall under gravity high above terrain
    ship.reset(500.0f, 100.0f);
    ship.setVelocity(0.0f, 0.0f);
    float y0 = ship.getY();
    for (int i = 0; i < 60; ++i) {
        ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
    }
    const float expectedG = 28.0f * FT2M; // 8.5344 m/s^2
    CHECK_NEAR(ship.getVy(), expectedG * 1.0f, 0.15f);
    CHECK(ship.getY() > y0 + 4.0f);

    // B. Balanced full thrust accelerates straight up with zero rotation
    ship.reset(500.0f, 200.0f);
    ship.setVelocity(0.0f, 0.0f);
    for (int i = 0; i < 30; ++i) {
        ship.update(DT, 1.0f, 1.0f, 0.0f, false, false, terrain, sound);
    }
    CHECK(ship.getVy() < -10.0f); // Strong upward velocity
    CHECK_NEAR(ship.getAngle(), 0.0f, 1e-4f);
    CHECK_NEAR(ship.getAngularVel(), 0.0f, 1e-4f);
    CHECK(!ship.getParticles().empty()); // Exhaust particles spawned

    // C. Left thruster only -> clockwise rotation (angle > 0) and rightward velocity (vx > 0)
    ship.reset(500.0f, 200.0f);
    ship.setVelocity(0.0f, 0.0f);
    for (int i = 0; i < 30; ++i) {
        ship.update(DT, 1.0f, 0.0f, 0.0f, false, false, terrain, sound);
    }
    CHECK(ship.getAngularVel() > 0.5f);
    CHECK(ship.getAngle() > 0.1f);
    CHECK(ship.getVx() > 0.5f);

    // D. Right thruster only -> counter-clockwise rotation (angle < 0) and leftward velocity (vx < 0)
    ship.reset(500.0f, 200.0f);
    ship.setVelocity(0.0f, 0.0f);
    for (int i = 0; i < 30; ++i) {
        ship.update(DT, 0.0f, 1.0f, 0.0f, false, false, terrain, sound);
    }
    CHECK(ship.getAngularVel() < -0.5f);
    CHECK(ship.getAngle() < -0.1f);
    CHECK(ship.getVx() < -0.5f);

    // E. RCS Attitude Thrusters physically impart both rotational torque AND linear force from hull mounts:
    //    - Port RCS at (-14 ft, -8 ft) firing for clockwise turn (rotateInput = +1.0) pushes hull rightward (+X)
    ship.reset(500.0f, 200.0f);
    ship.setVelocity(0.0f, 0.0f);
    ship.update(DT, 0.0f, 0.0f, 1.0f, false, false, terrain, sound);
    CHECK(ship.getAngularVel() > 0.0f); // Clockwise torque from upper-left hull mount
    CHECK(ship.getVx() > 0.15f);        // Rightward linear acceleration on center of mass!

    //    - Starboard RCS at (+14 ft, -8 ft) firing for counter-clockwise turn (rotateInput = -1.0) pushes hull leftward (-X)
    ship.reset(500.0f, 200.0f);
    ship.setVelocity(0.0f, 0.0f);
    ship.update(DT, 0.0f, 0.0f, -1.0f, false, false, terrain, sound);
    CHECK(ship.getAngularVel() < 0.0f); // Counter-clockwise torque from upper-right hull mount
    CHECK(ship.getVx() < -0.15f);       // Leftward linear acceleration on center of mass!

    // F. Force Vector Overlay is ON by default and records all active hull forces
    CHECK(ship.isForceVectorsVisible());
    ship.reset(500.0f, 200.0f);
    ship.setHoverThrustersEnabled(true);
    ship.update(DT, 1.0f, 1.0f, 1.0f, false, false, terrain, sound, true);
    const auto& fvecs = ship.getForceVectors();
    bool hasGrav = false, hasLThr = false, hasRThr = false, hasRcsL = false, hasNet = false;
    for (const auto& fv : fvecs) {
        if (fv.label == "GRAV")  hasGrav = true;
        if (fv.label == "L.THR") hasLThr = true;
        if (fv.label == "R.THR") hasRThr = true;
        if (fv.label == "RCS.L") hasRcsL = true;
        if (fv.label == "NET")   hasNet  = true;
    }
    CHECK(hasGrav && hasLThr && hasRThr && hasRcsL && hasNet);

    return true;
}

// ── 3. Fuel Management, RCS Clamping, Emergency Abort & Impulse Boost ───────
bool testFuelAbortAndImpulseBoost() {
    Terrain terrain;
    SoundEngine sound;
    Battleship ship;

    // A. Finite fuel consumption & fuel exhaustion cutoff
    ship.reset(500.0f, 200.0f, 20.0f);
    ship.setUnlimitedFuel(false);
    float f0 = ship.getFuel();
    for (int i = 0; i < 30; ++i) {
        ship.update(DT, 1.0f, 1.0f, 1.0f, false, false, terrain, sound);
    }
    CHECK(ship.getFuel() < f0);
    CHECK(std::abs(ship.getAngularVel()) <= 3.5001f); // Clamped to MAX_ANGULAR_VEL

    // Drain fuel completely and verify engines cut out
    ship.reset(500.0f, 200.0f, 0.0f);
    ship.setUnlimitedFuel(false);
    ship.setVelocity(0.0f, 0.0f);
    ship.update(DT, 1.0f, 1.0f, 1.0f, false, false, terrain, sound);
    CHECK_NEAR(ship.getThrottleLeft(), 0.0f, 1e-5f);
    CHECK_NEAR(ship.getThrottleRight(), 0.0f, 1e-5f);
    CHECK(ship.getVy() > 0.0f); // Falling due to gravity

    // B. Emergency Abort self-rights a tilted/falling ship and climbs
    ship.reset(500.0f, 200.0f, 300.0f);
    ship.setUnlimitedFuel(true);
    ship.setAngle(0.55f);      // Tilted ~31.5 deg
    ship.setAngularVel(0.0f);
    ship.setVelocity(5.0f, 10.0f); // Falling at 10 m/s
    ship.update(DT, 0.0f, 0.0f, 0.0f, true, false, terrain, sound);
    CHECK(ship.isAbortActive());
    for (int i = 0; i < 48; ++i) {
        ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
    }
    CHECK(std::abs(ship.getAngle()) < 0.25f); // Righted close to vertical
    CHECK(ship.getVy() < 0.0f);               // Reversed fall into climb

    // C. Impulse Thruster stops a medium-velocity fall in one frame
    ship.reset(500.0f, 200.0f);
    ship.setVelocity(0.0f, 30.0f); // Fast downward fall (30 m/s)
    ship.update(DT, 0.0f, 0.0f, 0.0f, false, true, terrain, sound);
    CHECK(ship.wasImpulseFired());
    CHECK(ship.getVy() < -25.0f); // Instantaneous retro delta-V (~62.5 m/s) reversed descent

    return true;
}

// ── 4. Air Brakes Physics & Stabilization Test (Task 1) ─────────────────────
bool testAirBrakes() {
    Terrain terrain;
    SoundEngine sound;

    Battleship unbraked;
    Battleship braked;

    unbraked.reset(500.0f, 150.0f);
    braked.reset(500.0f, 150.0f);

    unbraked.setVelocity(40.0f, 20.0f);
    unbraked.setAngularVel(2.5f);

    braked.setVelocity(40.0f, 20.0f);
    braked.setAngularVel(2.5f);

    // Simulate 1.5 seconds with airBrakeHeld = true on braked ship
    for (int i = 0; i < 90; ++i) {
        unbraked.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound, false);
        braked.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound, true);
    }

    CHECK(braked.isAirBrakesActive());
    CHECK_NEAR(braked.getAirBrakeDeploy(), 1.0f, 0.01f);
    CHECK(braked.getAirBrakeDragMagnitude() > 0.0f);

    // Horizontal velocity should be drastically reduced compared to unbraked (< 25% of unbraked)
    CHECK(braked.getVx() < unbraked.getVx() * 0.25f);
    CHECK(braked.getVx() > 0.0f); // Never reverses direction

    // Terminal vertical fall speed with air brakes should be much slower than unbraked free-fall
    CHECK(braked.getVy() < unbraked.getVy() * 0.45f);

    // Angular velocity should also be heavily damped by air brakes
    CHECK(std::abs(braked.getAngularVel()) < std::abs(unbraked.getAngularVel()) * 0.25f);

    // Test toggle API as well
    braked.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound, false);
    CHECK(!braked.isAirBrakesActive());
    braked.toggleAirBrakes();
    CHECK(braked.isAirBrakesToggled());
    CHECK(braked.isAirBrakesActive());

    return true;
}

// ── 5. Ground Effect Hover Thrusters Sim Tests (Task 2) ─────────────────────
bool testGroundEffectHoverThrusters() {
    Terrain terrain;
    SoundEngine sound;
    const auto& pad0 = terrain.getLandingPads()[0];
    float padX = (pad0.x1 + pad0.x2) * 0.5f;
    float surfY = pad0.y;

    // A. Liftoff from LANDED state & automatic modulation to steady hover
    {
        Battleship ship;
        ship.resetLanded(padX, surfY - 12.0f * FT2M);
        CHECK(ship.getState() == BattleshipState::LANDED);

        ship.setHoverThrustersEnabled(true);
        CHECK(ship.isHoverThrustersEnabled());

        // Simulate 6 seconds with zero manual throttle
        for (int i = 0; i < 360; ++i) {
            ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
        }

        CHECK(ship.getState() == BattleshipState::FLYING);
        CHECK_NEAR(ship.getDamage(), 0.0f, 1e-4f);

        float clearance = surfY - (ship.getY() + 12.0f * FT2M);
        CHECK_NEAR(clearance, Battleship::HOVER_TARGET_ALT, 1.5f);
        CHECK_NEAR(ship.getVy(), 0.0f, 0.5f);
        // Hover throttle should modulate around GRAVITY / HOVER_MAX_ACCEL (~0.609)
        CHECK_NEAR(ship.getHoverThrusterThrottle(), (28.0f * FT2M) / Battleship::HOVER_MAX_ACCEL, 0.12f);
    }

    // B. Slow Fall Arrest Sim Test:
    // Falling at a somewhat slow speed (7.0 m/s from 18m clearance) -> Hover thrusters catch the ship
    // before it strikes the ground!
    {
        Battleship ship;
        ship.reset(padX, surfY - 12.0f * FT2M - 18.0f);
        ship.setVelocity(0.0f, 7.0f); // Slow downward fall (7.0 m/s)
        ship.setHoverThrustersEnabled(true);

        float minClearance = 999.0f;
        for (int i = 0; i < 300; ++i) {
            ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
            float c = surfY - (ship.getY() + 12.0f * FT2M);
            minClearance = std::min(minClearance, c);
            auto imp = ship.getAndClearImpactEvent();
            CHECK(!imp.occurred);
        }

        CHECK(minClearance > 0.5f); // Never struck the ground!
        CHECK_NEAR(ship.getDamage(), 0.0f, 1e-4f);
        CHECK(ship.getState() == BattleshipState::FLYING);
        float finalClearance = surfY - (ship.getY() + 12.0f * FT2M);
        CHECK_NEAR(finalClearance, Battleship::HOVER_TARGET_ALT, 1.5f);
    }

    // C. Fast Fall Saturation Sim Test:
    // Falling at a fast speed (22.0 m/s from 20m clearance) -> Exceeds maximum hover thrust,
    // so the vehicle strikes the ground!
    {
        Battleship ship;
        ship.reset(padX, surfY - 12.0f * FT2M - 20.0f);
        ship.setVelocity(0.0f, 22.0f); // Fast downward fall (22.0 m/s)
        ship.setHoverThrustersEnabled(true);

        bool sawMaxHoverThrust = false;
        bool struckGround = false;
        for (int i = 0; i < 120; ++i) {
            ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
            if (ship.getHoverThrusterThrottle() > 0.99f) {
                sawMaxHoverThrust = true;
            }
            if (ship.getDamage() > 0.0f || ship.getState() == BattleshipState::CRASHED) {
                struckGround = true;
                break;
            }
        }

        CHECK(sawMaxHoverThrust); // Hover thrusters fired at 100% max thrust
        CHECK(struckGround);      // Bounded max thrust could not stop the high-speed fall
    }

    // D. Terrain Skating Sim Test:
    // Leave hover thrusters ON and skate horizontally across uneven terrain (flats, hills, slopes, pads)
    {
        Battleship ship;
        float startX = 250.0f;
        float startSurfY = terrain.getSurfaceHeightAt(startX);
        ship.reset(startX, startSurfY - 12.0f * FT2M - Battleship::HOVER_TARGET_ALT);
        ship.setVelocity(8.5f, 0.0f); // Skate eastward at 8.5 m/s across Tranquility Base flats, 45-deg hill & 3X pad
        ship.setHoverThrustersEnabled(true);

        float minThrottle = 1.0f;
        float maxThrottle = 0.0f;
        for (int i = 0; i < 900; ++i) { // 15 seconds -> ~127.5 meters across flat pad, 45-deg mountain & valley
            ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
            minThrottle = std::min(minThrottle, ship.getHoverThrusterThrottle());
            maxThrottle = std::max(maxThrottle, ship.getHoverThrusterThrottle());
            CHECK_NEAR(ship.getDamage(), 0.0f, 1e-4f);
            CHECK(ship.getState() == BattleshipState::FLYING);
        }

        CHECK(ship.getX() > startX + 120.0f); // Traveled across hills and pads
        CHECK(maxThrottle - minThrottle > 0.15f); // Automatically modulated thrust over slopes
    }

    return true;
}

// ── 6. Closed-Loop Flight, Touchdown, Re-Takeoff, Rebound & Crash Sim ────────
bool testLandingReboundAndCrashSim() {
    Terrain terrain;
    SoundEngine sound;
    const auto& pad0 = terrain.getLandingPads()[0];
    float padX = (pad0.x1 + pad0.x2) * 0.5f;
    float padY = pad0.y;

    // A. Start landed -> Take off -> Climb -> Softly land back on pad -> Take off again
    Battleship ship;
    ship.resetLanded(padX, padY - 12.0f * FT2M);
    CHECK(ship.getState() == BattleshipState::LANDED);

    // Fire both engines for 0.65s to climb ~6m above pad
    for (int i = 0; i < 40; ++i) {
        ship.update(DT, 0.65f, 0.65f, 0.0f, false, false, terrain, sound);
    }
    CHECK(ship.getState() == BattleshipState::FLYING);
    CHECK(ship.getY() < padY - 12.0f * FT2M - 2.0f);

    // Use closed-loop throttle controller to bring vy to a gentle 3.5 m/s descent onto the pad
    bool landedAgain = false;
    for (int i = 0; i < 600; ++i) {
        float targetVy = 3.5f; // Safe descent speed (< SAFE_MAX_VY = 6.096 m/s)
        float errVy = ship.getVy() - targetVy;
        float cmd = std::clamp(errVy * 0.25f, 0.0f, 1.0f);
        ship.update(DT, cmd, cmd, 0.0f, false, false, terrain, sound);
        if (ship.getState() == BattleshipState::LANDED) {
            landedAgain = true;
            break;
        }
    }
    CHECK(landedAgain);
    CHECK(ship.getLastPadMultiplier() == pad0.multiplier);
    CHECK_NEAR(ship.getDamage(), 0.0f, 1e-4f);

    // Verify we can take off a second time after landing (game does not stop)
    for (int i = 0; i < 30; ++i) {
        ship.update(DT, 1.0f, 1.0f, 0.0f, false, false, terrain, sound);
    }
    CHECK(ship.getState() == BattleshipState::FLYING);

    // B. Survivable rough impact -> Accrues damage and bounces upward
    ship.reset(padX, padY - 12.0f * FT2M - 2.0f);
    ship.setVelocity(2.0f, 12.0f); // Too fast for safe landing (12 m/s > 6.096 m/s), but < 100% damage
    for (int i = 0; i < 30; ++i) {
        ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
        auto imp = ship.getAndClearImpactEvent();
        if (imp.occurred) {
            CHECK(imp.speed > 10.0f);
            CHECK(imp.damageInflicted > 5.0f && imp.damageInflicted < 100.0f);
            break;
        }
    }
    CHECK(ship.getState() == BattleshipState::FLYING);
    CHECK(ship.getDamage() > 0.0f && ship.getDamage() < 100.0f);
    CHECK(ship.getVy() < 0.0f); // Rebounded upward

    // C. Fatal high-velocity crash -> 100% damage & tumbling debris
    ship.reset(padX, padY - 12.0f * FT2M - 2.0f);
    ship.setVelocity(0.0f, 90.0f); // 90 m/s impact -> >100% damage
    for (int i = 0; i < 15; ++i) {
        ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
        if (ship.getState() == BattleshipState::CRASHED) break;
    }
    CHECK(ship.getState() == BattleshipState::CRASHED);
    CHECK_NEAR(ship.getHull(), 0.0f, 1e-4f);
    CHECK(!ship.getDebris().empty());

    return true;
}

// ── 7. Thruster Particle Terrain Ricochet Test ──────────────────────────────
bool testParticleTerrainBounces() {
    std::srand(42);
    Terrain terrain;
    SoundEngine sound;
    const auto& pad0 = terrain.getLandingPads()[0];
    float padX = (pad0.x1 + pad0.x2) * 0.5f;

    Battleship ship;
    // Hover just above pad and fire thrusters so exhaust particles strike the pad and bounce
    ship.reset(padX, pad0.y - 12.0f * FT2M - 4.0f);
    ship.setVelocity(0.0f, 0.0f);

    int maxBounceSeen = 0;
    for (int i = 0; i < 45; ++i) {
        ship.setPosition(padX, pad0.y - 12.0f * FT2M - 4.0f);
        ship.setVelocity(0.0f, 0.0f);
        ship.update(DT, 1.0f, 1.0f, 0.0f, false, (i == 0), terrain, sound);
        for (const auto& p : ship.getParticles()) {
            maxBounceSeen = std::max(maxBounceSeen, p.bounceCount);
        }
    }
    CHECK(maxBounceSeen > 0);
    return true;
}

// ── 8. Headless End-to-End SDL2 Game Loop & Framebuffer Test ────────────────
bool testHeadlessGameEndToEnd() {
    setenv("SDL_VIDEODRIVER", "dummy", 1);
    setenv("SDL_AUDIODRIVER", "dummy", 1);

    Game game;
    CHECK(game.init(640, 360));
    CHECK(game.isRunning());
    CHECK(!game.isPaused());
    CHECK(game.getBattleship().getState() == BattleshipState::LANDED);

    // Step 5 initial frames and verify non-blank framebuffer rendering
    for (int i = 0; i < 5; ++i) {
        game.stepFrame(DT);
    }

    std::vector<Uint32> pixels(640 * 360, 0);
    int readRes = SDL_RenderReadPixels(game.getRenderer(), nullptr, SDL_PIXELFORMAT_ARGB8888,
                                       pixels.data(), 640 * sizeof(Uint32));
    CHECK(readRes == 0);
    Uint32 bgPixel = pixels[0];
    int nonBgPixels = 0;
    for (Uint32 px : pixels) {
        if (px != bgPixel) ++nonBgPixels;
    }
    CHECK(nonBgPixels > 500); // HUD, terrain, stars, and battleship rendered

    // Test keyboard toggles via synthetic SDL events:
    // 1. Toggle Hover Thrusters ('h'), Throttle Lock ('t'), Air Brakes ('k'), Coin ('c')
    pushKeyEvent(SDL_KEYDOWN, SDLK_h);
    pushKeyEvent(SDL_KEYDOWN, SDLK_t);
    pushKeyEvent(SDL_KEYDOWN, SDLK_k);
    pushKeyEvent(SDL_KEYDOWN, SDLK_c);
    game.stepFrame(DT);
    pushKeyEvent(SDL_KEYUP, SDLK_h);
    pushKeyEvent(SDL_KEYUP, SDLK_t);
    pushKeyEvent(SDL_KEYUP, SDLK_k);
    pushKeyEvent(SDL_KEYUP, SDLK_c);
    game.stepFrame(DT);

    CHECK(game.getBattleship().isHoverThrustersEnabled());
    CHECK(game.isThrottleLockMode());
    CHECK(game.getBattleship().isAirBrakesToggled());
    CHECK(game.getCoinsInserted() == 1);

    // Step 30 frames: since Hover Thrusters were enabled while landed, ship automatically lifts off!
    for (int i = 0; i < 30; ++i) {
        game.stepFrame(DT);
    }
    CHECK(game.getBattleship().getState() == BattleshipState::FLYING);

    // 2. Test Pause ('p') freezes gameTime
    pushKeyEvent(SDL_KEYDOWN, SDLK_p);
    game.stepFrame(DT);
    pushKeyEvent(SDL_KEYUP, SDLK_p);
    CHECK(game.isPaused());
    float pausedTime = game.getGameTime();
    game.stepFrame(DT);
    CHECK_NEAR(game.getGameTime(), pausedTime, 1e-5f);

    // Unpause
    pushKeyEvent(SDL_KEYDOWN, SDLK_p);
    game.stepFrame(DT);
    pushKeyEvent(SDL_KEYUP, SDLK_p);
    CHECK(!game.isPaused());

    // 3. Test Mission Site Cycling ('n')
    pushKeyEvent(SDL_KEYDOWN, SDLK_n);
    game.stepFrame(DT);
    pushKeyEvent(SDL_KEYUP, SDLK_n);
    CHECK(game.getTerrain().getCurrentSite() == MissionSite::TYCHO_CRATER);
    CHECK(game.getBattleship().getState() == BattleshipState::LANDED);

    // 4. Test Crash Vibration 2.0s Maximum Cap & Restart ('r')
    game.getBattleship().reset(500.0f, game.getTerrain().getSurfaceHeightAt(500.0f) - 12.0f * FT2M - 1.0f);
    game.getBattleship().setVelocity(0.0f, 100.0f);
    for (int i = 0; i < 10; ++i) {
        game.stepFrame(DT);
        if (game.getBattleship().getState() == BattleshipState::CRASHED) break;
    }
    CHECK(game.getBattleship().getState() == BattleshipState::CRASHED);
    CHECK(game.getCrashVibrationTimer() > 0.0f && game.getCrashVibrationTimer() <= 2.0f);

    // Step 2.1 seconds (126 frames) and verify crash vibration timer decays to <= 0
    for (int i = 0; i < 126; ++i) {
        game.stepFrame(DT);
    }
    CHECK(game.getCrashVibrationTimer() <= 0.0f);

    // Restart ('r') after crash
    pushKeyEvent(SDL_KEYDOWN, SDLK_r);
    game.stepFrame(DT);
    pushKeyEvent(SDL_KEYUP, SDLK_r);
    CHECK(game.getBattleship().getState() == BattleshipState::LANDED);
    CHECK_NEAR(game.getBattleship().getHull(), 100.0f, 1e-4f);

    game.shutdown();
    return true;
}

} // namespace

int main() {
    std::vector<TestCase> suite = {
        {"Terrain Sites, Collision & Raycasting", testTerrainSitesAndCollision},
        {"Propulsion, Gravity & Differential Steering", testPropulsionAndDifferentialSteering},
        {"Fuel, RCS Clamping, Abort & Impulse Boost", testFuelAbortAndImpulseBoost},
        {"Air Brakes Aerodynamic Drag & Stabilization", testAirBrakes},
        {"Ground Effect Hover Thrusters & Terrain Skating Sim", testGroundEffectHoverThrusters},
        {"Closed-Loop Landing, Re-Takeoff, Rebound & Crash Sim", testLandingReboundAndCrashSim},
        {"Thruster Particle Terrain Ricochet Physics", testParticleTerrainBounces},
        {"Headless SDL2 End-to-End Game Loop & Framebuffer", testHeadlessGameEndToEnd},
    };

    std::cout << "====================================================\n";
    std::cout << "  GROUND EFFECT - AUTOMATED TEST & SIMULATION SUITE \n";
    std::cout << "====================================================\n";

    for (const auto& tc : suite) {
        ++g_testsRun;
        std::cout << "[RUN ] " << tc.name << " ...\n";
        if (tc.fn()) {
            std::cout << "[PASS] " << tc.name << "\n";
        } else {
            ++g_testsFailed;
            std::cout << "[FAIL] " << tc.name << "\n";
        }
    }

    std::cout << "----------------------------------------------------\n";
    std::cout << "Tests run: " << g_testsRun
              << " | Checks: " << g_checksRun
              << " | Failures: " << g_testsFailed << "\n";
    std::cout << "====================================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
