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
    CHECK(terrain.getWorldWidth() > 4000.0f);
    CHECK(terrain.getWorldHeight() > 800.0f);
    CHECK(terrain.getWorldDepth() > 2000.0f);

    // Verify all 4 sites load with landing pads removed (Feature 4)
    for (int i = 0; i < (int)MissionSite::COUNT; ++i) {
        const auto& pads = terrain.getLandingPads();
        CHECK(pads.empty());
        terrain.cycleSite();
    }
    // After 4 cycles, back at Tranquility Base
    CHECK(terrain.getCurrentSite() == MissionSite::TRANQUILITY_BASE);

    // Raycast straight down onto terrain
    float midX = 600.0f * FT2M;
    float surfY = terrain.getSurfaceHeightAt(midX);
    Terrain::RayHit rayHit;
    bool hit = terrain.castRay(midX, surfY - 20.0f, 0.0f, 1.0f, 50.0f, rayHit);
    CHECK(hit);
    CHECK_NEAR(rayHit.dist, 20.0f, 0.5f);

    // Raycast with maxDist shorter than distance to ground should miss
    CHECK(!terrain.castRay(midX, surfY - 20.0f, 0.0f, 1.0f, 10.0f, rayHit));
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

    // C. Left trigger only (medium bottom VTOL thrusters) -> vertical climb along -Y with zero rotation
    ship.reset(500.0f, 200.0f);
    ship.setVelocity(0.0f, 0.0f);
    for (int i = 0; i < 30; ++i) {
        ship.update(DT, 1.0f, 0.0f, 0.0f, false, false, terrain, sound);
    }
    CHECK(ship.getVy() < -10.0f);
    CHECK_NEAR(ship.getAngle(), 0.0f, 1e-4f);
    CHECK_NEAR(ship.getVx(), 0.0f, 1e-4f);

    // D. Right trigger only (two large rear main thrusters) -> strong forward acceleration along +Z
    ship.reset(500.0f, 200.0f);
    ship.setVelocity(0.0f, 0.0f);
    for (int i = 0; i < 30; ++i) {
        ship.update(DT, 0.0f, 1.0f, 0.0f, false, false, terrain, sound);
    }
    CHECK(ship.getVz() > 15.0f);
    CHECK_NEAR(ship.getAngle(), 0.0f, 1e-4f);
    CHECK_NEAR(ship.getVx(), 0.0f, 1e-4f);

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
    // Angular velocity is unclamped and can freely spin up
    CHECK(std::abs(ship.getAngularVel()) > 2.0f);

    // Sustained full rotation exceeds previous 3.5 rad/s clamp
    {
        Battleship spinShip;
        spinShip.reset(500.0f, 200.0f, 300.0f);
        spinShip.setUnlimitedFuel(true);
        for (int i = 0; i < 60; ++i) {
            spinShip.update(DT, 0.0f, 0.0f, 1.0f, false, false, terrain, sound);
        }
        CHECK(spinShip.getAngularVel() > 3.5f); // Unclamped angular velocity
    }

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
    unbraked.setAirModeActive(false);
    braked.setAirModeActive(false);

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

    // Horizontal velocity should be gently reduced compared to unbraked (~60%-70%, subtle realistic braking)
    CHECK(braked.getVx() < unbraked.getVx() * 0.75f);
    CHECK(braked.getVx() > unbraked.getVx() * 0.50f); // Reduced effectiveness prevents unrealistic stops
    CHECK(braked.getVx() > 0.0f); // Never reverses direction

    // Terminal vertical fall speed with air brakes should be slower than unbraked free-fall
    CHECK(braked.getVy() < unbraked.getVy() * 0.80f);
    CHECK(braked.getVy() > unbraked.getVy() * 0.55f);

    // Angular velocity should also be gently damped by air brakes
    CHECK(std::abs(braked.getAngularVel()) < std::abs(unbraked.getAngularVel()) * 0.75f);
    CHECK(std::abs(braked.getAngularVel()) > std::abs(unbraked.getAngularVel()) * 0.45f);

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
    float testX = 600.0f * FT2M;
    float surfY = terrain.getSurfaceHeightAt(testX);

    // A. Liftoff from LANDED state & automatic modulation to steady hover
    {
        Battleship ship;
        ship.resetLanded(testX, surfY - 12.0f * FT2M);
        CHECK(ship.getState() == BattleshipState::LANDED);

        ship.setHoverThrustersEnabled(true);
        CHECK(ship.isHoverThrustersEnabled());

        // Simulate 6 seconds with zero manual throttle
        for (int i = 0; i < 360; ++i) {
            ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
        }

        CHECK(ship.getState() == BattleshipState::FLYING);
        CHECK_NEAR(ship.getDamage(), 0.0f, 1e-4f);

        float clearance = terrain.getSurfaceHeightAt(ship.getX(), ship.getZ()) - (ship.getY() + 12.0f * FT2M);
        CHECK_NEAR(clearance, Battleship::HOVER_TARGET_ALT, 2.5f);
        CHECK_NEAR(ship.getVy(), 0.0f, 2.0f);
        // Hover throttle should modulate around GRAVITY / HOVER_MAX_ACCEL (~0.609)
        CHECK_NEAR(ship.getHoverThrusterThrottle(), (28.0f * FT2M) / Battleship::HOVER_MAX_ACCEL, 0.12f);
    }

    // B. Slow Fall Arrest Sim Test:
    // Falling at a somewhat slow speed (7.0 m/s from 18m clearance) -> Hover thrusters catch the ship
    // before it strikes the ground!
    {
        Battleship ship;
        ship.reset(testX, surfY - 12.0f * FT2M - 18.0f);
        ship.setVelocity(0.0f, 7.0f); // Slow downward fall (7.0 m/s)
        ship.setHoverThrustersEnabled(true);

        float minClearance = 999.0f;
        for (int i = 0; i < 480; ++i) {
            ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
            float c = terrain.getSurfaceHeightAt(ship.getX(), ship.getZ()) - (ship.getY() + 12.0f * FT2M);
            minClearance = std::min(minClearance, c);
            auto imp = ship.getAndClearImpactEvent();
            CHECK(!imp.occurred);
        }

        CHECK(minClearance > 0.5f); // Never struck the ground!
        CHECK_NEAR(ship.getDamage(), 0.0f, 1e-4f);
        CHECK(ship.getState() == BattleshipState::FLYING);
        float finalClearance = terrain.getSurfaceHeightAt(ship.getX(), ship.getZ()) - (ship.getY() + 12.0f * FT2M);
        CHECK_NEAR(finalClearance, Battleship::HOVER_TARGET_ALT, 5.0f);
    }

    // C. Fast Fall Saturation Sim Test:
    // Falling at an extreme fast speed (95.0 m/s from 15m clearance) -> Exceeds magnetic damping & hover thrust,
    // so the vehicle strikes the ground!
    {
        Battleship ship;
        ship.reset(testX, surfY - 12.0f * FT2M - 15.0f);
        ship.setVelocity(0.0f, 95.0f); // Extreme downward dive (95.0 m/s)
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
        CHECK(maxThrottle - minThrottle > 0.05f); // Automatically modulated thrust over slopes
    }

    return true;
}

// ── 6. Closed-Loop Flight, Touchdown, Re-Takeoff, Rebound & Crash Sim ────────
bool testLandingReboundAndCrashSim() {
    Terrain terrain;
    SoundEngine sound;
    float testX = 600.0f * FT2M;
    float surfY = terrain.getSurfaceHeightAt(testX);

    // A. Start landed -> Take off -> Climb -> Softly land back on terrain -> Take off again
    Battleship ship;
    ship.resetLanded(testX, surfY - 12.0f * FT2M);
    CHECK(ship.getState() == BattleshipState::LANDED);

    // Fire bottom VTOL engines for 0.65s to climb ~6m above terrain
    for (int i = 0; i < 40; ++i) {
        ship.update(DT, 0.65f, 0.0f, 0.0f, false, false, terrain, sound);
    }
    CHECK(ship.getState() == BattleshipState::FLYING);
    CHECK(ship.getY() < surfY - 12.0f * FT2M - 2.0f);

    // Disable hover thrusters to allow landing descent
    ship.setHoverThrustersEnabled(false);

    // Use closed-loop VTOL throttle controller to bring vy to a gentle 3.5 m/s descent onto the terrain
    bool landedAgain = false;
    for (int i = 0; i < 600; ++i) {
        float targetVy = 3.5f; // Safe descent speed (< SAFE_MAX_VY = 6.096 m/s)
        float errVy = ship.getVy() - targetVy;
        float cmd = std::clamp(errVy * 0.25f, 0.0f, 1.0f);
        ship.update(DT, cmd, 0.0f, 0.0f, false, false, terrain, sound);
        if (ship.getState() == BattleshipState::LANDED) {
            landedAgain = true;
            break;
        }
    }
    CHECK(landedAgain);
    CHECK_NEAR(ship.getDamage(), 0.0f, 1e-4f);

    // Re-enable hover thrusters and take off a second time after landing
    ship.setHoverThrustersEnabled(true);
    for (int i = 0; i < 30; ++i) {
        ship.update(DT, 1.0f, 0.0f, 0.0f, false, false, terrain, sound);
    }
    CHECK(ship.getState() == BattleshipState::FLYING);

    // B. Survivable rough impact -> Accrues damage and bounces upward
    ship.reset(testX, surfY - 12.0f * FT2M - 2.0f);
    ship.setVelocity(2.0f, 12.0f); // Too fast for safe landing (12 m/s > 6.096 m/s), but < 100% damage
    for (int i = 0; i < 30; ++i) {
        ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
        auto imp = ship.getAndClearImpactEvent();
        if (imp.occurred) {
            CHECK(imp.speed > 5.0f);
            CHECK(imp.damageInflicted > 5.0f && imp.damageInflicted < 100.0f);
            break;
        }
    }
    CHECK(ship.getState() == BattleshipState::FLYING);
    CHECK(ship.getDamage() > 0.0f && ship.getDamage() < 100.0f);
    CHECK(ship.getVy() < 0.0f); // Rebounded upward

    // C. Fatal high-velocity crash -> 100% damage & tumbling debris
    ship.reset(testX, surfY - 12.0f * FT2M - 2.0f);
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
    float testX = 600.0f * FT2M;
    float surfY = terrain.getSurfaceHeightAt(testX);

    Battleship ship;
    // Hover just above terrain and fire thrusters so exhaust particles strike terrain and bounce
    ship.reset(testX, surfY - 12.0f * FT2M - 4.0f);
    ship.setVelocity(0.0f, 0.0f);

    int maxBounceSeen = 0;
    for (int i = 0; i < 45; ++i) {
        ship.setPosition(testX, surfY - 12.0f * FT2M - 4.0f);
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
    CHECK(game.getBattleship().getState() == BattleshipState::FLYING);

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
    CHECK(game.getBattleship().getState() == BattleshipState::FLYING);
    CHECK_NEAR(game.getBattleship().getHull(), 100.0f, 1e-4f);

    // 5. Test 3D Camera Mode Cycling ('m' / 'Tab') & 3D Pitch Input ('w')
    CHECK(game.getCamera().use3D);
    CHECK(game.getCamera().viewMode == CameraViewMode::CHASE_3D);
    pushKeyEvent(SDL_KEYDOWN, SDLK_TAB);
    game.stepFrame(DT);
    pushKeyEvent(SDL_KEYUP, SDLK_TAB);
    CHECK(game.getCamera().viewMode == CameraViewMode::ORBIT_3D);
    CHECK(game.getCamera().use3D);

    // Cycle through ISOMETRIC_3D -> CLASSIC_2D -> back to CHASE_3D
    game.cycleCameraMode();
    CHECK(game.getCamera().viewMode == CameraViewMode::ISOMETRIC_3D);
    game.cycleCameraMode();
    CHECK(game.getCamera().viewMode == CameraViewMode::CLASSIC_2D);
    CHECK(!game.getCamera().use3D);
    game.cycleCameraMode();
    CHECK(game.getCamera().viewMode == CameraViewMode::CHASE_3D);
    CHECK(game.getCamera().use3D);

    // 6. Test 6-DOF Local Ship-Frame Rocket League Camera Implementation:
    //    (A) Camera is anchored 1:1 to ship's 3D rotation matrix (rolls, pitches, yaws 1:1 with ship) at ~30m close distance
    //    (B) Right-stick swivel pans camera in local ship frame without affecting ship attitude, and snaps back when released
    {
        float testX = 600.0f * FT2M;
        float testY = game.getTerrain().getSurfaceHeightAt(testX) - 12.0f * FT2M;

        // (A) Launch ship and rotate/fly in 3D: verify 1:1 6-DOF ship-frame camera basis & close ~30m distance
        game.getBattleship().reset(testX, testY - 120.0f, 340.0f, 0.0f);
        game.getBattleship().setVelocity(0.0f, 0.0f, 0.0f);
        for (int i = 0; i < 30; ++i) {
            game.stepFrame(DT);
        }
        float baseDist = game.getCamera().getDistance();
        CHECK_NEAR(baseDist, 52.0f, 3.0f); // Farther ~52m distance even at 120m altitude (no altitude zoom-out)

        // Rotate ship in 3D (roll = 0.42 rad, pitch = 0.32 rad, yaw = 0.35 rad) with high velocity
        game.getBattleship().setAngle(0.42f);
        game.getBattleship().setPitch(0.32f);
        game.getBattleship().setYaw(0.35f);
        game.getBattleship().setVelocity(18.0f, -2.0f, 30.0f);
        for (int i = 0; i < 20; ++i) {
            game.stepFrame(DT);
        }
        CHECK(game.getCamera().hasCustomBasis);
        Vec3 camPos, right, camDown, forward;
        game.getCamera().getBasis(camPos, right, camDown, forward);
        // Verify camera right vector matches ship's 3D local right axis (1, 0, 0) 1:1
        Vec3 shipRight = game.getBattleship().localToWorldDir({1.0f, 0.0f, 0.0f}).normalized();
        CHECK_NEAR(right.dot(shipRight), 1.0f, 1e-3f);
        CHECK(std::abs(right.y) > 0.20f); // Camera rolls 1:1 with ship bank angle!
        CHECK(game.getCamera().camPitch > 0.35f);
        // Verify supersonic distance stretch from Stiffness < 1.0
        CHECK(game.getCamera().getDistance() > baseDist + 2.5f);

        // (B) Reset landed on terrain; deflect secondary camera controls ('l' yaw right, 'i' pitch up)
        //     Verify camera swivels while ship attitude remains 0, and snaps back when released!
        game.getBattleship().resetLanded(testX, testY, 340.0f, 0.0f);
        for (int i = 0; i < 30; ++i) {
            game.stepFrame(DT);
        }
        float centeredYaw = game.getCamera().camYaw;
        CHECK(std::abs(centeredYaw) < 0.05f);

        pushKeyEvent(SDL_KEYDOWN, SDLK_l);
        pushKeyEvent(SDL_KEYDOWN, SDLK_i);
        for (int i = 0; i < 15; ++i) {
            game.stepFrame(DT);
        }
        float swiveledYaw = game.getCamera().camYaw;
        CHECK(swiveledYaw > 0.80f);
        // Ship attitude is completely unaffected by secondary camera stick/keys
        CHECK_NEAR(game.getBattleship().getAngle(), 0.0f, 1e-5f);
        CHECK_NEAR(game.getBattleship().getPitch(), 0.0f, 1e-5f);
        CHECK_NEAR(game.getBattleship().getYaw(), 0.0f, 1e-5f);
        CHECK_NEAR(game.getBattleship().getAngularVel(), 0.0f, 1e-5f);
        CHECK_NEAR(game.getBattleship().getPitchVel(), 0.0f, 1e-5f);

        // Release secondary camera stick/keys -> verify Rocket League auto-centering snap-back!
        pushKeyEvent(SDL_KEYUP, SDLK_l);
        pushKeyEvent(SDL_KEYUP, SDLK_i);
        for (int i = 0; i < 30; ++i) {
            game.stepFrame(DT);
        }
        CHECK(std::abs(game.getCamera().camYaw) < 0.05f);

        // Verify camera can freely swivel below the terrain surface (no terrain floor clamping)
        pushKeyEvent(SDL_KEYDOWN, SDLK_o);
        for (int i = 0; i < 15; ++i) {
            game.stepFrame(DT);
        }
        CHECK(game.getCamera().camPitch < -0.35f);
        game.getCamera().getBasis(camPos, right, camDown, forward);
        float groundSurfY = game.getTerrain().getSurfaceHeightAt(testX);
        CHECK(camPos.y > groundSurfY); // Camera position is below terrain surface (+Y is down)
        pushKeyEvent(SDL_KEYUP, SDLK_o);
        for (int i = 0; i < 30; ++i) {
            game.stepFrame(DT);
        }

        // Verify fixed 256 Hz physics tick rate and FPS counters
        CHECK_NEAR(Game::PHYSICS_TICK_RATE, 256.0f, 1e-4f);
        CHECK_NEAR(Game::PHYSICS_DT, 1.0f / 256.0f, 1e-6f);
        int ticksBefore = game.getTotalPhysicsTicks();
        for (int i = 0; i < 60; ++i) {
            game.stepFrame(DT);
        }
        int ticksElapsed = game.getTotalPhysicsTicks() - ticksBefore;
        // 60 frames at 1/60s = 1.0s simulated time -> 256 physics ticks (+/- 1 accumulator rounding tick)
        CHECK(ticksElapsed >= 255 && ticksElapsed <= 257);
        CHECK_NEAR(game.getPhysicsFps(), 256.0f, 5.0f);
        CHECK_NEAR(game.getGraphicsFps(), 60.0f, 2.0f);
    }

    game.shutdown();
    return true;
}

// ── 9. 3D Perspective Camera, Orthonormal Basis & Clipping Test ─────────────
bool test3DCameraProjectionAndClipping() {
    Camera3D cam;
    cam.screenWidth = 1280;
    cam.screenHeight = 720;
    cam.x = 300.0f;
    cam.y = 500.0f;
    cam.z = 0.0f;
    cam.zoom = 3.28084f;
    cam.use3D = true;
    cam.camYaw = 0.25f;
    cam.camPitch = 0.30f;

    // Verify camera basis is right-handed and orthonormal for arbitrary yaw & pitch
    Vec3 camPos, right, camDown, forward;
    cam.getBasis(camPos, right, camDown, forward);
    CHECK_NEAR(right.length(), 1.0f, 1e-4f);
    CHECK_NEAR(camDown.length(), 1.0f, 1e-4f);
    CHECK_NEAR(forward.length(), 1.0f, 1e-4f);
    CHECK_NEAR(right.dot(forward), 0.0f, 1e-4f);
    CHECK_NEAR(right.dot(camDown), 0.0f, 1e-4f);
    CHECK_NEAR(camDown.dot(forward), 0.0f, 1e-4f);

    // Target center (cam.x, cam.y, cam.z) projects to exact screen center (640, 360)
    float sx = 0.0f, sy = 0.0f, depth = 0.0f;
    CHECK(cam.worldToScreen3D(cam.x, cam.y, cam.z, sx, sy, &depth));
    CHECK_NEAR(sx, 640.0f, 0.5f);
    CHECK_NEAR(sy, 360.0f, 0.5f);
    CHECK_NEAR(depth, cam.getDistance(), 0.5f);

    // Perspective foreshortening: an X offset further away in +Z projects narrower on screen than at target Z
    cam.camYaw = 0.0f;
    cam.camPitch = 0.0f;
    float sxNear = 0.0f, syNear = 0.0f;
    float sxFar = 0.0f, syFar = 0.0f;
    CHECK(cam.worldToScreen3D(cam.x + 20.0f, cam.y, 0.0f, sxNear, syNear));
    CHECK(cam.worldToScreen3D(cam.x + 20.0f, cam.y, 200.0f, sxFar, syFar));
    CHECK((sxNear - 640.0f) > (sxFar - 640.0f) * 1.3f);

    // Point behind the camera near plane returns false
    cam.getBasis(camPos, right, camDown, forward);
    Vec3 behindCam = camPos - forward * 5.0f;
    CHECK(!cam.worldToScreen3D(behindCam.x, behindCam.y, behindCam.z, sx, sy));
    return true;
}

// ── 10. 3D Terrain Heightfield, Normals & 3D Raycasting ─────────────────────
bool test3DTerrainHeightfieldAndRaycasting() {
    Terrain terrain;
    CHECK(terrain.getWorldDepth() > 2000.0f);
    CHECK(terrain.getWorldWidth() > 4000.0f);

    for (int i = 0; i < (int)MissionSite::COUNT; ++i) {
        const auto& pads = terrain.getLandingPads();
        CHECK(pads.empty()); // Landing pads removed (Feature 4)

        float testX = 600.0f * FT2M;
        float surfY = terrain.getSurfaceHeightAt(testX, 0.0f);
        Vec3 n = terrain.getSurfaceNormalAt(testX, 0.0f);
        CHECK_NEAR(n.length(), 1.0f, 0.02f);
        CHECK(n.y < -0.5f); // Normal points upward out of ground

        // 3D raycast straight down onto terrain
        Terrain::RayHit3D hit3d;
        bool hit = terrain.castRay3D({testX, surfY - 25.0f, 15.0f}, {0.0f, 1.0f, 0.0f}, 60.0f, hit3d);
        CHECK(hit);
        CHECK(!hit3d.isPad);
        CHECK_NEAR(hit3d.dist, 25.0f, 0.5f);

        // Far North/South canyon walls have 3D elevation relief compared to centerline
        float hCenter = terrain.getSurfaceHeightAt(600.0f, 0.0f);
        float hNorthWall = terrain.getSurfaceHeightAt(600.0f, terrain.getWorldDepth() * 0.4f);
        CHECK(std::abs(hNorthWall - hCenter) > 5.0f);

        terrain.cycleSite();
    }
    return true;
}

// ── 11. 6-DOF 3D Flight, Pitch RCS, 3D Hover Skating & 3D Pad Landing Sim ───
bool test3DFlightPitchHoverAndLandingSim() {
    Terrain terrain;
    SoundEngine sound;
    Battleship ship;

    // Verify 3D wireframe mesh is populated with volumetric Z segments and scaled by SHIP_SCALE (1.35x)
    CHECK(ship.getCraftLines3D().size() > 80);
    CHECK_NEAR(Battleship::SHIP_SCALE, 1.35f, 1e-4f);
    bool hasNonZeroZ = false;
    float maxAbsX = 0.0f;
    float maxAbsZ = 0.0f;
    for (const auto& l : ship.getCraftLines3D()) {
        if (std::abs(l.z1) > 1.0f || std::abs(l.z2) > 1.0f) {
            hasNonZeroZ = true;
        }
        maxAbsX = std::max({maxAbsX, std::abs(l.x1), std::abs(l.x2)});
        maxAbsZ = std::max({maxAbsZ, std::abs(l.z1), std::abs(l.z2)});
    }
    CHECK(hasNonZeroZ);
    CHECK_NEAR(maxAbsX, 23.0f * FT2M * Battleship::SHIP_SCALE, 1e-3f);
    CHECK_NEAR(maxAbsZ, 15.0f * FT2M * Battleship::SHIP_SCALE, 1e-3f);

    // A. Pitch RCS & Pitched Main Thruster Propulsion along +Z axis
    ship.reset(500.0f, 200.0f, 300.0f, 0.0f);
    ship.setVelocity(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 30; ++i) {
        ship.update(DT, 1.0f, 1.0f, 0.0f, false, false, terrain, sound, false, 1.0f, 0.0f);
    }
    CHECK(ship.getPitchVel() > 0.5f);
    CHECK(ship.getPitch() > 0.1f);
    CHECK(ship.getVz() > 1.0f); // Accelerated forward along +Z!
    CHECK(ship.getZ() > 0.2f);

    // B. 3D Emergency Abort self-rights both Roll and Pitch simultaneously
    ship.reset(500.0f, 200.0f, 300.0f, 20.0f);
    ship.setAngle(0.45f);
    ship.setPitch(-0.45f);
    ship.setVelocity(4.0f, 10.0f, -5.0f);
    ship.update(DT, 0.0f, 0.0f, 0.0f, true, false, terrain, sound);
    for (int i = 0; i < 48; ++i) {
        ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
    }
    CHECK(std::abs(ship.getAngle()) < 0.25f);
    CHECK(std::abs(ship.getPitch()) < 0.25f);
    CHECK(ship.getVy() < 0.0f);

    // C. 3D Diagonal Terrain Skating with Ground Effect Hover Thrusters across (X, Z)
    {
        float startX = 280.0f;
        float startZ = -40.0f;
        float surfY = terrain.getSurfaceHeightAt(startX, startZ);
        ship.reset(startX, surfY - 12.0f * FT2M - Battleship::HOVER_TARGET_ALT, 340.0f, startZ);
        ship.setVelocity(7.0f, 0.0f, 5.5f); // Skate diagonally North-East across 3D terrain
        ship.setHoverThrustersEnabled(true);

        for (int i = 0; i < 480; ++i) {
            ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
            CHECK_NEAR(ship.getDamage(), 0.0f, 1e-4f);
            CHECK(ship.getState() == BattleshipState::FLYING);
        }
        CHECK(ship.getX() > startX + 50.0f);
        CHECK(ship.getZ() > startZ + 35.0f);
    }

    // D. 3D Touchdown at an off-center Z coordinate
    {
        float testX = 600.0f * FT2M;
        float padZ = 18.0f; // 18 m North of centerline
        float surfY = terrain.getSurfaceHeightAt(testX, padZ);
        ship.reset(testX, surfY - 12.0f * FT2M - 3.0f, 340.0f, padZ);
        ship.setVelocity(1.0f, 3.2f, -1.0f); // Safe 3D descent speed
        ship.setHoverThrustersEnabled(false);

        bool landed3D = false;
        for (int i = 0; i < 120; ++i) {
            float cmd = std::clamp((ship.getVy() - 3.5f) * 0.25f, 0.0f, 1.0f);
            ship.update(DT, cmd, 0.0f, 0.0f, false, false, terrain, sound);
            if (ship.getState() == BattleshipState::LANDED) {
                landed3D = true;
                break;
            }
        }
        CHECK(landed3D);
        CHECK_NEAR(ship.getDamage(), 0.0f, 1e-4f);
        ship.setHoverThrustersEnabled(true);
    }

    // E. Pure Rocket / FPV Drone Dynamics — Zero Wing Lift (lift removed per specification)
    {
        ship.reset(500.0f, 120.0f, 300.0f, 0.0f);
        ship.setHoverThrustersEnabled(false);
        ship.setVelocity(0.0f, 0.5f, 5.0f);
        ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
        CHECK_NEAR(ship.getWingLiftAccel().length(), 0.0f, 1e-5f);

        ship.reset(500.0f, 120.0f, 300.0f, 0.0f);
        ship.setHoverThrustersEnabled(false);
        ship.setVelocity(0.0f, 2.0f, 20.0f);
        ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
        CHECK_NEAR(ship.getWingLiftAccel().length(), 0.0f, 1e-5f);

        ship.reset(500.0f, 120.0f, 300.0f, 0.0f);
        ship.setHoverThrustersEnabled(false);
        ship.setVelocity(0.0f, 4.0f, 40.0f);
        ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
        CHECK_NEAR(ship.getWingLiftAccel().length(), 0.0f, 1e-5f);

        // In pure ballistic flight with hover off, the ship falls strictly under gravity
        ship.reset(500.0f, 100.0f, 300.0f, 0.0f);
        ship.setHoverThrustersEnabled(false);
        ship.setPitch(-0.08f);
        ship.setVelocity(0.0f, 0.0f, 42.0f);
        float startY = ship.getY();
        for (int i = 0; i < 60; ++i) { // 1.0s ballistic flight
            ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
        }
        float drop = ship.getY() - startY;
        float expectedBallisticDrop = 0.5f * (28.0f * FT2M) * 1.0f * 1.0f; // 0.5 * g * t^2 ~ 4.27 m
        CHECK_NEAR(drop, expectedBallisticDrop, 0.25f); // Pure rocket ballistic gravity drop!
    }

    // F. Task 2.1 & 2.2: Maglev Ground Sliding & Normal-Axis Eddy-Current Magnetic Damping
    {
        // F1. Normal-axis magnetic damping acts strictly along the 3D terrain normal n_hat
        float testX = 310.0f; // Sloped canyon terrain
        float testZ = -25.0f;
        Vec3 nHat = terrain.getSurfaceNormalAt(testX, testZ);
        float surfY = terrain.getSurfaceHeightAt(testX, testZ);
        ship.reset(testX, surfY - 12.0f * FT2M - 10.0f, 300.0f, testZ);
        ship.setHoverThrustersEnabled(true);
        ship.setVelocity(15.0f, 14.0f, 20.0f); // Approaching ground with tangential + normal speed
        ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);

        Vec3 magDamp = ship.getMagneticDampingAccel();
        CHECK(magDamp.length() > 8.0f);
        // Cross product with terrain normal must be ~0 (strictly along normal n_hat!)
        Vec3 crossN = magDamp.cross(nHat);
        CHECK_NEAR(crossN.length(), 0.0f, 1e-3f);
        // Opposes approach (points in +nHat direction)
        CHECK(magDamp.dot(nHat) > 8.0f);

        // F2. Shallow high-speed flight dive transitions smoothly into Maglev slide without striking ground
        float flatX = 600.0f * FT2M;
        float surfY_F = terrain.getSurfaceHeightAt(flatX, -60.0f);
        ship.reset(flatX, surfY_F - 12.0f * FT2M - 28.0f, 340.0f, -60.0f);
        ship.setHoverThrustersEnabled(true);
        ship.setVelocity(0.0f, 12.5f, 36.0f); // Fast shallow dive (12.5 m/s sink, 36 m/s forward)
        for (int i = 0; i < 150; ++i) {
            ship.update(DT, 0.0f, 1.0f, 0.0f, false, false, terrain, sound); // Rear thrusters on!
            CHECK_NEAR(ship.getDamage(), 0.0f, 1e-4f);
        }
        // Normal damping arrested the dive and rear thrusters + Maglev boost accelerated forward speed!
        CHECK(ship.getState() == BattleshipState::FLYING);
        CHECK(ship.getVz() > 45.0f);

        // F3. Overly steep & fast dive exceeds magnetic damping and strikes the ground
        float steepY = terrain.getSurfaceHeightAt(flatX, 0.0f);
        ship.reset(flatX, steepY - 12.0f * FT2M - 18.0f, 340.0f, 0.0f);
        ship.setHoverThrustersEnabled(true);
        ship.setVelocity(0.0f, 55.0f, 5.0f); // Steep 55 m/s downward dive from 18 m exceeds magnetic damping
        bool struckGround = false;
        for (int i = 0; i < 90; ++i) {
            ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
            if (ship.getDamage() > 0.0f || ship.getState() == BattleshipState::CRASHED) {
                struckGround = true;
                break;
            }
        }
        CHECK(struckGround);
    }

    return true;
}

// ── 12. FPV Drone Controls, Air Mode, Ground Turning & Mag-Lev Tests ────────
bool testFPVDroneAndNewFeatures() {
    // 1. FPV Drone Controls via InputManager
    {
        SDL_Init(SDL_INIT_VIDEO);
        InputManager input;

        // Q key -> Bottom thrusters (throttleLeft)
        pushKeyEvent(SDL_KEYDOWN, SDLK_q);
        input.update();
        CHECK_NEAR(input.getState().throttleLeft, 1.0f, 1e-4f);
        CHECK_NEAR(input.getState().throttleRight, 0.0f, 1e-4f);
        pushKeyEvent(SDL_KEYUP, SDLK_q);

        // E key -> Rear main thrusters (throttleRight)
        pushKeyEvent(SDL_KEYDOWN, SDLK_e);
        input.update();
        CHECK_NEAR(input.getState().throttleRight, 1.0f, 1e-4f);
        CHECK_NEAR(input.getState().throttleLeft, 0.0f, 1e-4f);
        pushKeyEvent(SDL_KEYUP, SDLK_e);

        // Z key -> Yaw (Left Stick X)
        pushKeyEvent(SDL_KEYDOWN, SDLK_z);
        input.update();
        CHECK(input.getState().yawAxis < 0.0f);
        CHECK_NEAR(input.getState().rotateAxis, 0.0f, 1e-4f);
        pushKeyEvent(SDL_KEYUP, SDLK_z);

        // Left Arrow -> Roll (Right Stick X)
        pushKeyEvent(SDL_KEYDOWN, SDLK_LEFT);
        input.update();
        CHECK(input.getState().rotateAxis < 0.0f);
        pushKeyEvent(SDL_KEYUP, SDLK_LEFT);

        // W key -> Pitch (Right Stick Y)
        pushKeyEvent(SDL_KEYDOWN, SDLK_w);
        input.update();
        CHECK(input.getState().pitchAxis > 0.0f);
        pushKeyEvent(SDL_KEYUP, SDLK_w);

        // Verify analog state mapping
        InputState testState;
        testState.throttleLeft = 0.85f;
        testState.throttleRight = 0.95f;
        testState.yawAxis = 0.75f;
        testState.rotateAxis = -0.5f;
        testState.pitchAxis = 0.6f;
        input.setStateForTesting(testState);
        CHECK_NEAR(input.getState().throttleLeft, 0.85f, 1e-4f);
        CHECK_NEAR(input.getState().throttleRight, 0.95f, 1e-4f);
        CHECK_NEAR(input.getState().yawAxis, 0.75f, 1e-4f);
        CHECK_NEAR(input.getState().rotateAxis, -0.5f, 1e-4f);
        CHECK_NEAR(input.getState().pitchAxis, 0.6f, 1e-4f);
    }

    Terrain terrain;
    SoundEngine sound;

    // 2. Air Mode: stick deflection determines target rates; neutral sticks set target rate 0 & PD maintains it
    {
        Battleship ship;
        ship.reset(600.0f * FT2M, 150.0f, 300.0f, 0.0f);
        CHECK(ship.isAirModeActive());

        // A. Stick deflection determines target rates
        ship.update(DT, 0.0f, 0.0f, 0.5f, false, false, terrain, sound, false, -0.4f, 0.6f);
        CHECK_NEAR(ship.getTargetRollRate(), 0.5f * Battleship::MAX_ROLL_RATE, 1e-4f);
        CHECK_NEAR(ship.getTargetPitchRate(), -0.4f * Battleship::MAX_PITCH_RATE, 1e-4f);
        CHECK_NEAR(ship.getTargetYawRate(), 0.6f * Battleship::MAX_YAW_RATE, 1e-4f);

        // B. Rate tracking under sustained stick deflection
        for (int i = 0; i < 25; ++i) {
            ship.update(DT, 0.0f, 0.0f, 0.5f, false, false, terrain, sound, false, 0.3f, 0.0f);
        }
        CHECK_NEAR(ship.getTargetRollRate(), 0.5f * Battleship::MAX_ROLL_RATE, 1e-4f);
        CHECK_NEAR(ship.getTargetPitchRate(), 0.3f * Battleship::MAX_PITCH_RATE, 1e-4f);
        CHECK_NEAR(ship.getAngularVel(), 0.5f * Battleship::MAX_ROLL_RATE, 0.30f);
        CHECK_NEAR(ship.getPitchVel(), 0.3f * Battleship::MAX_PITCH_RATE, 0.30f);

        float rollHeld = ship.getAngle();
        float pitchHeld = ship.getPitch();
        CHECK(std::abs(rollHeld) > 0.10f);
        CHECK(std::abs(pitchHeld) > 0.05f);

        // C. Sticks returned to neutral: target rates become 0 and PD controller maintains 0 rate & held attitude
        ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound, false, 0.0f, 0.0f);
        CHECK_NEAR(ship.getTargetRollRate(), 0.0f, 1e-5f);
        CHECK_NEAR(ship.getTargetPitchRate(), 0.0f, 1e-5f);
        CHECK_NEAR(ship.getTargetYawRate(), 0.0f, 1e-5f);

        for (int i = 0; i < 60; ++i) {
            ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound, false, 0.0f, 0.0f);
        }
        CHECK_NEAR(ship.getTargetRollRate(), 0.0f, 1e-5f);
        CHECK_NEAR(ship.getTargetPitchRate(), 0.0f, 1e-5f);
        CHECK_NEAR(ship.getTargetYawRate(), 0.0f, 1e-5f);
        CHECK_NEAR(ship.getAngularVel(), 0.0f, 0.08f);
        CHECK_NEAR(ship.getPitchVel(), 0.0f, 0.08f);
        CHECK_NEAR(ship.getYawVel(), 0.0f, 0.08f);
        CHECK_NEAR(ship.getAngle(), rollHeld, 0.20f);
        CHECK_NEAR(ship.getPitch(), pitchHeld, 0.20f);

        // D. External disturbance rejection while sticks neutral: PD controller actively drives rates back to 0
        ship.setAngularVel(1.8f);
        ship.setPitchVel(-1.5f);
        for (int i = 0; i < 60; ++i) {
            ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound, false, 0.0f, 0.0f);
        }
        CHECK_NEAR(ship.getAngularVel(), 0.0f, 0.10f);
        CHECK_NEAR(ship.getPitchVel(), 0.0f, 0.10f);
        CHECK_NEAR(ship.getAngle(), rollHeld, 0.20f);
        CHECK_NEAR(ship.getPitch(), pitchHeld, 0.20f);

        // E. Verification of zero snap-back when releasing sticks
        // Spin roll up and then release: verify angular velocity damps to 0 monotonically without reversing/snapping back
        for (int i = 0; i < 15; ++i) {
            ship.update(DT, 0.0f, 0.0f, 0.6f, false, false, terrain, sound, false, 0.0f, 0.0f);
        }
        CHECK(ship.getAngularVel() > 1.5f);
        float prevRoll = ship.getAngle();
        bool snappedBack = false;
        for (int i = 0; i < 45; ++i) {
            ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound, false, 0.0f, 0.0f);
            float currRoll = ship.getAngle();
            // If the ship rotated clockwise (positive), it must never reverse backwards (snap back)
            if (currRoll < prevRoll - 0.005f) {
                snappedBack = true;
            }
            prevRoll = currRoll;
        }
        CHECK(!snappedBack);
        CHECK_NEAR(ship.getAngularVel(), 0.0f, 0.05f);

        // F. Verification of zero wing lift (pure rocket/drone dynamics)
        ship.reset(600.0f * FT2M, 150.0f, 300.0f, 0.0f);
        ship.setVelocity(0.0f, 0.0f, 30.0f); // Fast forward cruise
        ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound, false);
        Vec3 lift = ship.getWingLiftAccel();
        CHECK_NEAR(lift.length(), 0.0f, 1e-5f);
        bool hasLiftVector = false;
        for (const auto& fv : ship.getForceVectors()) {
            if (fv.label == "LIFT") {
                hasLiftVector = true;
            }
        }
        CHECK(!hasLiftVector); // No aerodynamic lift vector
    }

    // 3. Ground yaw effectiveness for turning
    {
        Battleship groundShip;
        float testX = 600.0f * FT2M;
        float groundY = terrain.getSurfaceHeightAt(testX, 0.0f);
        // Position right in ground cushion
        groundShip.reset(testX, groundY - 12.0f * FT2M - 1.5f, 300.0f, 0.0f);
        groundShip.setVelocity(0.0f, 0.0f, 25.0f); // 25 m/s forward speed

        // Steer with yaw on ground
        for (int i = 0; i < 20; ++i) {
            groundShip.update(DT, 0.0f, 0.5f, 0.0f, false, false, terrain, sound, false, 0.0f, 1.0f);
        }
        // Turning on ground should produce strong yaw rate and redirect forward speed sideways (vx)
        CHECK(std::abs(groundShip.getYawVel()) > 0.5f || std::abs(groundShip.getYaw()) > 0.2f);
        CHECK(std::abs(groundShip.getVx()) > 1.0f);
    }

    // 4. Mag-Lev always on & high magnetic damping cushion
    {
        Battleship ship;
        CHECK(ship.isHoverThrustersEnabled());
        float testX = 600.0f * FT2M;
        float groundY = terrain.getSurfaceHeightAt(testX, 0.0f);
        ship.reset(testX, groundY - 12.0f * FT2M - 20.0f, 300.0f, 0.0f);
        ship.setVelocity(0.0f, 15.0f, 0.0f); // Descending at 15 m/s toward ground

        ship.update(DT, 0.0f, 0.0f, 0.0f, false, false, terrain, sound);
        Vec3 magDamp = ship.getMagneticDampingAccel();
        CHECK(magDamp.length() > 5.0f); // Strong magnetic damping active
        bool hasMagDampVector = false;
        for (const auto& fv : ship.getForceVectors()) {
            if (fv.label == "MAG.DAMP") {
                hasMagDampVector = true;
                CHECK(fv.ay < -4.0f); // Upward deceleration opposing descent
            }
        }
        CHECK(hasMagDampVector);
    }

    // 5. Enlarged Map dimensions and pads removed
    {
        CHECK(terrain.getWorldWidth() >= 14400.0f * FT2M);
        CHECK(terrain.getWorldHeight() >= 2800.0f * FT2M);
        CHECK(terrain.getWorldDepth() >= 7200.0f * FT2M);
        CHECK(terrain.getLandingPads().empty());
    }

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
        {"3D Perspective Camera, Orthonormal Basis & Clipping", test3DCameraProjectionAndClipping},
        {"3D Terrain Heightfield, Normals, 3D Pads & Raycasting", test3DTerrainHeightfieldAndRaycasting},
        {"6-DOF 3D Flight, Pitch RCS, 3D Hover & 3D Landing Sim", test3DFlightPitchHoverAndLandingSim},
        {"FPV Drone Controls, Air Mode, Ground Yaw & Enlarged Map", testFPVDroneAndNewFeatures},
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
