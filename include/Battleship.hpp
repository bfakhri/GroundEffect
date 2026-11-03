#pragma once

#include "VectorRenderer.hpp"
#include "SoundEngine.hpp"
#include "Terrain.hpp"
#include <vector>

enum class BattleshipState {
    FLYING,
    LANDED,
    CRASHED
};

struct DebrisLine {
    float x1, y1;       // [m] Start coordinate of debris segment in world space
    float x2, y2;       // [m] End coordinate of debris segment in world space
    float vx, vy;       // [m/s] Linear velocity components
    float angle;        // [rad] Orientation angle
    float angularVel;   // [rad/s] Angular rotational velocity
    float life;         // [s] Remaining debris lifetime
};

struct ThrusterParticle {
    float x, y;          // [m] Position in world space
    float vx, vy;        // [m/s] Linear velocity
    float life;          // [s] Remaining lifetime
    float maxLife;       // [s] Initial total lifetime
    int bounceCount = 0; // [count] Number of terrain bounces
    SDL_Color color;     // Base color
};

struct ImpactEvent {
    bool occurred = false;
    float speed = 0.0f;           // [m/s] Impact speed magnitude
    float damageInflicted = 0.0f; // [%] Structural damage percentage accrued
};

class Battleship {
public:
    Battleship();

    void reset(float startX, float startY, float startFuel = 340.194f); // startX [m], startY [m], startFuel [kg] (converted from 750.0 lb)
    void resetLanded(float startX, float startY, float startFuel = 340.194f); // startX [m], startY [m], startFuel [kg]
    void update(float dt, float throttleLeft, float throttleRight, float rotateInput, bool abortTriggered, bool boostTriggered,
                const Terrain& terrain, SoundEngine& sound); // dt [s], throttle in [0.0 - 1.0], rotateInput in [-1.0 to +1.0]

    void render(SDL_Renderer* renderer, const Camera2D& cam, float glow = 1.0f);

    // Getters & Setters
    float getX() const { return m_x; }                   // [m] Craft horizontal position X
    float getY() const { return m_y; }                   // [m] Craft vertical position Y
    float getVx() const { return m_vx; }                 // [m/s] Craft horizontal velocity Vx
    float getVy() const { return m_vy; }                 // [m/s] Craft vertical velocity Vy
    float getAngle() const { return m_angle; }           // [rad] Pitch angle (0 rad is pointing straight up)
    float getAngularVel() const { return m_angularVel; } // [rad/s] Angular velocity
    float getFuel() const { return m_fuel; }             // [kg] Propellant mass remaining
    void addFuel(float amount) { m_fuel += amount; }     // amount in [kg]
    bool isUnlimitedFuel() const { return m_unlimitedFuel; }
    void setUnlimitedFuel(bool enabled) { m_unlimitedFuel = enabled; }
    void toggleUnlimitedFuel() { m_unlimitedFuel = !m_unlimitedFuel; }
    float getDamage() const { return m_damage; }         // [%] Structural damage accrued (0.0% to 100.0%)
    float getHull() const { return std::max(0.0f, 100.0f - m_damage); } // [%] Structural integrity remaining
    bool wasImpulseFired() const { return m_impulseBoostActive; }
    ImpactEvent getAndClearImpactEvent() {
        ImpactEvent ev = m_lastImpact;
        m_lastImpact.occurred = false;
        return ev;
    }
    BattleshipState getState() const { return m_state; }
    int getLastPadMultiplier() const { return m_lastPadMultiplier; }    // [dimensionless] (2x, 3x, 4x, 5x)
    float getThrottle() const { return (m_throttleLeft + m_throttleRight) * 0.5f; } // [0.0 - 1.0, dimensionless]
    float getThrottleLeft() const { return m_throttleLeft; }            // [0.0 - 1.0, dimensionless]
    float getThrottleRight() const { return m_throttleRight; }          // [0.0 - 1.0, dimensionless]
    float getGroundEffectMagnitude() const { return m_groundEffectMagnitude; } // [0.0 - 1.0, dimensionless]

    // Safe landing thresholds in SI units
    static constexpr float SAFE_MAX_VY = 6.096f;      // [m/s] Maximum safe vertical touchdown speed (converted from 20.0 ft/s)
    static constexpr float SAFE_MAX_VX = 4.2672f;     // [m/s] Maximum safe horizontal touchdown speed (converted from 14.0 ft/s)
    static constexpr float SAFE_MAX_ANGLE_DEG = 7.0f; // [deg] Maximum safe attitude tilt angle (0.12217 rad)

    bool isSafeSpeed() const;
    bool isSafeAngle() const;

private:
    void updatePhysics(float dt, float throttleLeft, float throttleRight, float rotateInput, bool abortTriggered, bool boostTriggered,
                       const Terrain& terrain, SoundEngine& sound);
    void checkTerrainCollision(const Terrain& terrain, SoundEngine& sound);
    void triggerCrash(SoundEngine& sound);
    void triggerLanding(int multiplier, float hitY, SoundEngine& sound);
    void spawnThrusterParticles(float dt);
    void spawnImpulseParticles();
    void updateThrusterParticles(float dt, const Terrain& terrain);

    // State
    BattleshipState m_state = BattleshipState::FLYING;
    bool m_impulseBoostActive = false; // [flag] One-frame impulse thruster fired
    float m_landedSurfaceY = 0.0f;   // [m] Surface Y datum when landed
    float m_takeoffCooldown = 0.0f;  // [s] Cooldown timer after takeoff to prevent re-landing immediately
    float m_x = 304.8f;              // [m] Craft horizontal position X (converted from 1000.0 ft)
    float m_y = 121.92f;             // [m] Craft vertical position Y (converted from 400.0 ft)
    float m_vx = 7.62f;              // [m/s] Horizontal velocity Vx (converted from 25.0 ft/s)
    float m_vy = 0.0f;               // [m/s] Vertical velocity Vy (converted from 0.0 ft/s)
    float m_angle = 0.0f;            // [rad] Attitude angle (0 rad is pointing straight up)
    float m_angularVel = 0.0f;       // [rad/s] Angular velocity
    float m_fuel = 340.194f;         // [kg] Fuel mass remaining (converted from 750.0 lb)
    bool m_unlimitedFuel = true;

    float m_throttleLeft = 0.0f;     // [0.0 - 1.0, dimensionless] Left engine throttle fraction
    float m_throttleRight = 0.0f;    // [0.0 - 1.0, dimensionless] Right engine throttle fraction
    bool m_abortActive = false;
    float m_abortTimer = 0.0f;       // [s] Abort burn timer

    // Ground effect plume impingement
    float m_groundEffectMagnitude = 0.0f; // [0.0 - 1.0, dimensionless] Normalized GE force magnitude (last frame)

    // Damage & impact tracking
    float m_damage = 0.0f;           // [%] Cumulative structural damage (0.0% undamaged to 100.0% destroyed)
    float m_impactCooldown = 0.0f;   // [s] Cooldown timer between consecutive impacts
    float m_impactFlashTimer = 0.0f; // [s] Visual flash timer upon taking hull damage
    ImpactEvent m_lastImpact;

    int m_lastPadMultiplier = 0;     // [dimensionless] Landing pad multiplier
    float m_flameFlicker = 0.0f;     // [rad] Flame flicker phase angle
    float m_rcsPuffLeft = 0.0f;      // [s] Left RCS attitude jet visual persistence timer
    float m_rcsPuffRight = 0.0f;     // [s] Right RCS attitude jet visual persistence timer

    // Vector Debris
    std::vector<DebrisLine> m_debris;
    std::vector<ThrusterParticle> m_particles;

    // Craft Local Geometry Lines (relative to center of mass)
    struct LocalLine {
        float x1, y1; // [m] Local start coordinate relative to center of mass
        float x2, y2; // [m] Local end coordinate relative to center of mass
    };
    std::vector<LocalLine> m_craftLines;
    void initCraftGeometry();
};
