#pragma once

#include "VectorRenderer.hpp"
#include "SoundEngine.hpp"
#include "Terrain.hpp"
#include <vector>
#include <string>

enum class BattleshipState {
    FLYING,
    LANDED,
    CRASHED
};

struct DebrisLine {
    float x1, y1, z1 = 0.0f; // [m] Start coordinate of debris segment in 3D world space
    float x2, y2, z2 = 0.0f; // [m] End coordinate of debris segment in 3D world space
    float vx, vy, vz = 0.0f; // [m/s] 3D Linear velocity components
    float angle;             // [rad] Orientation angle
    float angularVel;        // [rad/s] Angular rotational velocity
    float life;              // [s] Remaining debris lifetime
};

struct ThrusterParticle {
    float x, y, z = 0.0f;    // [m] 3D Position in world space
    float vx, vy, vz = 0.0f; // [m/s] 3D Linear velocity
    float life;              // [s] Remaining lifetime
    float maxLife;           // [s] Initial total lifetime
    int bounceCount = 0;     // [count] Number of terrain bounces
    SDL_Color color;         // Base color
    bool isImpulse = false;  // [flag] True for high-energy explosive impulse blast streaks
};

struct ImpactEvent {
    bool occurred = false;
    float speed = 0.0f;           // [m/s] Impact speed magnitude
    float damageInflicted = 0.0f; // [%] Structural damage percentage accrued
};

struct ForceVector {
    float wx, wy, wz = 0.0f; // [m] 3D World-space application point on hull or Center of Mass
    float ax, ay, az = 0.0f; // [m/s^2] 3D World-space force/acceleration vector
    SDL_Color color;         // Vector display color
    std::string label;       // Short force identifier tag
};

class Battleship {
public:
    Battleship();

    void reset(float startX, float startY, float startFuel = 340.194f, float startZ = 0.0f);
    void resetLanded(float startX, float startY, float startFuel = 340.194f, float startZ = 0.0f);
    void update(float dt, float throttleLeft, float throttleRight, float rotateInput, bool abortTriggered, bool boostTriggered,
                const Terrain& terrain, SoundEngine& sound, bool airBrakeHeld = false,
                float pitchInput = 0.0f, float yawInput = 0.0f);

    void render(SDL_Renderer* renderer, const Camera2D& cam, float glow = 1.0f);
    void renderDropShadow(SDL_Renderer* renderer, const Camera2D& cam, const Terrain& terrain, float glow = 1.0f) const;

    // Getters & Setters (2D + 3D)
    float getX() const { return m_x; }                   // [m] Craft East/West position X
    float getY() const { return m_y; }                   // [m] Craft vertical position Y (+Y is down)
    float getZ() const { return m_z; }                   // [m] Craft North/South depth position Z
    float getVx() const { return m_vx; }                 // [m/s] Craft horizontal velocity Vx
    float getVy() const { return m_vy; }                 // [m/s] Craft vertical velocity Vy
    float getVz() const { return m_vz; }                 // [m/s] Craft depth velocity Vz
    Vec3 getPosition3D() const { return {m_x, m_y, m_z}; }
    Vec3 getVelocity3D() const { return {m_vx, m_vy, m_vz}; }

    float getAngle() const { return m_angle; }           // [rad] Roll angle in XY plane (0 rad is vertical)
    float getAngularVel() const { return m_angularVel; } // [rad/s] Roll angular velocity
    float getPitch() const { return m_pitch; }           // [rad] Pitch angle in YZ plane (0 rad is vertical)
    float getPitchVel() const { return m_pitchVel; }     // [rad/s] Pitch angular velocity
    float getYaw() const { return m_yaw; }               // [rad] Yaw heading angle in XZ plane
    float getYawVel() const { return m_yawVel; }         // [rad/s] Yaw angular velocity

    void setPosition(float x, float y, float z = 0.0f) { m_x = x; m_y = y; m_z = z; }
    void setVelocity(float vx, float vy, float vz = 0.0f) { m_vx = vx; m_vy = vy; m_vz = vz; }
    void setAngle(float angle) { m_angle = angle; }
    void setAngularVel(float angularVel) { m_angularVel = angularVel; }
    void setPitch(float pitch) { m_pitch = pitch; }
    void setPitchVel(float pitchVel) { m_pitchVel = pitchVel; }
    void setYaw(float yaw) { m_yaw = yaw; }
    void setYawVel(float yawVel) { m_yawVel = yawVel; }

    Vec3 localToWorldDir(const Vec3& localDir) const;
    Vec3 localToWorldPoint(const Vec3& localPt) const;

    float getFuel() const { return m_fuel; }             // [kg] Propellant mass remaining
    void addFuel(float amount) { m_fuel += amount; }     // amount in [kg]
    bool isUnlimitedFuel() const { return m_unlimitedFuel; }
    void setUnlimitedFuel(bool enabled) { m_unlimitedFuel = enabled; }
    void toggleUnlimitedFuel() { m_unlimitedFuel = !m_unlimitedFuel; }
    float getDamage() const { return m_damage; }         // [%] Structural damage accrued (0.0% to 100.0%)
    float getHull() const { return std::max(0.0f, 100.0f - m_damage); } // [%] Structural integrity remaining
    bool wasImpulseFired() const { return m_impulseBoostActive; }
    bool isAbortActive() const { return m_abortActive; }

    // Air Brakes
    bool isAirBrakesActive() const { return m_airBrakesActive; }
    bool isAirBrakesToggled() const { return m_airBrakesToggled; }
    void setAirBrakes(bool enabled) {
        m_airBrakesToggled = enabled;
        m_airBrakesActive = enabled;
        if (enabled) m_airBrakeDeploy = 1.0f;
    }
    void toggleAirBrakes() { setAirBrakes(!m_airBrakesToggled); }
    float getAirBrakeDeploy() const { return m_airBrakeDeploy; }
    float getAirBrakeDragMagnitude() const { return m_airBrakeDragMagnitude; }

    // Ground Effect Hover / Maglev System & Magnetic Damping (Always ON by default)
    bool isHoverThrustersEnabled() const { return m_hoverThrustersEnabled; }
    void setHoverThrustersEnabled(bool enabled) {
        m_hoverThrustersEnabled = enabled;
    }
    void toggleHoverThrusters() { m_hoverThrustersEnabled = true; }
    float getHoverThrusterThrottle() const { return m_hoverThrusterThrottle; }
    bool isMaglevEnabled() const { return m_hoverThrustersEnabled; }
    void setMaglevEnabled(bool enabled) { m_hoverThrustersEnabled = enabled; }
    Vec3 getWingLiftAccel() const { return m_lastWingLiftAccel; }
    Vec3 getMagneticDampingAccel() const { return m_lastMagDampAccel; }

    // Air Mode (Rate Control & Attitude Hold)
    static constexpr float MAX_ROLL_RATE  = 5.0f; // [rad/s] Maximum roll rate for full stick input (~286 deg/s)
    static constexpr float MAX_PITCH_RATE = 3.5f; // [rad/s] Maximum pitch rate for full stick input (~200 deg/s)
    static constexpr float MAX_YAW_RATE   = 3.5f; // [rad/s] Maximum air yaw rate for full stick input (~200 deg/s)

    bool isAirModeActive() const { return m_airModeActive; }
    void setAirModeActive(bool active) { m_airModeActive = active; }
    float getTargetRoll() const { return m_targetRoll; }
    float getTargetPitch() const { return m_targetPitch; }
    float getTargetYaw() const { return m_targetYaw; }
    float getTargetRollRate() const { return m_targetRollRate; }
    float getTargetPitchRate() const { return m_targetPitchRate; }
    float getTargetYawRate() const { return m_targetYawRate; }
    void setTargetAttitude(float roll, float pitch, float yaw) {
        m_targetRoll = roll;
        m_targetPitch = pitch;
        m_targetYaw = yaw;
        m_targetRollRate = 0.0f;
        m_targetPitchRate = 0.0f;
        m_targetYawRate = 0.0f;
        m_rollAttitudeLocked = true;
        m_pitchAttitudeLocked = true;
        m_yawAttitudeLocked = true;
    }
    bool isRollAttitudeLocked() const { return m_rollAttitudeLocked; }
    bool isPitchAttitudeLocked() const { return m_pitchAttitudeLocked; }
    bool isYawAttitudeLocked() const { return m_yawAttitudeLocked; }

    // Force Vector Visualization Overlay (ON by default)
    bool isForceVectorsVisible() const { return m_showForceVectors; }
    void setForceVectorsVisible(bool visible) { m_showForceVectors = visible; }
    void toggleForceVectors() { m_showForceVectors = !m_showForceVectors; }
    const std::vector<ForceVector>& getForceVectors() const { return m_forceVectors; }

    const std::vector<ThrusterParticle>& getParticles() const { return m_particles; }
    const std::vector<DebrisLine>& getDebris() const { return m_debris; }

    // Craft Local 3D Geometry Lines (relative to center of mass)
    struct LocalLine3D {
        float x1, y1, z1;
        float x2, y2, z2;
    };
    const std::vector<LocalLine3D>& getCraftLines3D() const { return m_craftLines3D; }

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
    static constexpr float SAFE_MAX_VY = 6.096f;      // [m/s] Maximum safe vertical touchdown speed (20.0 ft/s)
    static constexpr float SAFE_MAX_VX = 4.2672f;     // [m/s] Maximum safe horizontal touchdown speed (14.0 ft/s)
    static constexpr float SAFE_MAX_ANGLE_DEG = 7.0f; // [deg] Maximum safe attitude tilt angle (0.12217 rad)
    static constexpr float HOVER_TARGET_ALT = 9.7536f;// [m] Target ground-effect hover clearance (32.0 ft)
    static constexpr float HOVER_MAX_ALT = 22.86f;    // [m] Max ground-effect hover engagement altitude (75.0 ft)
    static constexpr float HOVER_MAX_ACCEL = 14.0208f;// [m/s^2] Max upward acceleration of hover thrusters (46.0 ft/s^2)
    static constexpr float HULL_INERTIA = 12.774f;    // [m^2] Specific rotational moment of inertia (r_g^2)
    static constexpr float RCS_THRUST_ACCEL = 35.0f;  // [m/s^2] Max lateral thrust acceleration from RCS thruster
    static constexpr float SHIP_SCALE = 1.35f;        // [dimensionless] Hull scale factor (1.35x larger battleship)
    static constexpr float WING_LIFT_COEFF = 0.0f;   // [1/m] Zero wing lift (pure rocket/drone flight per specification)
    static constexpr float MAG_DAMP_MAX_ALT = 45.0f;  // [m] Outer altitude envelope for normal-axis magnetic damping
    static constexpr float MAG_DAMP_COEFF = 2.50f;    // [1/s] Increased eddy-current normal magnetic damping coefficient

    bool isSafeSpeed() const;
    bool isSafeAngle() const;

private:
    void applyForceAtLocalPoint3D(const Vec3& localR,
                                  const Vec3& localA,
                                  Vec3& accumWorldA,
                                  float& accumRollAccel,
                                  float& accumPitchAccel,
                                  float& accumYawAccel,
                                  const std::string& label, SDL_Color color);
    void updatePhysics(float dt, float throttleLeft, float throttleRight, float rotateInput,
                       float pitchInput, float yawInput,
                       bool abortTriggered, bool boostTriggered,
                       const Terrain& terrain, SoundEngine& sound);
    void updateHoverThrusters(float dt, float rotateInput, float pitchInput, float diffThrust,
                              const Terrain& terrain, SoundEngine& sound,
                              Vec3& accumWorldA, float& accumRollAccel, float& accumPitchAccel, float& accumYawAccel);
    void renderForceVectors(SDL_Renderer* renderer, const Camera2D& cam, float glow);
    void checkTerrainCollision(const Terrain& terrain, SoundEngine& sound);
    void triggerCrash(SoundEngine& sound);
    void triggerLanding(int multiplier, float hitY, SoundEngine& sound);
    void spawnThrusterParticles(float dt);
    void spawnHoverParticles(float dt);
    void spawnImpulseParticles();
    void updateThrusterParticles(float dt, const Terrain& terrain);

    // State
    BattleshipState m_state = BattleshipState::FLYING;
    bool m_impulseBoostActive = false; // [flag] One-frame impulse thruster fired
    float m_landedSurfaceY = 0.0f;   // [m] Surface Y datum when landed
    float m_takeoffCooldown = 0.0f;  // [s] Cooldown timer after takeoff to prevent re-landing immediately
    float m_x = 304.8f;              // [m] Craft horizontal position X (1000.0 ft)
    float m_y = 121.92f;             // [m] Craft vertical position Y (400.0 ft)
    float m_z = 0.0f;                // [m] Craft depth position Z
    float m_vx = 7.62f;              // [m/s] Horizontal velocity Vx (25.0 ft/s)
    float m_vy = 0.0f;               // [m/s] Vertical velocity Vy (0.0 ft/s)
    float m_vz = 0.0f;               // [m/s] Depth velocity Vz (0.0 ft/s)
    float m_angle = 0.0f;            // [rad] Roll attitude angle in XY plane (0 rad is pointing straight up)
    float m_angularVel = 0.0f;       // [rad/s] Roll angular velocity
    float m_pitch = 0.0f;            // [rad] Pitch attitude angle in YZ plane (0 rad is pointing straight up)
    float m_pitchVel = 0.0f;         // [rad/s] Pitch angular velocity
    float m_yaw = 0.0f;              // [rad] Yaw heading angle in XZ plane
    float m_yawVel = 0.0f;           // [rad/s] Yaw angular velocity
    float m_fuel = 340.194f;         // [kg] Fuel mass remaining (750.0 lb)
    bool m_unlimitedFuel = true;

    float m_throttleLeft = 0.0f;     // [0.0 - 1.0, dimensionless] Left engine throttle fraction
    float m_throttleRight = 0.0f;    // [0.0 - 1.0, dimensionless] Right engine throttle fraction
    bool m_abortActive = false;
    float m_abortTimer = 0.0f;       // [s] Abort burn timer

    // Air Mode attitude & rate tracking
    bool m_airModeActive = true;
    float m_targetRoll = 0.0f;
    float m_targetPitch = 0.0f;
    float m_targetYaw = 0.0f;
    float m_targetRollRate = 0.0f;
    float m_targetPitchRate = 0.0f;
    float m_targetYawRate = 0.0f;
    bool m_rollAttitudeLocked = true;
    bool m_pitchAttitudeLocked = true;
    bool m_yawAttitudeLocked = true;

    // Air Brakes state
    bool m_airBrakesToggled = false; // [flag] Persistent air brake toggle state
    bool m_airBrakesActive = false;  // [flag] Effective air brake state (toggled or held)
    float m_airBrakeDeploy = 0.0f;   // [0.0 - 1.0] Air brake flap deployment fraction
    float m_airBrakeDragMagnitude = 0.0f; // [0.0 - 1.0] Normalized drag force magnitude

    // Ground Effect Hover Thrusters / Maglev & Magnetic Damping state
    bool m_hoverThrustersEnabled = true;   // [flag] Maglev hover cushion enabled by default
    float m_hoverThrusterThrottle = 0.0f;  // [0.0 - 1.0] Modulated maglev hover output fraction
    Vec3 m_lastWingLiftAccel{0.0f, 0.0f, 0.0f}; // [m/s^2] Last tick aerodynamic wing lift acceleration
    Vec3 m_lastMagDampAccel{0.0f, 0.0f, 0.0f};  // [m/s^2] Last tick normal-axis magnetic damping acceleration

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
    float m_rcsPuffFore = 0.0f;      // [s] Fore pitch RCS jet visual persistence timer
    float m_rcsPuffAft = 0.0f;       // [s] Aft pitch RCS jet visual persistence timer

    // Vector Debris & Force Vector Overlay
    std::vector<DebrisLine> m_debris;
    std::vector<ThrusterParticle> m_particles;
    bool m_showForceVectors = true; // [flag] Force vector overlay enabled by default
    std::vector<ForceVector> m_forceVectors;

    // Craft Local 2D & 3D Geometry Lines
    struct LocalLine {
        float x1, y1;
        float x2, y2;
    };
    std::vector<LocalLine> m_craftLines;
    std::vector<LocalLine3D> m_craftLines3D;
    void initCraftGeometry();
};
