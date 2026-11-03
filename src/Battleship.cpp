#include "Battleship.hpp"
#include "VectorFont.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

// Conversion factors between Imperial and SI units
static constexpr float FT2M = 0.3048f;           // [m/ft] Length conversion factor (feet to meters)
static constexpr float LB2KG = 0.45359237f;      // [kg/lb] Mass conversion factor (pounds to kilograms)

static const float GRAVITY = 28.0f * FT2M;           // [m/s^2] Downward gravitational acceleration: 8.5344 m/s^2 (converted from 28.0 ft/s^2)
static const float THRUST_PER_ENGINE = 60.0f * FT2M; // [m/s^2] Main engine thrust acceleration: 18.288 m/s^2 (converted from 60.0 ft/s^2)
static const float THRUST_EXPONENT = 1.6f;           // [dimensionless] Slightly exponential throttle curve for precision feathering
static const float RCS_FUEL_BURN_RATE = 2.5f * LB2KG;// [kg/s] Fuel consumed by RCS thrusters per second at full rotation: 1.134 kg/s
static const float FUEL_BURN_RATE = 28.0f * LB2KG;   // [kg/s] Fuel consumed at 100% dual throttle per second: 12.701 kg/s
static const float DAMAGE_PER_VELOCITY = 0.40f / FT2M;// [%/(m/s)] Structural damage percentage accrued per m/s impact speed: 1.3123 %/(m/s)
static const float IMPULSE_DELTA_V = 205.0f * FT2M;  // [m/s] Impulse Thruster velocity change: 62.484 m/s
static const float RAD2DEG = 180.0f / 3.1415926535f; // [deg/rad] Radians to degrees conversion factor

static inline float applyThrustCurve(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return std::pow(t, THRUST_EXPONENT);
}

static void wrapAngleRad(float& a) {
    while (a > 3.14159265f) a -= 6.2831853f;
    while (a < -3.14159265f) a += 6.2831853f;
}

Battleship::Battleship() {
    initCraftGeometry();
    resetLanded(975.0f * FT2M, (1720.0f - 12.0f) * FT2M, 750.0f * LB2KG, 0.0f);
}

Vec3 Battleship::localToWorldDir(const Vec3& localDir) const {
    // 1. Roll (m_angle) in XY plane
    float cR = std::cos(m_angle);
    float sR = std::sin(m_angle);
    float x1 = localDir.x * cR - localDir.y * sR;
    float y1 = localDir.x * sR + localDir.y * cR;
    float z1 = localDir.z;

    // 2. Pitch (m_pitch) in YZ plane (positive pitch tilts top -Y toward +Z)
    float cP = std::cos(m_pitch);
    float sP = std::sin(m_pitch);
    float x2 = x1;
    float y2 = y1 * cP + z1 * sP;
    float z2 = -y1 * sP + z1 * cP;

    // 3. Yaw (m_yaw) in XZ plane
    float cY = std::cos(m_yaw);
    float sY = std::sin(m_yaw);
    float wx = x2 * cY + z2 * sY;
    float wy = y2;
    float wz = -x2 * sY + z2 * cY;

    return {wx, wy, wz};
}

Vec3 Battleship::localToWorldPoint(const Vec3& localPt) const {
    Vec3 d = localToWorldDir(localPt);
    return {m_x + d.x, m_y + d.y, m_z + d.z};
}

void Battleship::initCraftGeometry() {
    m_craftLines.clear();
    m_craftLines3D.clear();

    // ── 2D Profile Geometry (Z=0 Silhouette) ────────────────────────────────
    // 1. Armored Citadel & Main Hull
    m_craftLines.push_back({-11.0f * FT2M,  0.0f * FT2M,  11.0f * FT2M,  0.0f * FT2M});
    m_craftLines.push_back({-15.0f * FT2M, -6.0f * FT2M,  15.0f * FT2M, -6.0f * FT2M});
    m_craftLines.push_back({-12.0f * FT2M, -12.0f * FT2M, 12.0f * FT2M, -12.0f * FT2M});
    m_craftLines.push_back({-11.0f * FT2M,  0.0f * FT2M, -16.0f * FT2M, -4.0f * FT2M});
    m_craftLines.push_back({ 11.0f * FT2M,  0.0f * FT2M,  16.0f * FT2M, -4.0f * FT2M});
    m_craftLines.push_back({-16.0f * FT2M, -4.0f * FT2M, -15.0f * FT2M, -8.0f * FT2M});
    m_craftLines.push_back({ 16.0f * FT2M, -4.0f * FT2M,  15.0f * FT2M, -8.0f * FT2M});
    m_craftLines.push_back({-15.0f * FT2M, -8.0f * FT2M, -12.0f * FT2M, -12.0f * FT2M});
    m_craftLines.push_back({ 15.0f * FT2M, -8.0f * FT2M,  12.0f * FT2M, -12.0f * FT2M});
    m_craftLines.push_back({-16.0f * FT2M, -4.0f * FT2M,  16.0f * FT2M, -4.0f * FT2M});

    // 2. Command Bridge Superstructure & Sensor Mast
    m_craftLines.push_back({-10.0f * FT2M, -12.0f * FT2M, -6.0f * FT2M, -18.0f * FT2M});
    m_craftLines.push_back({ 10.0f * FT2M, -12.0f * FT2M,  6.0f * FT2M, -18.0f * FT2M});
    m_craftLines.push_back({-6.0f * FT2M, -18.0f * FT2M,  6.0f * FT2M, -18.0f * FT2M});
    m_craftLines.push_back({-6.0f * FT2M, -18.0f * FT2M, -4.0f * FT2M, -20.5f * FT2M});
    m_craftLines.push_back({ 6.0f * FT2M, -18.0f * FT2M,  4.0f * FT2M, -20.5f * FT2M});
    m_craftLines.push_back({-4.0f * FT2M, -20.5f * FT2M,  4.0f * FT2M, -20.5f * FT2M});
    m_craftLines.push_back({-4.5f * FT2M, -18.8f * FT2M,  4.5f * FT2M, -18.8f * FT2M});
    m_craftLines.push_back({ 0.0f * FT2M, -20.5f * FT2M,  0.0f * FT2M, -23.5f * FT2M});
    m_craftLines.push_back({-2.5f * FT2M, -22.0f * FT2M,  2.5f * FT2M, -22.0f * FT2M});
    m_craftLines.push_back({-2.5f * FT2M, -22.0f * FT2M,  0.0f * FT2M, -23.5f * FT2M});
    m_craftLines.push_back({ 2.5f * FT2M, -22.0f * FT2M,  0.0f * FT2M, -23.5f * FT2M});

    // 3. Twin Heavy Artillery Turrets
    m_craftLines.push_back({-8.5f * FT2M, -12.0f * FT2M, -7.5f * FT2M, -14.5f * FT2M});
    m_craftLines.push_back({-7.5f * FT2M, -14.5f * FT2M, -3.5f * FT2M, -14.5f * FT2M});
    m_craftLines.push_back({-3.5f * FT2M, -14.5f * FT2M, -2.5f * FT2M, -12.0f * FT2M});
    m_craftLines.push_back({-6.5f * FT2M, -14.5f * FT2M, -6.5f * FT2M, -17.5f * FT2M});
    m_craftLines.push_back({-4.5f * FT2M, -14.5f * FT2M, -4.5f * FT2M, -17.5f * FT2M});
    m_craftLines.push_back({-7.0f * FT2M, -17.5f * FT2M, -4.0f * FT2M, -17.5f * FT2M});

    m_craftLines.push_back({ 2.5f * FT2M, -12.0f * FT2M,  3.5f * FT2M, -14.5f * FT2M});
    m_craftLines.push_back({ 3.5f * FT2M, -14.5f * FT2M,  7.5f * FT2M, -14.5f * FT2M});
    m_craftLines.push_back({ 7.5f * FT2M, -14.5f * FT2M,  8.5f * FT2M, -12.0f * FT2M});
    m_craftLines.push_back({ 4.5f * FT2M, -14.5f * FT2M,  4.5f * FT2M, -17.5f * FT2M});
    m_craftLines.push_back({ 6.5f * FT2M, -14.5f * FT2M,  6.5f * FT2M, -17.5f * FT2M});
    m_craftLines.push_back({ 4.0f * FT2M, -17.5f * FT2M,  7.0f * FT2M, -17.5f * FT2M});

    // 4. Flank Torpedo Sponsons
    m_craftLines.push_back({-15.0f * FT2M, -7.5f * FT2M, -19.5f * FT2M, -5.5f * FT2M});
    m_craftLines.push_back({-19.5f * FT2M, -5.5f * FT2M, -19.5f * FT2M, -2.5f * FT2M});
    m_craftLines.push_back({-19.5f * FT2M, -2.5f * FT2M, -15.0f * FT2M, -1.0f * FT2M});
    m_craftLines.push_back({-19.5f * FT2M, -4.0f * FT2M, -15.0f * FT2M, -4.0f * FT2M});

    m_craftLines.push_back({ 15.0f * FT2M, -7.5f * FT2M,  19.5f * FT2M, -5.5f * FT2M});
    m_craftLines.push_back({ 19.5f * FT2M, -5.5f * FT2M,  19.5f * FT2M, -2.5f * FT2M});
    m_craftLines.push_back({ 19.5f * FT2M, -2.5f * FT2M,  15.0f * FT2M, -1.0f * FT2M});
    m_craftLines.push_back({ 19.5f * FT2M, -4.0f * FT2M,  15.0f * FT2M, -4.0f * FT2M});

    // 5. Twin Engine Cowlings
    m_craftLines.push_back({-9.0f * FT2M, 0.0f * FT2M, -9.0f * FT2M, 5.0f * FT2M});
    m_craftLines.push_back({-2.0f * FT2M, 0.0f * FT2M, -2.0f * FT2M, 5.0f * FT2M});
    m_craftLines.push_back({-9.0f * FT2M, 5.0f * FT2M, -2.0f * FT2M, 5.0f * FT2M});
    m_craftLines.push_back({-8.0f * FT2M, 2.5f * FT2M, -3.0f * FT2M, 2.5f * FT2M});

    m_craftLines.push_back({ 2.0f * FT2M, 0.0f * FT2M,  2.0f * FT2M, 5.0f * FT2M});
    m_craftLines.push_back({ 9.0f * FT2M, 0.0f * FT2M,  9.0f * FT2M, 5.0f * FT2M});
    m_craftLines.push_back({ 2.0f * FT2M, 5.0f * FT2M,  9.0f * FT2M, 5.0f * FT2M});
    m_craftLines.push_back({ 3.0f * FT2M, 2.5f * FT2M,  8.0f * FT2M, 2.5f * FT2M});

    m_craftLines.push_back({-2.0f * FT2M, 0.0f * FT2M, -1.5f * FT2M, 3.0f * FT2M});
    m_craftLines.push_back({ 2.0f * FT2M, 0.0f * FT2M,  1.5f * FT2M, 3.0f * FT2M});
    m_craftLines.push_back({-1.5f * FT2M, 3.0f * FT2M,  1.5f * FT2M, 3.0f * FT2M});

    // 6. Combat Landing Skids
    m_craftLines.push_back({-11.0f * FT2M,  0.0f * FT2M, -18.0f * FT2M, 12.0f * FT2M});
    m_craftLines.push_back({-16.0f * FT2M, -4.0f * FT2M, -18.0f * FT2M, 12.0f * FT2M});
    m_craftLines.push_back({ -6.0f * FT2M,  0.0f * FT2M, -18.0f * FT2M, 12.0f * FT2M});
    m_craftLines.push_back({-23.0f * FT2M, 12.0f * FT2M, -13.0f * FT2M, 12.0f * FT2M});
    m_craftLines.push_back({-23.0f * FT2M, 12.0f * FT2M, -25.0f * FT2M,  9.5f * FT2M});
    m_craftLines.push_back({-13.0f * FT2M, 12.0f * FT2M, -11.0f * FT2M,  9.5f * FT2M});

    m_craftLines.push_back({ 11.0f * FT2M,  0.0f * FT2M,  18.0f * FT2M, 12.0f * FT2M});
    m_craftLines.push_back({ 16.0f * FT2M, -4.0f * FT2M,  18.0f * FT2M, 12.0f * FT2M});
    m_craftLines.push_back({  6.0f * FT2M,  0.0f * FT2M,  18.0f * FT2M, 12.0f * FT2M});
    m_craftLines.push_back({ 13.0f * FT2M, 12.0f * FT2M,  23.0f * FT2M, 12.0f * FT2M});
    m_craftLines.push_back({ 23.0f * FT2M, 12.0f * FT2M,  25.0f * FT2M,  9.5f * FT2M});
    m_craftLines.push_back({ 13.0f * FT2M, 12.0f * FT2M,  11.0f * FT2M,  9.5f * FT2M});

    // ── Full 3D Volumetric Wireframe Battleship Geometry (in Feet -> Meters) ──
    auto scaleY = [](float yMeters) {
        constexpr float skidY = 12.0f * FT2M;
        return skidY + (yMeters - skidY) * SHIP_SCALE;
    };
    for (auto& seg : m_craftLines) {
        seg.x1 *= SHIP_SCALE;
        seg.y1 = scaleY(seg.y1);
        seg.x2 *= SHIP_SCALE;
        seg.y2 = scaleY(seg.y2);
    }

    auto add3D = [&](float x1, float y1, float z1, float x2, float y2, float z2) {
        m_craftLines3D.push_back({
            x1 * FT2M * SHIP_SCALE, scaleY(y1 * FT2M), z1 * FT2M * SHIP_SCALE,
            x2 * FT2M * SHIP_SCALE, scaleY(y2 * FT2M), z2 * FT2M * SHIP_SCALE
        });
    };

    // A. 3D Armored Citadel Hull Rings (Fore z = +9 ft, Aft z = -9 ft)
    const float zHull[2] = {-9.0f, 9.0f};
    for (float z : zHull) {
        add3D(-11.0f,  0.0f, z,  11.0f,  0.0f, z);
        add3D(-16.0f, -4.0f, z,  16.0f, -4.0f, z);
        add3D(-12.0f, -12.0f, z, 12.0f, -12.0f, z);
        add3D(-11.0f,  0.0f, z, -16.0f, -4.0f, z);
        add3D( 11.0f,  0.0f, z,  16.0f, -4.0f, z);
        add3D(-16.0f, -4.0f, z, -15.0f, -8.0f, z);
        add3D( 16.0f, -4.0f, z,  15.0f, -8.0f, z);
        add3D(-15.0f, -8.0f, z, -12.0f, -12.0f, z);
        add3D( 15.0f, -8.0f, z,  12.0f, -12.0f, z);
    }

    // Longitudinal 3D Citadel Chines connecting Aft (-9 ft) to Fore (+9 ft)
    add3D(-11.0f,  0.0f, -9.0f, -11.0f,  0.0f,  9.0f);
    add3D( 11.0f,  0.0f, -9.0f,  11.0f,  0.0f,  9.0f);
    add3D(-16.0f, -4.0f, -9.0f, -16.0f, -4.0f,  9.0f);
    add3D( 16.0f, -4.0f, -9.0f,  16.0f, -4.0f,  9.0f);
    add3D(-15.0f, -8.0f, -9.0f, -15.0f, -8.0f,  9.0f);
    add3D( 15.0f, -8.0f, -9.0f,  15.0f, -8.0f,  9.0f);
    add3D(-12.0f, -12.0f, -9.0f, -12.0f, -12.0f, 9.0f);
    add3D( 12.0f, -12.0f, -9.0f,  12.0f, -12.0f, 9.0f);

    // Beveled Bow Prow (z = +13.5 ft) & Stern Bulkhead (z = -13.5 ft)
    for (float signZ : {-1.0f, 1.0f}) {
        float zMid = 9.0f * signZ;
        float zTip = 13.5f * signZ;
        add3D(-7.0f, -2.0f, zTip,  7.0f, -2.0f, zTip);
        add3D(-7.0f, -10.0f, zTip, 7.0f, -10.0f, zTip);
        add3D(-7.0f, -2.0f, zTip, -7.0f, -10.0f, zTip);
        add3D( 7.0f, -2.0f, zTip,  7.0f, -10.0f, zTip);
        add3D(-11.0f,  0.0f, zMid, -7.0f, -2.0f, zTip);
        add3D( 11.0f,  0.0f, zMid,  7.0f, -2.0f, zTip);
        add3D(-12.0f, -12.0f, zMid, -7.0f, -10.0f, zTip);
        add3D( 12.0f, -12.0f, zMid,  7.0f, -10.0f, zTip);
    }

    // B. 3D Command Bridge Superstructure & Sensor Mast
    const float zBr[2] = {-4.5f, 4.5f};
    for (float z : zBr) {
        add3D(-10.0f, -12.0f, z, -6.0f, -18.0f, z);
        add3D( 10.0f, -12.0f, z,  6.0f, -18.0f, z);
        add3D(-6.0f,  -18.0f, z,  6.0f, -18.0f, z);
        add3D(-6.0f,  -18.0f, z, -4.0f, -20.5f, z);
        add3D( 6.0f,  -18.0f, z,  4.0f, -20.5f, z);
        add3D(-4.0f,  -20.5f, z,  4.0f, -20.5f, z);
    }
    add3D(-6.0f, -18.0f, -4.5f, -6.0f, -18.0f, 4.5f);
    add3D( 6.0f, -18.0f, -4.5f,  6.0f, -18.0f, 4.5f);
    add3D(-4.0f, -20.5f, -4.5f, -4.0f, -20.5f, 4.5f);
    add3D( 4.0f, -20.5f, -4.5f,  4.0f, -20.5f, 4.5f);
    // 3D Radar & Targeting Mast
    add3D( 0.0f, -20.5f,  0.0f,  0.0f, -24.0f,  0.0f);
    add3D(-3.0f, -22.2f,  0.0f,  3.0f, -22.2f,  0.0f);
    add3D( 0.0f, -22.2f, -3.0f,  0.0f, -22.2f,  3.0f);
    add3D(-3.0f, -22.2f,  0.0f,  0.0f, -24.0f,  0.0f);
    add3D( 3.0f, -22.2f,  0.0f,  0.0f, -24.0f,  0.0f);

    // C. Port & Starboard 3D Dual-Barrel Artillery Turrets
    for (float sX : {-1.0f, 1.0f}) {
        float xIn  = 2.8f * sX;
        float xOut = 8.2f * sX;
        float xB1  = 4.5f * sX;
        float xB2  = 6.5f * sX;
        add3D(xIn,  -12.0f, -3.0f, xOut, -12.0f, -3.0f);
        add3D(xIn,  -12.0f,  3.0f, xOut, -12.0f,  3.0f);
        add3D(xIn,  -14.5f, -2.0f, xOut, -14.5f, -2.0f);
        add3D(xIn,  -14.5f,  2.0f, xOut, -14.5f,  2.0f);
        add3D(xIn,  -14.5f, -2.0f, xIn,  -14.5f,  2.0f);
        add3D(xOut, -14.5f, -2.0f, xOut, -14.5f,  2.0f);
        add3D(xB1,  -14.5f,  1.0f, xB1,  -17.8f,  3.5f);
        add3D(xB2,  -14.5f,  1.0f, xB2,  -17.8f,  3.5f);
    }

    // D. Port & Starboard 3D Flank Torpedo Sponsons
    for (float sX : {-1.0f, 1.0f}) {
        for (float z : {-4.5f, 4.5f}) {
            add3D(15.0f * sX, -7.5f, z, 19.5f * sX, -5.5f, z);
            add3D(19.5f * sX, -5.5f, z, 19.5f * sX, -2.5f, z);
            add3D(19.5f * sX, -2.5f, z, 15.0f * sX, -1.0f, z);
        }
        add3D(19.5f * sX, -5.5f, -4.5f, 19.5f * sX, -5.5f, 4.5f);
        add3D(19.5f * sX, -2.5f, -4.5f, 19.5f * sX, -2.5f, 4.5f);
    }

    // E. Twin 3D Octagonal Engine Cowlings & Bells (Port x = -5.5 ft, Starboard x = +5.5 ft)
    for (float cx : {-5.5f, 5.5f}) {
        const float rTop = 3.0f;
        const float rBot = 3.6f;
        const int segs = 6;
        for (int i = 0; i < segs; ++i) {
            float a1 = (float)i * (6.2831853f / segs);
            float a2 = (float)(i + 1) * (6.2831853f / segs);
            float x1t = cx + rTop * std::cos(a1), z1t = rTop * std::sin(a1);
            float x2t = cx + rTop * std::cos(a2), z2t = rTop * std::sin(a2);
            float x1b = cx + rBot * std::cos(a1), z1b = rBot * std::sin(a1);
            float x2b = cx + rBot * std::cos(a2), z2b = rBot * std::sin(a2);

            add3D(x1t, 0.0f, z1t, x2t, 0.0f, z2t); // Top engine ring
            add3D(x1b, 5.0f, z1b, x2b, 5.0f, z2b); // Exhaust bell rim
            add3D(x1t, 0.0f, z1t, x1b, 5.0f, z1b); // Bell vertical rib
        }
    }

    // F. 4-Corner Reinforced 3D Combat Landing Gear & Full-Length Twin Skids
    for (float sX : {-1.0f, 1.0f}) {
        float skidX = 18.0f * sX;
        // Full-length 3D ground contact runner from z = -12 ft (aft) to z = +12 ft (fore)
        add3D(skidX - 1.5f * sX, 12.0f, -12.0f, skidX - 1.5f * sX, 12.0f, 12.0f);
        add3D(skidX + 1.5f * sX, 12.0f, -12.0f, skidX + 1.5f * sX, 12.0f, 12.0f);
        add3D(skidX - 1.5f * sX, 12.0f, -12.0f, skidX + 1.5f * sX, 12.0f, -12.0f);
        add3D(skidX - 1.5f * sX, 12.0f,  12.0f, skidX + 1.5f * sX, 12.0f,  12.0f);
        // Upturned 3D skid bow (fore) & heel (aft)
        add3D(skidX, 12.0f,  12.0f, skidX + 1.5f * sX, 9.5f,  15.0f);
        add3D(skidX, 12.0f, -12.0f, skidX + 1.5f * sX, 9.5f, -15.0f);
        // Lateral skid outrigger bars at z = -8 ft and z = +8 ft
        for (float zLeg : {-8.0f, 8.0f}) {
            add3D(11.0f * sX,  0.0f, zLeg, skidX, 12.0f, zLeg);
            add3D(16.0f * sX, -4.0f, zLeg, skidX, 12.0f, zLeg);
            add3D( 6.0f * sX,  0.0f, zLeg, skidX, 12.0f, zLeg);
            add3D(13.0f * sX, 12.0f, zLeg, 23.0f * sX, 12.0f, zLeg);
        }
    }
}

void Battleship::reset(float startX, float startY, float startFuel, float startZ) {
    m_state = BattleshipState::FLYING;
    m_x = startX;              // [m]
    m_y = startY;              // [m]
    m_z = startZ;              // [m]
    m_vx = 30.0f * FT2M;       // [m/s] Initial entry horizontal drift: 9.144 m/s
    m_vy = 2.0f * FT2M;        // [m/s] Initial entry vertical drift: 0.6096 m/s
    m_vz = 0.0f;               // [m/s]
    m_angle = 0.0f;            // [rad]
    m_angularVel = 0.0f;       // [rad/s]
    m_pitch = 0.0f;            // [rad]
    m_pitchVel = 0.0f;         // [rad/s]
    m_yaw = 0.0f;              // [rad]
    m_yawVel = 0.0f;           // [rad/s]
    m_fuel = startFuel;        // [kg]
    m_throttleLeft = 0.0f;     // [0.0 - 1.0, dimensionless]
    m_throttleRight = 0.0f;    // [0.0 - 1.0, dimensionless]
    m_abortActive = false;
    m_abortTimer = 0.0f;       // [s]
    m_damage = 0.0f;           // [%]
    m_impactCooldown = 0.0f;   // [s]
    m_impactFlashTimer = 0.0f; // [s]
    m_lastImpact = {};
    m_lastPadMultiplier = 0;
    m_flameFlicker = 0.0f;     // [rad]
    m_rcsPuffLeft = 0.0f;      // [s]
    m_rcsPuffRight = 0.0f;     // [s]
    m_rcsPuffFore = 0.0f;      // [s]
    m_rcsPuffAft = 0.0f;       // [s]
    m_debris.clear();
    m_particles.clear();
    m_takeoffCooldown = 0.0f;
    m_landedSurfaceY = startY;
    m_impulseBoostActive = false;
    m_airBrakesToggled = false;
    m_airBrakesActive = false;
    m_airBrakeDeploy = 0.0f;
    m_airBrakeDragMagnitude = 0.0f;
    m_hoverThrustersEnabled = false;
    m_hoverThrusterThrottle = 0.0f;
    m_groundEffectMagnitude = 0.0f;
    m_forceVectors.clear();
}

void Battleship::applyForceAtLocalPoint3D(const Vec3& localR,
                                          const Vec3& localA,
                                          Vec3& accumWorldA,
                                          float& accumRollAccel,
                                          float& accumPitchAccel,
                                          float& accumYawAccel,
                                          const std::string& label, SDL_Color color) {
    Vec3 worldPos = localToWorldPoint(localR);
    Vec3 worldA = localToWorldDir(localA);

    accumWorldA += worldA;

    // 3D rigid-body torques around center of mass:
    // Roll torque (XY plane):  tau_roll  = r_x * F_y - r_y * F_x
    // Pitch torque (YZ plane): tau_pitch = r_z * F_y - r_y * F_z
    // Yaw torque (XZ plane):   tau_yaw   = r_x * F_z - r_z * F_x
    float tauRoll  = localR.x * localA.y - localR.y * localA.x;
    float tauPitch = localR.z * localA.y - localR.y * localA.z;
    float tauYaw   = localR.x * localA.z - localR.z * localA.x;

    accumRollAccel  += tauRoll / HULL_INERTIA;
    accumPitchAccel += tauPitch / HULL_INERTIA;
    accumYawAccel   += tauYaw / HULL_INERTIA;

    if (!label.empty() && worldA.lengthSq() > 1e-3f) {
        m_forceVectors.push_back({worldPos.x, worldPos.y, worldPos.z, worldA.x, worldA.y, worldA.z, color, label});
    }
}

void Battleship::resetLanded(float startX, float startY, float startFuel, float startZ) {
    reset(startX, startY, startFuel, startZ);
    m_state = BattleshipState::LANDED;
    m_landedSurfaceY = startY;
    m_y = startY;
    m_vx = 0.0f;
    m_vy = 0.0f;
    m_vz = 0.0f;
    m_angle = 0.0f;
    m_angularVel = 0.0f;
    m_pitch = 0.0f;
    m_pitchVel = 0.0f;
    m_yaw = 0.0f;
    m_yawVel = 0.0f;
}

bool Battleship::isSafeSpeed() const {
    float horizSpeed = std::sqrt(m_vx * m_vx + m_vz * m_vz);
    return horizSpeed <= SAFE_MAX_VX && m_vy <= SAFE_MAX_VY && m_vy >= -10.0f * FT2M; // [m/s]
}

bool Battleship::isSafeAngle() const {
    float tiltRad = std::sqrt(m_angle * m_angle + m_pitch * m_pitch);
    float deg = tiltRad * RAD2DEG; // [deg]
    return deg <= SAFE_MAX_ANGLE_DEG;
}

void Battleship::update(float dt, float throttleLeft, float throttleRight, float rotateInput, bool abortTriggered, bool boostTriggered,
                        const Terrain& terrain, SoundEngine& sound, bool airBrakeHeld,
                        float pitchInput, float yawInput) {
    m_impulseBoostActive = false;
    m_forceVectors.clear();
    m_airBrakesActive = m_airBrakesToggled || airBrakeHeld;
    if (m_airBrakesActive) {
        m_airBrakeDeploy = std::min(1.0f, m_airBrakeDeploy + 12.0f * dt);
    } else {
        m_airBrakeDeploy = std::max(0.0f, m_airBrakeDeploy - 12.0f * dt);
    }

    if (m_state == BattleshipState::CRASHED) {
        // Update 3D debris motion
        for (auto& d : m_debris) {
            d.x1 += d.vx * dt;
            d.y1 += d.vy * dt;
            d.z1 += d.vz * dt;
            d.x2 += d.vx * dt;
            d.y2 += d.vy * dt;
            d.z2 += d.vz * dt;
            d.vy += GRAVITY * 0.4f * dt;
            d.angle += d.angularVel * dt;
            d.life -= dt * 0.5f;
        }
        updateThrusterParticles(dt, terrain);
        sound.setThrust(0.0f);
        return;
    }

    if (m_state == BattleshipState::LANDED) {
        if (boostTriggered && (m_unlimitedFuel || m_fuel > 5.0f * LB2KG)) {
            m_state = BattleshipState::FLYING;
            m_takeoffCooldown = 0.6f;
            static const float IMPULSE_GE_MAX_TOTAL_DV = 2.0f * (25.2f / 120.0f) * IMPULSE_DELTA_V; // [m/s] 26.24 m/s
            m_vy = -(IMPULSE_DELTA_V + IMPULSE_GE_MAX_TOTAL_DV);
            m_y += m_vy * dt;
            m_throttleLeft = 1.0f;
            m_throttleRight = 1.0f;
            m_impulseBoostActive = true;
            m_groundEffectMagnitude = 1.0f;
            sound.playImpulseBoost();
            spawnImpulseParticles();
            if (!m_unlimitedFuel) {
                m_fuel = std::max(0.0f, m_fuel - 15.0f * LB2KG);
            }
            m_forceVectors.push_back({m_x - 5.5f * FT2M, m_y + 5.0f * FT2M, m_z, 0.0f, -THRUST_PER_ENGINE * 1.5f, 0.0f, {80, 255, 120, 255}, "L.IMP"});
            m_forceVectors.push_back({m_x + 5.5f * FT2M, m_y + 5.0f * FT2M, m_z, 0.0f, -THRUST_PER_ENGINE * 1.5f, 0.0f, {80, 255, 120, 255}, "R.IMP"});
            return;
        }

        // If Ground Effect Hover Thrusters are turned ON while landed, automatically lift off into hover
        if (m_hoverThrustersEnabled && (m_unlimitedFuel || m_fuel > 0.0f)) {
            m_state = BattleshipState::FLYING;
            m_takeoffCooldown = 0.6f;
        } else {
            // While landed, check if pilot applies sufficient upward thrust to take off
            float rawThrottleLeft = throttleLeft;
            float rawThrottleRight = throttleRight;
            if (!m_unlimitedFuel && m_fuel <= 0.0f) {
                rawThrottleLeft = 0.0f;
                rawThrottleRight = 0.0f;
            }
            float appliedThrottleLeft = applyThrustCurve(rawThrottleLeft);
            float appliedThrottleRight = applyThrustCurve(rawThrottleRight);
            float totalThrottle = appliedThrottleLeft + appliedThrottleRight;

            m_throttleLeft = appliedThrottleLeft;
            m_throttleRight = appliedThrottleRight;
            sound.setThrust(std::clamp(totalThrottle * 0.6f, 0.0f, 1.0f));

            if (!m_unlimitedFuel && totalThrottle > 0.0f && m_fuel > 0.0f) {
                float fuelUsed = (totalThrottle * 0.5f) * FUEL_BURN_RATE * dt;
                m_fuel = std::max(0.0f, m_fuel - fuelUsed);
            }

            spawnThrusterParticles(dt);
            updateThrusterParticles(dt, terrain);

            float leftThrust = appliedThrottleLeft * THRUST_PER_ENGINE;
            float rightThrust = appliedThrottleRight * THRUST_PER_ENGINE;
            float totalThrustAccel = leftThrust + rightThrust;
            float ay = -totalThrustAccel + GRAVITY;

            if (ay < -0.1f) {
                m_state = BattleshipState::FLYING;
                m_takeoffCooldown = 0.6f;
                m_vy += ay * dt;
                m_y += m_vy * dt;
            } else {
                m_y = m_landedSurfaceY;
                m_vx = 0.0f;
                m_vy = 0.0f;
                m_vz = 0.0f;
                m_angle = 0.0f;
                m_angularVel = 0.0f;
                m_pitch = 0.0f;
                m_pitchVel = 0.0f;
                m_yawVel = 0.0f;

                // Record static equilibrium force vectors while resting on pad
                m_forceVectors.push_back({m_x, m_y, m_z, 0.0f, GRAVITY, 0.0f, {195, 140, 255, 220}, "GRAV"});
                if (leftThrust > 0.1f) {
                    m_forceVectors.push_back({m_x - 5.5f * FT2M, m_y + 5.0f * FT2M, m_z, 0.0f, -leftThrust, 0.0f, {80, 255, 120, 255}, "L.THR"});
                }
                if (rightThrust > 0.1f) {
                    m_forceVectors.push_back({m_x + 5.5f * FT2M, m_y + 5.0f * FT2M, m_z, 0.0f, -rightThrust, 0.0f, {80, 255, 120, 255}, "R.THR"});
                }
                float padNormalPerSkid = std::max(0.0f, (GRAVITY - totalThrustAccel) * 0.5f);
                if (padNormalPerSkid > 0.1f) {
                    m_forceVectors.push_back({m_x - 18.0f * FT2M, m_y + 12.0f * FT2M, m_z, 0.0f, -padNormalPerSkid, 0.0f, {160, 230, 180, 220}, "PAD.L"});
                    m_forceVectors.push_back({m_x + 18.0f * FT2M, m_y + 12.0f * FT2M, m_z, 0.0f, -padNormalPerSkid, 0.0f, {160, 230, 180, 220}, "PAD.R"});
                }
                return;
            }
        }
    }

    if (m_takeoffCooldown > 0.0f) m_takeoffCooldown -= dt;

    // Decay impact timers
    if (m_impactCooldown > 0.0f) m_impactCooldown -= dt;
    if (m_impactFlashTimer > 0.0f) m_impactFlashTimer -= dt;

    // 3D Flying physics
    updatePhysics(dt, throttleLeft, throttleRight, rotateInput, pitchInput, yawInput,
                  abortTriggered, boostTriggered, terrain, sound);

    // Thruster & hover particle debris animation and 3D bounce physics
    spawnThrusterParticles(dt);
    spawnHoverParticles(dt);
    updateThrusterParticles(dt, terrain);

    // 3D Collision check against terrain
    checkTerrainCollision(terrain, sound);
}

void Battleship::updatePhysics(float dt, float throttleLeft, float throttleRight, float rotateInput,
                               float pitchInput, float yawInput,
                               bool abortTriggered, bool boostTriggered,
                               const Terrain& terrain, SoundEngine& sound) {
    // Check abort condition
    if (abortTriggered && !m_abortActive && (m_unlimitedFuel || m_fuel > 20.0f * LB2KG)) {
        m_abortActive = true;
        m_abortTimer = 0.85f; // [s] Abort retro burn duration
        if (!m_unlimitedFuel) {
            m_fuel = std::max(0.0f, m_fuel - 80.0f * LB2KG); // [kg] Abort fuel penalty: 36.29 kg
        }
        sound.playBeep(1200.0f, 0.2f); // [Hz], [s]
    }

    float rawThrottleLeft = throttleLeft;
    float rawThrottleRight = throttleRight;
    float appliedRotate = rotateInput;
    float appliedPitch  = pitchInput;
    float appliedYaw    = yawInput;

    // Impulse Thruster one-frame boost via main thrusters along 3D hull vertical axis (0, -1, 0)
    if (boostTriggered && (m_unlimitedFuel || m_fuel > 5.0f * LB2KG)) {
        m_impulseBoostActive = true;
        Vec3 thrustDir = localToWorldDir({0.0f, -1.0f, 0.0f});

        m_vx += thrustDir.x * IMPULSE_DELTA_V;
        m_vy += thrustDir.y * IMPULSE_DELTA_V;
        m_vz += thrustDir.z * IMPULSE_DELTA_V;

        rawThrottleLeft = 1.0f;
        rawThrottleRight = 1.0f;

        spawnImpulseParticles();
        sound.playImpulseBoost();

        if (!m_unlimitedFuel) {
            m_fuel = std::max(0.0f, m_fuel - 15.0f * LB2KG);
        }
    }

    if (m_abortActive) {
        m_abortTimer -= dt;
        rawThrottleLeft = 1.0f;
        rawThrottleRight = 1.0f;
        // Right the battleship towards 0 roll & 0 pitch using 3D PD attitude control
        float targetRollTorque  = -m_angle * 3.5f - m_angularVel * 2.2f;
        float targetPitchTorque = -m_pitch * 3.5f - m_pitchVel * 2.2f;
        appliedRotate = std::clamp(targetRollTorque, -1.0f, 1.0f);
        appliedPitch  = std::clamp(targetPitchTorque, -1.0f, 1.0f);
        appliedYaw    = std::clamp(-m_yawVel * 2.0f, -1.0f, 1.0f);

        if (m_abortTimer <= 0.0f) {
            m_abortActive = false;
        }
    }

    // Out of fuel check
    if (!m_unlimitedFuel && m_fuel <= 0.0f) {
        m_fuel = 0.0f;
        rawThrottleLeft = 0.0f;
        rawThrottleRight = 0.0f;
        appliedRotate = 0.0f;
        appliedPitch = 0.0f;
        appliedYaw = 0.0f;
    }

    // Slightly exponential thrust response curve (pow 1.6)
    float appliedThrottleLeft = applyThrustCurve(rawThrottleLeft);
    float appliedThrottleRight = applyThrustCurve(rawThrottleRight);

    // Burn main engine fuel
    float totalThrottle = appliedThrottleLeft + appliedThrottleRight;
    if (!m_unlimitedFuel && totalThrottle > 0.0f && m_fuel > 0.0f) {
        float fuelUsed = (totalThrottle * 0.5f) * FUEL_BURN_RATE * dt;
        m_fuel = std::max(0.0f, m_fuel - fuelUsed);
    }
    m_throttleLeft = appliedThrottleLeft;
    m_throttleRight = appliedThrottleRight;

    // Continuous engine sound driven by total thrust
    sound.setThrust(std::clamp(totalThrottle * 0.6f, 0.0f, 1.0f));

    // ── 3D Rigid-Body Thruster Force & Torque Accumulation ───────────────────
    Vec3 totalWorldA{0.0f, GRAVITY, 0.0f};
    float totalRollAccel  = 0.0f;
    float totalPitchAccel = 0.0f;
    float totalYawAccel   = 0.0f;

    // Gravity acts at center of mass (m_x, m_y, m_z)
    m_forceVectors.push_back({m_x, m_y, m_z, 0.0f, GRAVITY, 0.0f, {195, 140, 255, 220}, "GRAV"});

    // 1. Port & Starboard Main Thrusters mounted at local (-5.5 ft, +5.0 ft, 0) and (+5.5 ft, +5.0 ft, 0)
    float diffThrust = appliedThrottleLeft - appliedThrottleRight;
    if (appliedThrottleLeft > 0.0f) {
        applyForceAtLocalPoint3D({-5.5f * FT2M, 5.0f * FT2M, 0.0f},
                                 {0.0f, -appliedThrottleLeft * THRUST_PER_ENGINE, 0.0f},
                                 totalWorldA, totalRollAccel, totalPitchAccel, totalYawAccel,
                                 "L.THR", {80, 255, 120, 255});
    }
    if (appliedThrottleRight > 0.0f) {
        applyForceAtLocalPoint3D({5.5f * FT2M, 5.0f * FT2M, 0.0f},
                                 {0.0f, -appliedThrottleRight * THRUST_PER_ENGINE, 0.0f},
                                 totalWorldA, totalRollAccel, totalPitchAccel, totalYawAccel,
                                 "R.THR", {80, 255, 120, 255});
    }

    // 2. Port & Starboard Roll RCS Attitude Thrusters at (-14.0 ft, -8.0 ft, 0) and (+14.0 ft, -8.0 ft, 0)
    if (std::abs(appliedRotate) > 0.05f && (m_unlimitedFuel || m_fuel > 0.0f)) {
        float rcsLocalForceX = appliedRotate * RCS_THRUST_ACCEL;
        if (appliedRotate > 0.05f) {
            applyForceAtLocalPoint3D({-14.0f * FT2M, -8.0f * FT2M, 0.0f},
                                     {rcsLocalForceX, 0.0f, 0.0f},
                                     totalWorldA, totalRollAccel, totalPitchAccel, totalYawAccel,
                                     "RCS.L", {255, 135, 70, 255});
            m_rcsPuffLeft = 0.1f;
        } else if (appliedRotate < -0.05f) {
            applyForceAtLocalPoint3D({14.0f * FT2M, -8.0f * FT2M, 0.0f},
                                     {rcsLocalForceX, 0.0f, 0.0f},
                                     totalWorldA, totalRollAccel, totalPitchAccel, totalYawAccel,
                                     "RCS.R", {255, 135, 70, 255});
            m_rcsPuffRight = 0.1f;
        }
        sound.playRCS();

        if (!m_unlimitedFuel) {
            float rcsFuel = std::abs(appliedRotate) * RCS_FUEL_BURN_RATE * dt;
            m_fuel = std::max(0.0f, m_fuel - rcsFuel);
        }
    }

    // 2b. Fore & Aft Pitch RCS Attitude Thrusters mounted at (0, -8.0 ft, -11.0 ft) and (0, -8.0 ft, +11.0 ft)
    if (std::abs(appliedPitch) > 0.05f && (m_unlimitedFuel || m_fuel > 0.0f)) {
        float rcsLocalForceZ = appliedPitch * RCS_THRUST_ACCEL;
        if (appliedPitch > 0.05f) {
            applyForceAtLocalPoint3D({0.0f, -8.0f * FT2M, -11.0f * FT2M},
                                     {0.0f, 0.0f, rcsLocalForceZ},
                                     totalWorldA, totalRollAccel, totalPitchAccel, totalYawAccel,
                                     "RCS.A", {255, 165, 70, 255});
            m_rcsPuffAft = 0.1f;
        } else if (appliedPitch < -0.05f) {
            applyForceAtLocalPoint3D({0.0f, -8.0f * FT2M, 11.0f * FT2M},
                                     {0.0f, 0.0f, rcsLocalForceZ},
                                     totalWorldA, totalRollAccel, totalPitchAccel, totalYawAccel,
                                     "RCS.F", {255, 165, 70, 255});
            m_rcsPuffFore = 0.1f;
        }
        sound.playRCS();

        if (!m_unlimitedFuel) {
            float rcsFuel = std::abs(appliedPitch) * RCS_FUEL_BURN_RATE * dt;
            m_fuel = std::max(0.0f, m_fuel - rcsFuel);
        }
    }

    // 2c. Yaw RCS Thrusters
    if (std::abs(appliedYaw) > 0.05f && (m_unlimitedFuel || m_fuel > 0.0f)) {
        totalYawAccel += appliedYaw * 2.4f;
        sound.playRCS();
    }

    // 3. Automatic 3D Ground Effect Hover Thrusters
    updateHoverThrusters(dt, appliedRotate, appliedPitch, diffThrust, terrain, sound,
                         totalWorldA, totalRollAccel, totalPitchAccel, totalYawAccel);

    // Integrate 3D angular velocities & attitude angles
    m_angularVel += totalRollAccel * dt;
    m_pitchVel   += totalPitchAccel * dt;
    m_yawVel     += totalYawAccel * dt;

    m_angle += m_angularVel * dt;
    m_pitch += m_pitchVel * dt;
    m_yaw   += m_yawVel * dt;

    wrapAngleRad(m_angle);
    wrapAngleRad(m_pitch);
    wrapAngleRad(m_yaw);

    // Integrate 3D linear velocity from accumulated world forces
    m_vx += totalWorldA.x * dt;
    m_vy += totalWorldA.y * dt;
    m_vz += totalWorldA.z * dt;

    // ── 3D Air Brakes: Aerodynamic Drag & Rotational Stabilization ───────────
    Vec3 dragAccel{0.0f, 0.0f, 0.0f};
    if (m_airBrakeDeploy > 0.001f) {
        static const float AIR_BRAKE_LINEAR_DRAG = 0.18f;  // [1/s]
        static const float AIR_BRAKE_QUAD_DRAG   = 0.0035f;// [1/m]
        static const float AIR_BRAKE_ANG_DRAG    = 0.35f;  // [1/s]

        Vec3 preV{m_vx, m_vy, m_vz};
        float speed = preV.length();
        float dragRate = (AIR_BRAKE_LINEAR_DRAG + AIR_BRAKE_QUAD_DRAG * speed) * m_airBrakeDeploy;
        float atten = 1.0f / (1.0f + dragRate * dt);
        m_vx *= atten;
        m_vy *= atten;
        m_vz *= atten;

        float angAtten = 1.0f / (1.0f + (AIR_BRAKE_ANG_DRAG * m_airBrakeDeploy) * dt);
        m_angularVel *= angAtten;
        m_pitchVel   *= angAtten;
        m_yawVel     *= angAtten;

        m_airBrakeDragMagnitude = std::clamp((speed * dragRate) / (50.0f * FT2M), 0.0f, 1.0f);

        if (dt > 1e-5f) {
            dragAccel = (Vec3{m_vx, m_vy, m_vz} - preV) / dt;
            if (dragAccel.lengthSq() > 0.05f) {
                m_forceVectors.push_back({m_x, m_y, m_z, dragAccel.x, dragAccel.y, dragAccel.z, {255, 195, 70, 240}, "DRAG"});
            }
        }
    } else {
        m_airBrakeDragMagnitude = 0.0f;
    }

    m_x += m_vx * dt;
    m_y += m_vy * dt;
    m_z += m_vz * dt;

    // Keep Z within world depth bounds with soft boundary reflection
    float maxAbsZ = terrain.getWorldDepth() * 0.48f;
    if (m_z > maxAbsZ) { m_z = maxAbsZ; m_vz = -std::abs(m_vz) * 0.5f; }
    if (m_z < -maxAbsZ) { m_z = -maxAbsZ; m_vz = std::abs(m_vz) * 0.5f; }

    // ── 3D Ground Effect: Plume Impingement Ray Casting ───────────────────────
    static const int   GE_NUM_RAYS        = 7;
    static const float GE_CONE_HALF_ANGLE = 0.5236f;                // [rad] ±30 degrees
    static const float GE_MAX_DIST        = 180.0f * FT2M;          // [m] 54.864 m
    static const float GE_FORCE_PER_RAY   = 1.8f * FT2M;            // [m/s^2] 0.54864 m/s^2
    static const float GE_FALLOFF         = 65.0f * FT2M;           // [m] 19.812 m
    static const float GE_TORQUE_SCALE    = 0.018f / (FT2M * FT2M); // [rad/(m*s^2)] 0.19375

    static const int   IMPULSE_GE_NUM_RAYS        = 11;
    static const float IMPULSE_GE_CONE_HALF_ANGLE = 0.8727f;                // [rad] ±50 degrees
    static const float IMPULSE_GE_MAX_DIST        = 2.0f * GE_MAX_DIST;
    static const float IMPULSE_GE_FALLOFF         = 2.0f * GE_FALLOFF;
    static const float IMPULSE_GE_MAX_TOTAL_DV    = 2.0f * (25.2f / 120.0f) * IMPULSE_DELTA_V;
    static const float IMPULSE_GE_DV_PER_RAY      = IMPULSE_GE_MAX_TOTAL_DV / (2.0f * (float)IMPULSE_GE_NUM_RAYS);

    Vec3 exhaustDir = localToWorldDir({0.0f, 1.0f, 0.0f}); // Local down (+Y)
    Vec3 perpXDir   = localToWorldDir({1.0f, 0.0f, 0.0f}); // Local starboard (+X)
    Vec3 perpZDir   = localToWorldDir({0.0f, 0.0f, 1.0f}); // Local fore (+Z)

    struct NozzleInfo3D { Vec3 pos; float throttle; };
    NozzleInfo3D nozzles[2] = {
        { localToWorldPoint({-5.5f * FT2M, 5.0f * FT2M, 0.0f}), appliedThrottleLeft },
        { localToWorldPoint({ 5.5f * FT2M, 5.0f * FT2M, 0.0f}), appliedThrottleRight }
    };

    Vec3 geAccel{0.0f, 0.0f, 0.0f};
    float geTorqueRoll  = 0.0f;
    float geTorquePitch = 0.0f;

    for (const auto& nozzle : nozzles) {
        if (nozzle.throttle < 0.01f) continue;

        // Primary 7-ray lateral fan in the XY roll plane
        for (int r = 0; r < GE_NUM_RAYS; ++r) {
            float spread = (GE_NUM_RAYS > 1)
                ? GE_CONE_HALF_ANGLE * (2.0f * r / (GE_NUM_RAYS - 1) - 1.0f)
                : 0.0f;
            float cosS = std::cos(spread);
            float sinS = std::sin(spread);

            Vec3 rayDir = (exhaustDir * cosS + perpXDir * sinS).normalized();

            Terrain::RayHit3D hit;
            if (terrain.castRay3D(nozzle.pos, rayDir, GE_MAX_DIST, hit)) {
                float decay = std::exp(-hit.dist / GE_FALLOFF);
                float accelMag = nozzle.throttle * GE_FORCE_PER_RAY * decay;

                Vec3 f = hit.normal * accelMag;
                geAccel += f;

                Vec3 arm = nozzle.pos - Vec3{m_x, m_y, m_z};
                geTorqueRoll  += (arm.x * f.y - arm.y * f.x) * GE_TORQUE_SCALE;
                geTorquePitch += (arm.z * f.y - arm.y * f.z) * GE_TORQUE_SCALE;
            }
        }

        // Additional longitudinal Z-cone rays for 3D pitch & Z-slope ground-effect interaction
        if (std::abs(m_z) > 1e-3f || std::abs(m_pitch) > 1e-3f || std::abs(m_vz) > 1e-3f) {
            const float zSpreads[4] = {-0.45f, -0.22f, 0.22f, 0.45f};
            for (float spZ : zSpreads) {
                Vec3 rayDir = (exhaustDir * std::cos(spZ) + perpZDir * std::sin(spZ)).normalized();
                Terrain::RayHit3D hit;
                if (terrain.castRay3D(nozzle.pos, rayDir, GE_MAX_DIST, hit)) {
                    float decay = std::exp(-hit.dist / GE_FALLOFF);
                    float accelMag = nozzle.throttle * (GE_FORCE_PER_RAY * 0.45f) * decay;
                    Vec3 f = hit.normal * accelMag;
                    geAccel.z += f.z;
                    Vec3 arm = nozzle.pos - Vec3{m_x, m_y, m_z};
                    geTorquePitch += (arm.z * f.y - arm.y * f.z) * GE_TORQUE_SCALE;
                }
            }
        }
    }

    // Apply continuous 3D ground effect to velocity and rotation
    m_vx += geAccel.x * dt;
    m_vy += geAccel.y * dt;
    m_vz += geAccel.z * dt;
    m_angularVel += geTorqueRoll * dt;
    m_pitchVel   += geTorquePitch * dt;

    if (geAccel.lengthSq() > 0.05f) {
        m_forceVectors.push_back({m_x, m_y + 8.0f * FT2M, m_z, geAccel.x, geAccel.y, geAccel.z, {120, 225, 255, 235}, "G.EFF"});
    }

    // Record net 3D acceleration vector at center of mass
    Vec3 netA = totalWorldA + dragAccel + geAccel;
    if (netA.lengthSq() > 0.05f) {
        m_forceVectors.push_back({m_x, m_y, m_z, netA.x, netA.y, netA.z, {255, 255, 255, 245}, "NET"});
    }

    // ── Instantaneous Impulse Thruster 3D Ground Effect ───────────────────────
    Vec3 impulseGeDeltaV{0.0f, 0.0f, 0.0f};
    float impulseGeTorqueRoll  = 0.0f;
    float impulseGeTorquePitch = 0.0f;

    if (m_impulseBoostActive) {
        for (const auto& nozzle : nozzles) {
            for (int r = 0; r < IMPULSE_GE_NUM_RAYS; ++r) {
                float spread = (IMPULSE_GE_NUM_RAYS > 1)
                    ? IMPULSE_GE_CONE_HALF_ANGLE * (2.0f * r / (IMPULSE_GE_NUM_RAYS - 1) - 1.0f)
                    : 0.0f;
                float cosS = std::cos(spread);
                float sinS = std::sin(spread);

                Vec3 rayDir = (exhaustDir * cosS + perpXDir * sinS).normalized();

                Terrain::RayHit3D hit;
                if (terrain.castRay3D(nozzle.pos, rayDir, IMPULSE_GE_MAX_DIST, hit)) {
                    float decay = std::exp(-hit.dist / IMPULSE_GE_FALLOFF);
                    float dvMag = IMPULSE_GE_DV_PER_RAY * decay;

                    Vec3 dv = hit.normal * dvMag;
                    impulseGeDeltaV += dv;

                    Vec3 arm = nozzle.pos - Vec3{m_x, m_y, m_z};
                    impulseGeTorqueRoll  += (arm.x * dv.y - arm.y * dv.x) * GE_TORQUE_SCALE;
                    impulseGeTorquePitch += (arm.z * dv.y - arm.y * dv.z) * GE_TORQUE_SCALE;
                }
            }
        }

        m_vx += impulseGeDeltaV.x;
        m_vy += impulseGeDeltaV.y;
        m_vz += impulseGeDeltaV.z;
        m_angularVel += impulseGeTorqueRoll;
        m_pitchVel   += impulseGeTorquePitch;
    }

    // Store normalized magnitude for HUD
    static const float GE_MAX_TOTAL = 2.0f * GE_NUM_RAYS * GE_FORCE_PER_RAY;
    float geMag = geAccel.length();
    m_groundEffectMagnitude = std::min(1.0f, geMag / GE_MAX_TOTAL);

    if (m_impulseBoostActive) {
        float impGeMag = impulseGeDeltaV.length();
        if (impGeMag > 0.05f) {
            m_groundEffectMagnitude = std::max(m_groundEffectMagnitude, std::min(1.0f, impGeMag / (IMPULSE_GE_MAX_TOTAL_DV * 0.35f)));
        }
    }

    if (m_hoverThrustersEnabled && m_hoverThrusterThrottle > 0.01f) {
        m_groundEffectMagnitude = std::max(m_groundEffectMagnitude, m_hoverThrusterThrottle);
        float totThr = m_throttleLeft + m_throttleRight;
        sound.setThrust(std::clamp(std::max(totThr * 0.6f, m_hoverThrusterThrottle * 0.35f), 0.0f, 1.0f));
    }

    // Decay RCS puffs
    if (m_rcsPuffLeft > 0.0f)  m_rcsPuffLeft -= dt;
    if (m_rcsPuffRight > 0.0f) m_rcsPuffRight -= dt;
    if (m_rcsPuffFore > 0.0f)  m_rcsPuffFore -= dt;
    if (m_rcsPuffAft > 0.0f)   m_rcsPuffAft -= dt;

    // Flame flicker animation
    m_flameFlicker += dt * 30.0f;
}

void Battleship::updateHoverThrusters(float dt, float rotateInput, float pitchInput, float diffThrust,
                                      const Terrain& terrain, SoundEngine& /*sound*/,
                                      Vec3& accumWorldA, float& accumRollAccel,
                                      float& accumPitchAccel, float& accumYawAccel) {
    if (!m_hoverThrustersEnabled || (!m_unlimitedFuel && m_fuel <= 0.0f)) {
        m_hoverThrusterThrottle = 0.0f;
        return;
    }

    // Sample 3D terrain surface beneath center, port/starboard/fore/aft skids, and 3D velocity lookahead points
    const float lookTimes[4] = { 0.30f, 0.65f, 1.10f, 1.55f };
    float minSurfY = terrain.getSurfaceHeightAt(m_x, m_z);
    minSurfY = std::min(minSurfY, terrain.getSurfaceHeightAt(m_x - 22.0f * FT2M, m_z));
    minSurfY = std::min(minSurfY, terrain.getSurfaceHeightAt(m_x + 22.0f * FT2M, m_z));
    minSurfY = std::min(minSurfY, terrain.getSurfaceHeightAt(m_x - 11.0f * FT2M, m_z));
    minSurfY = std::min(minSurfY, terrain.getSurfaceHeightAt(m_x + 11.0f * FT2M, m_z));
    if (std::abs(m_z) > 1e-3f || std::abs(m_vz) > 1e-3f) {
        minSurfY = std::min(minSurfY, terrain.getSurfaceHeightAt(m_x, m_z - 15.0f * FT2M));
        minSurfY = std::min(minSurfY, terrain.getSurfaceHeightAt(m_x, m_z + 15.0f * FT2M));
    }

    for (float tLook : lookTimes) {
        float lookDx = std::clamp(m_vx * tLook, -110.0f * FT2M, 110.0f * FT2M);
        float lookDz = std::clamp(m_vz * tLook, -110.0f * FT2M, 110.0f * FT2M);
        minSurfY = std::min(minSurfY, terrain.getSurfaceHeightAt(m_x + lookDx, m_z + lookDz));
    }

    float clearance = minSurfY - (m_y + 12.0f * FT2M); // [m]

    // Local & forward 3D terrain slope for contour-following velocity tracking while skating in (X, Z)
    float surfLeft  = terrain.getSurfaceHeightAt(m_x - 15.0f * FT2M, m_z);
    float surfRight = terrain.getSurfaceHeightAt(m_x + 15.0f * FT2M, m_z);
    float localSlopeX = (surfRight - surfLeft) / (30.0f * FT2M);

    float forwardDx = (m_vx >= 0.0f) ? 35.0f * FT2M : -35.0f * FT2M;
    float forwardSlopeX = (terrain.getSurfaceHeightAt(m_x + forwardDx, m_z) - terrain.getSurfaceHeightAt(m_x, m_z)) / forwardDx;

    float terrainVyZ = 0.0f;
    if (std::abs(m_vz) > 0.05f) {
        float forwardDz = (m_vz >= 0.0f) ? 35.0f * FT2M : -35.0f * FT2M;
        float forwardSlopeZ = (terrain.getSurfaceHeightAt(m_x, m_z + forwardDz) - terrain.getSurfaceHeightAt(m_x, m_z)) / forwardDz;
        terrainVyZ = forwardSlopeZ * m_vz;
    }

    float terrainVyLocal = localSlopeX * m_vx + terrainVyZ;
    float terrainVyForward = forwardSlopeX * m_vx + terrainVyZ;
    float terrainVy = std::min(terrainVyLocal, terrainVyForward); // [m/s] (<0 when skating uphill)
    float currentVy = m_vy + accumWorldA.y * dt;
    float relVy = currentVy - terrainVy;

    if (clearance < HOVER_MAX_ALT) {
        float altError = HOVER_TARGET_ALT - clearance;

        // Critically-damped PD + gravity feedforward modulation law
        float desiredUpAccel = GRAVITY + 2.6f * altError + 3.6f * relVy;

        if (relVy > 0.5f && clearance > HOVER_TARGET_ALT) {
            float netMaxDecel = HOVER_MAX_ACCEL - GRAVITY;
            float stopDist = (relVy * relVy) / (2.0f * netMaxDecel);
            if (clearance - HOVER_TARGET_ALT <= stopDist * 1.20f + 2.0f) {
                desiredUpAccel = std::max(desiredUpAccel, HOVER_MAX_ACCEL);
            }
        }

        m_hoverThrusterThrottle = std::clamp(desiredUpAccel / HOVER_MAX_ACCEL, 0.0f, 1.0f);
        float hoverUpAccel = m_hoverThrusterThrottle * HOVER_MAX_ACCEL;

        // Attitude stabilization in Roll and Pitch
        float stabTorqueRoll = 0.0f;
        if (std::abs(rotateInput) < 0.05f && std::abs(diffThrust) < 0.05f) {
            stabTorqueRoll = (-3.2f * m_angle - 2.6f * m_angularVel) * m_hoverThrusterThrottle;
        }
        if (std::abs(pitchInput) < 0.05f) {
            accumPitchAccel += (-3.2f * m_pitch - 2.6f * m_pitchVel) * m_hoverThrusterThrottle;
        }

        float cosA = std::cos(m_angle);
        float sinA = std::sin(m_angle);
        float rx = 14.5f * FT2M;
        float ry = 10.5f * FT2M;

        float deltaA = 0.0f;
        if (std::abs(cosA) > 0.25f) {
            deltaA = (HULL_INERTIA * stabTorqueRoll - hoverUpAccel * ry * sinA) / (2.0f * rx * cosA);
        }
        float hoverLeftUp  = 0.5f * hoverUpAccel + deltaA;
        float hoverRightUp = 0.5f * hoverUpAccel - deltaA;

        // Convert world-upward nozzle force (0, -hoverUp, 0) to local hull frame
        float cP = std::cos(m_pitch);
        float sP = std::sin(m_pitch);
        Vec3 localLeftF  = {-hoverLeftUp * sinA,  -hoverLeftUp * cosA * cP,  hoverLeftUp * cosA * sP};
        Vec3 localRightF = {-hoverRightUp * sinA, -hoverRightUp * cosA * cP, hoverRightUp * cosA * sP};

        applyForceAtLocalPoint3D({-rx, ry, 0.0f}, localLeftF,
                                 accumWorldA, accumRollAccel, accumPitchAccel, accumYawAccel,
                                 "HOV.L", {60, 235, 255, 255});
        applyForceAtLocalPoint3D({ rx, ry, 0.0f}, localRightF,
                                 accumWorldA, accumRollAccel, accumPitchAccel, accumYawAccel,
                                 "HOV.R", {60, 235, 255, 255});

        // Consume hover propellant when unlimited fuel is off
        if (!m_unlimitedFuel && m_hoverThrusterThrottle > 0.0f) {
            float hoverFuel = (m_hoverThrusterThrottle * 0.35f) * FUEL_BURN_RATE * dt;
            m_fuel = std::max(0.0f, m_fuel - hoverFuel);
        }
    } else {
        m_hoverThrusterThrottle = 0.0f;
    }
}

void Battleship::checkTerrainCollision(const Terrain& terrain, SoundEngine& sound) {
    if (m_impactCooldown > 0.0f) {
        return;
    }

    // 3D Test points on footpads, lower chassis, engine nozzles, and cabin top
    std::vector<Vec3> testPoints = {
        {-18.0f * FT2M, 12.0f * FT2M,  0.0f}, { 18.0f * FT2M, 12.0f * FT2M,  0.0f}, // Center footpads
        {-18.0f * FT2M, 12.0f * FT2M, -9.0f * FT2M}, { 18.0f * FT2M, 12.0f * FT2M,  9.0f * FT2M}, // 3D Fore/Aft skid tips
        {-14.0f * FT2M, -4.0f * FT2M,  0.0f}, { 14.0f * FT2M, -4.0f * FT2M,  0.0f}, // Mid body
        {-5.5f * FT2M,   5.0f * FT2M,  0.0f}, {  5.5f * FT2M,  5.0f * FT2M,  0.0f}, // Left & Right nozzles
        { 0.0f * FT2M, -21.0f * FT2M,  0.0f}                                        // Cabin top
    };

    bool anyCollision = false;
    bool touchedPad = false;
    int padMultiplier = 0;
    float padHitY = 0.0f;

    for (size_t i = 1; i < testPoints.size(); ++i) {
        Vec3 w1 = localToWorldPoint(testPoints[i - 1]);
        Vec3 w2 = localToWorldPoint(testPoints[i]);

        bool isPad = false;
        int mult = 0;
        float hY = 0.0f;
        if (terrain.checkCollision3D(w1, w2, isPad, mult, hY)) {
            anyCollision = true;
            if (isPad) {
                touchedPad = true;
                padMultiplier = mult;
                padHitY = hY;
            }
            break;
        }
    }

    if (anyCollision) {
        // Evaluate touchdown safety (only lock into LANDED state if hover thrusters are OFF)
        if (!m_hoverThrustersEnabled && touchedPad && isSafeSpeed() && isSafeAngle() && m_takeoffCooldown <= 0.0f && m_vy >= 0.0f) {
            triggerLanding(padMultiplier, padHitY, sound);
            return;
        }

        // Calculate 3D impact velocity magnitude in [m/s]
        float speed = std::sqrt(m_vx * m_vx + m_vy * m_vy + m_vz * m_vz);

        // Damage is proportional to impact velocity
        float damageInflicted = std::max(2.5f, speed * DAMAGE_PER_VELOCITY);
        m_damage = std::min(100.0f, m_damage + damageInflicted);

        // Store impact event for controller vibration and UI alerts
        m_lastImpact.occurred = true;
        m_lastImpact.speed = speed;
        m_lastImpact.damageInflicted = damageInflicted;

        // If total damage reaches 100%, vehicle breaks apart into 3D tumbling debris
        if (m_damage >= 100.0f) {
            triggerCrash(sound);
            return;
        }

        // Vehicle survives the impact!
        float soundIntensity = std::clamp(speed / (45.0f * FT2M), 0.35f, 1.0f);
        sound.playImpact(soundIntensity);
        m_impactFlashTimer = 0.25f;

        // 3D Rebound physics: bounce vehicle off terrain surface
        if (m_vy > 0.0f) {
            m_vy = -std::max(12.0f * FT2M, m_vy * 0.40f);
        } else {
            m_vy = -m_vy * 0.40f;
        }
        m_vx = -m_vx * 0.35f;
        m_vz = -m_vz * 0.35f;

        // Angular perturbation from impact
        m_angularVel = -m_angularVel * 0.35f + ((m_vx >= 0.0f) ? 0.75f : -0.75f);
        m_pitchVel   = -m_pitchVel * 0.35f;

        // Push vehicle slightly clear of surface to prevent immediate re-collision
        m_y -= 6.0f * FT2M;
        m_impactCooldown = 0.22f;
    }
}

void Battleship::triggerLanding(int multiplier, float hitY, SoundEngine& sound) {
    m_state = BattleshipState::LANDED;
    m_lastPadMultiplier = multiplier;
    m_vx = 0.0f;
    m_vy = 0.0f;
    m_vz = 0.0f;
    m_angularVel = 0.0f;
    m_pitchVel = 0.0f;
    m_yawVel = 0.0f;
    m_angle = 0.0f;
    m_pitch = 0.0f;
    m_landedSurfaceY = hitY - 12.0f * FT2M;
    m_y = m_landedSurfaceY;
    sound.playLandFanfare();
}

void Battleship::spawnThrusterParticles(float dt) {
    if (m_state == BattleshipState::CRASHED) return;
    if (!m_unlimitedFuel && m_fuel <= 0.0f) return;

    auto emitFromNozzle = [&](float nozzleLocalX, float throttle) {
        if (throttle < 0.02f) return;
        float spawnRate = (120.0f + 260.0f * throttle) * throttle;
        int numParticles = (int)(spawnRate * dt);
        if (((float)std::rand() / (float)RAND_MAX) < (spawnRate * dt - numParticles)) {
            numParticles++;
        }

        Vec3 exDir   = localToWorldDir({0.0f, 1.0f, 0.0f});
        Vec3 perpX   = localToWorldDir({1.0f, 0.0f, 0.0f});
        Vec3 perpZ   = localToWorldDir({0.0f, 0.0f, 1.0f});
        Vec3 nozzleW = localToWorldPoint({nozzleLocalX, 5.0f * FT2M, 0.0f});

        float mouthSpread = (0.7f + 0.3f * (1.0f - throttle)) * FT2M;

        for (int i = 0; i < numParticles; ++i) {
            if (m_particles.size() >= 1200) break;
            ThrusterParticle p;
            float offX = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * mouthSpread;
            float offZ = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * mouthSpread;
            Vec3 pos = nozzleW + perpX * offX + perpZ * offZ;
            p.x = pos.x;
            p.y = pos.y;
            p.z = pos.z;

            float speedFt = 90.0f + 380.0f * throttle + (((float)std::rand() / (float)RAND_MAX) * 50.0f * throttle);
            float speed = speedFt * FT2M;

            float maxHalfAngle = 0.065f - 0.035f * throttle;
            float coneX = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * (2.0f * maxHalfAngle);
            float coneZ = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * (2.0f * maxHalfAngle);
            Vec3 dir = (exDir + perpX * std::sin(coneX) + perpZ * std::sin(coneZ)).normalized();

            p.vx = m_vx * 0.25f + dir.x * speed;
            p.vy = m_vy * 0.25f + dir.y * speed;
            p.vz = m_vz * 0.25f + dir.z * speed;

            float baseLife = 0.16f + 0.22f * throttle;
            p.maxLife = baseLife + (((float)std::rand() / (float)RAND_MAX) * 0.08f);
            p.life = p.maxLife;
            p.bounceCount = 0;

            int colorRand = std::rand() % 100;
            if (throttle > 0.65f && colorRand < 40) {
                p.color = {255, 255, 245, 255};
            } else if (colorRand < 30) {
                p.color = {255, 240, 180, 255};
            } else if (colorRand < 75) {
                p.color = {255, 175, 40, 255};
            } else {
                p.color = {255, 100, 25, 255};
            }
            m_particles.push_back(p);
        }
    };

    emitFromNozzle(-5.5f * FT2M, m_throttleLeft);
    emitFromNozzle( 5.5f * FT2M, m_throttleRight);
}

void Battleship::spawnHoverParticles(float dt) {
    if (m_state == BattleshipState::CRASHED) return;
    if (!m_hoverThrustersEnabled || m_hoverThrusterThrottle < 0.03f) return;
    if (!m_unlimitedFuel && m_fuel <= 0.0f) return;

    auto emitFromVentralHoverJet = [&](float jetLocalX, float modThrottle) {
        float spawnRate = 110.0f * modThrottle;
        int numParticles = (int)(spawnRate * dt);
        if (((float)std::rand() / (float)RAND_MAX) < (spawnRate * dt - numParticles)) {
            numParticles++;
        }

        Vec3 jetW = localToWorldPoint({jetLocalX, 10.5f * FT2M, 0.0f});

        for (int i = 0; i < numParticles; ++i) {
            if (m_particles.size() >= 1200) break;
            ThrusterParticle p;
            float spreadX = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * 3.0f * FT2M;
            float spreadZ = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * 6.0f * FT2M;
            p.x = jetW.x + spreadX;
            p.y = jetW.y;
            p.z = jetW.z + spreadZ;

            float speed = (80.0f + 160.0f * modThrottle) * FT2M;
            float coneX = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * 0.45f;
            float coneZ = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * 0.45f;
            p.vx = m_vx * 0.3f + std::sin(coneX) * speed;
            p.vy = m_vy * 0.2f + std::cos(coneX) * speed;
            p.vz = m_vz * 0.3f + std::sin(coneZ) * speed;
            p.maxLife = 0.22f + 0.14f * modThrottle;
            p.life = p.maxLife;
            p.bounceCount = 0;

            int cRand = std::rand() % 3;
            if (cRand == 0) {
                p.color = {200, 250, 255, 255};
            } else if (cRand == 1) {
                p.color = {80, 220, 255, 255};
            } else {
                p.color = {60, 175, 245, 255};
            }
            m_particles.push_back(p);
        }
    };

    emitFromVentralHoverJet(-14.5f * FT2M, m_hoverThrusterThrottle);
    emitFromVentralHoverJet( 14.5f * FT2M, m_hoverThrusterThrottle);
}

void Battleship::spawnImpulseParticles() {
    auto emitImpulseFromNozzle = [&](float nozzleLocalX) {
        Vec3 exDir   = localToWorldDir({0.0f, 1.0f, 0.0f});
        Vec3 perpX   = localToWorldDir({1.0f, 0.0f, 0.0f});
        Vec3 perpZ   = localToWorldDir({0.0f, 0.0f, 1.0f});
        Vec3 nozzleW = localToWorldPoint({nozzleLocalX, 5.0f * FT2M, 0.0f});

        for (int i = 0; i < 85; ++i) {
            if (m_particles.size() >= 1400) break;
            ThrusterParticle p;
            p.isImpulse = true;

            float mouthOffset = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * 2.5f * FT2M;
            Vec3 pos = nozzleW + perpX * mouthOffset;
            p.x = pos.x;
            p.y = pos.y;
            p.z = pos.z;

            float blastAngle = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * 4.7f;
            float blastZ     = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * 1.4f;
            Vec3 dir = (exDir * std::cos(blastAngle) + perpX * std::sin(blastAngle) + perpZ * std::sin(blastZ)).normalized();

            float speed = (380.0f + (std::rand() % 340)) * FT2M;

            p.vx = m_vx * 0.15f + dir.x * speed;
            p.vy = m_vy * 0.15f + dir.y * speed;
            p.vz = m_vz * 0.15f + dir.z * speed;
            p.maxLife = 0.40f + ((float)(std::rand() % 35) / 100.0f);
            p.life = p.maxLife;
            p.bounceCount = 0;

            int colorType = std::rand() % 4;
            if (colorType == 0) {
                p.color = {255, 255, 255, 255};
            } else if (colorType == 1) {
                p.color = {210, 245, 255, 255};
            } else if (colorType == 2) {
                p.color = {255, 230, 90, 255};
            } else {
                p.color = {255, 140, 35, 255};
            }
            m_particles.push_back(p);
        }
    };

    emitImpulseFromNozzle(-5.5f * FT2M);
    emitImpulseFromNozzle( 5.5f * FT2M);
}

void Battleship::updateThrusterParticles(float dt, const Terrain& terrain) {
    for (auto& p : m_particles) {
        if (p.life <= 0.0f) continue;

        float oldX = p.x;
        float oldY = p.y;
        float oldZ = p.z;

        p.vy += GRAVITY * 0.25f * dt;
        p.vx *= (1.0f - 0.30f * dt);
        p.vy *= (1.0f - 0.30f * dt);
        p.vz *= (1.0f - 0.30f * dt);

        float nextX = p.x + p.vx * dt;
        float nextY = p.y + p.vy * dt;
        float nextZ = p.z + p.vz * dt;

        float moveDx = nextX - oldX;
        float moveDy = nextY - oldY;
        float moveDz = nextZ - oldZ;
        float moveDist2D = std::sqrt(moveDx * moveDx + moveDy * moveDy);
        (void)moveDz;

        bool bounced = false;
        if (moveDist2D > 1e-4f) {
            float dirX = moveDx / moveDist2D;
            float dirY = moveDy / moveDist2D;

            Terrain::RayHit hit;
            if (terrain.castRay(oldX, oldY, dirX, dirY, moveDist2D, hit)) {
                float dot = p.vx * hit.normalX + p.vy * hit.normalY;
                if (dot < 0.0f) {
                    float inSpeed = std::sqrt(p.vx * p.vx + p.vy * p.vy + p.vz * p.vz);
                    float tangentX = -hit.normalY;
                    float tangentY =  hit.normalX;

                    float psi = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * 2.7f;
                    float cosPsi = std::cos(psi);
                    float sinPsi = std::sin(psi);

                    float rebDirX = hit.normalX * cosPsi + tangentX * sinPsi;
                    float rebDirY = hit.normalY * cosPsi + tangentY * sinPsi;
                    float rebDirZ = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * 0.65f;

                    float speedRetain = p.isImpulse ? (0.65f + ((float)(std::rand() % 30) / 100.0f))
                                                    : (0.45f + ((float)(std::rand() % 35) / 100.0f));
                    float reboundSpeed = std::max(35.0f * FT2M, inSpeed * speedRetain);

                    p.vx = rebDirX * reboundSpeed;
                    p.vy = rebDirY * reboundSpeed;
                    p.vz = rebDirZ * reboundSpeed;

                    p.x = oldX + dirX * hit.dist + hit.normalX * 0.25f;
                    p.y = oldY + dirY * hit.dist + hit.normalY * 0.25f;
                    p.z = nextZ;
                    p.bounceCount++;
                    p.life *= (p.isImpulse ? 0.90f : 0.85f);
                    bounced = true;
                }
            }
        }

        if (!bounced) {
            p.x = nextX;
            p.y = nextY;
            p.z = nextZ;
            float surfY = terrain.getSurfaceHeightAt(p.x, p.z);
            if (p.y >= surfY) {
                p.y = surfY - 0.2f;
                float currentSpeed = std::sqrt(p.vx * p.vx + p.vy * p.vy + p.vz * p.vz);
                float rebSpeed = std::max(30.0f * FT2M, currentSpeed * (p.isImpulse ? 0.70f : 0.50f));
                float angle = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * 2.65f;
                float zAngle = (((float)std::rand() / (float)RAND_MAX) - 0.5f) * 1.2f;
                p.vx = std::sin(angle) * rebSpeed;
                p.vy = -std::abs(std::cos(angle)) * rebSpeed;
                p.vz = std::sin(zAngle) * rebSpeed * 0.5f;
                p.bounceCount++;
                p.life *= (p.isImpulse ? 0.90f : 0.85f);
            }
        }

        p.life -= dt;
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(), [](const ThrusterParticle& p) {
            return p.life <= 0.0f;
        }),
        m_particles.end()
    );
}

void Battleship::triggerCrash(SoundEngine& sound) {
    m_state = BattleshipState::CRASHED;
    sound.playExplosion();

    // Break 3D craft wireframe into tumbling 3D vector debris pieces
    m_debris.clear();
    for (const auto& line : m_craftLines3D) {
        DebrisLine d;
        Vec3 w1 = localToWorldPoint({line.x1, line.y1, line.z1});
        Vec3 w2 = localToWorldPoint({line.x2, line.y2, line.z2});
        d.x1 = w1.x; d.y1 = w1.y; d.z1 = w1.z;
        d.x2 = w2.x; d.y2 = w2.y; d.z2 = w2.z;

        float centerLx = (line.x1 + line.x2) * 0.5f;
        float centerLy = (line.y1 + line.y2) * 0.5f;
        float centerLz = (line.z1 + line.z2) * 0.5f;
        float blastSpeed = (15.0f + (std::rand() % 45)) * FT2M;
        d.vx = m_vx * 0.4f + centerLx * blastSpeed * 0.08f + (((std::rand() % 30) - 15) * FT2M);
        d.vy = m_vy * 0.4f + centerLy * blastSpeed * 0.08f - ((std::rand() % 35) * FT2M);
        d.vz = m_vz * 0.4f + centerLz * blastSpeed * 0.08f + (((std::rand() % 30) - 15) * FT2M);
        d.angle = m_angle;
        d.angularVel = ((std::rand() % 100) - 50) * 0.1f;
        d.life = 1.0f;

        m_debris.push_back(d);
    }
}

void Battleship::renderDropShadow(SDL_Renderer* renderer, const Camera2D& cam, const Terrain& terrain, float glow) const {
    if (m_state == BattleshipState::CRASHED || !cam.use3D) return;

    float surfY = terrain.getSurfaceHeightAt(m_x, m_z);
    float skidY = m_y + 12.0f * FT2M;
    float alt = std::max(0.0f, surfY - skidY);

    // 1. Vertical Laser Plumb-Line from keel to lunar surface
    SDL_Color plumbColor = isSafeSpeed() && isSafeAngle()
        ? SDL_Color{90, 255, 160, 130}
        : SDL_Color{255, 140, 90, 130};
    VectorRenderer::drawLineWorld3D(renderer, cam, m_x, skidY, m_z, m_x, surfY, m_z, plumbColor, glow * 0.85f, 1);

    // 2. 3D Projected Ground Target Reticle on Terrain Surface directly beneath craft
    float rX = 18.0f * FT2M * SHIP_SCALE;
    float rZ = 12.0f * FT2M * SHIP_SCALE;
    float yFL = terrain.getSurfaceHeightAt(m_x - rX, m_z + rZ) - 0.15f;
    float yFR = terrain.getSurfaceHeightAt(m_x + rX, m_z + rZ) - 0.15f;
    float yBL = terrain.getSurfaceHeightAt(m_x - rX, m_z - rZ) - 0.15f;
    float yBR = terrain.getSurfaceHeightAt(m_x + rX, m_z - rZ) - 0.15f;

    SDL_Color shadowCol = (alt < 40.0f) ? SDL_Color{110, 255, 220, 210} : SDL_Color{90, 190, 170, 150};
    VectorRenderer::drawLineWorld3D(renderer, cam, m_x - rX, yFL, m_z + rZ, m_x + rX, yFR, m_z + rZ, shadowCol, glow, 1);
    VectorRenderer::drawLineWorld3D(renderer, cam, m_x + rX, yFR, m_z + rZ, m_x + rX, yBR, m_z - rZ, shadowCol, glow, 1);
    VectorRenderer::drawLineWorld3D(renderer, cam, m_x + rX, yBR, m_z - rZ, m_x - rX, yBL, m_z - rZ, shadowCol, glow, 1);
    VectorRenderer::drawLineWorld3D(renderer, cam, m_x - rX, yBL, m_z - rZ, m_x - rX, yFL, m_z + rZ, shadowCol, glow, 1);

    // Surface crosshair ticks
    VectorRenderer::drawLineWorld3D(renderer, cam, m_x - 4.0f, surfY - 0.15f, m_z, m_x + 4.0f, surfY - 0.15f, m_z, shadowCol, glow * 1.2f, 2);
    VectorRenderer::drawLineWorld3D(renderer, cam, m_x, surfY - 0.15f, m_z - 4.0f, m_x, surfY - 0.15f, m_z + 4.0f, shadowCol, glow * 1.2f, 2);
}

void Battleship::render(SDL_Renderer* renderer, const Camera2D& cam, float glow) {
    SDL_Color craftColor = {240, 255, 240, 255};
    if (m_impactFlashTimer > 0.0f) {
        craftColor = {255, 110, 90, 255};
        glow *= 1.8f;
    } else if (m_damage > 65.0f && std::fmod(m_flameFlicker, 6.0f) < 1.5f) {
        craftColor = {255, 190, 140, 255};
    }
    SDL_Color rcsColor = {100, 220, 255, 255};

    // 1. Draw Expelled 3D Thruster Particle Debris
    for (const auto& p : m_particles) {
        if (p.life <= 0.0f) continue;
        float lifeRatio = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
        Uint8 alpha = (Uint8)(lifeRatio * 255.0f);

        SDL_Color col = p.color;
        col.a = alpha;

        float speed = std::sqrt(p.vx * p.vx + p.vy * p.vy + p.vz * p.vz);

        float streakLen;
        int thickness = 1;
        float particleGlow = glow * 1.4f;

        if (p.isImpulse) {
            streakLen = std::clamp(speed * 0.075f, 3.5f * FT2M, 24.0f * FT2M);
            thickness = 2;
            particleGlow = glow * 2.4f;
        } else {
            float minLen = (p.bounceCount > 0) ? 0.6f * FT2M : 0.4f * FT2M;
            streakLen = std::clamp(speed * 0.035f, minLen, 5.0f * FT2M);
        }

        Vec3 p1{p.x, p.y, p.z};
        Vec3 vDir = Vec3{p.vx, p.vy, p.vz} / (speed + 1e-4f);
        Vec3 p2 = p1 - vDir * streakLen;

        VectorRenderer::drawLineWorld3D(renderer, cam, p1, p2, col, particleGlow, thickness);
    }

    if (m_state == BattleshipState::CRASHED) {
        for (const auto& d : m_debris) {
            if (d.life <= 0.0f) continue;
            Uint8 alpha = (Uint8)(std::clamp(d.life, 0.0f, 1.0f) * 255.0f);
            SDL_Color debColor = {craftColor.r, craftColor.g, craftColor.b, alpha};
            VectorRenderer::drawLineWorld3D(renderer, cam, d.x1, d.y1, d.z1, d.x2, d.y2, d.z2, debColor, glow);
        }
        return;
    }

    // 2. Draw 3D Craft Wireframe (or 2D profile when in classic 2D mode)
    if (cam.use3D) {
        for (const auto& line : m_craftLines3D) {
            Vec3 w1 = localToWorldPoint({line.x1, line.y1, line.z1});
            Vec3 w2 = localToWorldPoint({line.x2, line.y2, line.z2});
            VectorRenderer::drawLineWorld3D(renderer, cam, w1, w2, craftColor, glow, 2);
        }
    }
    // Always draw primary silhouette lines at z=0 for crisp profile readability
    for (const auto& line : m_craftLines) {
        Vec3 w1 = localToWorldPoint({line.x1, line.y1, 0.0f});
        Vec3 w2 = localToWorldPoint({line.x2, line.y2, 0.0f});
        VectorRenderer::drawLineWorld3D(renderer, cam, w1, w2, craftColor, glow, 2);
    }

    auto scaleLocalPt = [](float lx, float ly, float lz) -> Vec3 {
        constexpr float skidY = 12.0f * FT2M;
        return {
            lx * SHIP_SCALE,
            skidY + (ly - skidY) * SHIP_SCALE,
            lz * SHIP_SCALE
        };
    };

    // 3. Draw 3D Steady Nozzle Core Plume Cones
    auto drawSteadyNozzleFlare3D = [&](float nozzleCenterX, float throttle) {
        float flameLength = (4.0f * FT2M + throttle * 12.0f * FT2M) * SHIP_SCALE;
        float flameWidth = 2.4f * FT2M * SHIP_SCALE;
        float nozY = 12.0f * FT2M + (5.0f * FT2M - 12.0f * FT2M) * SHIP_SCALE;
        float nozX = nozzleCenterX * SHIP_SCALE;

        Vec3 tip = localToWorldPoint({nozX, nozY + flameLength, 0.0f});
        Vec3 r1  = localToWorldPoint({nozX - flameWidth, nozY, 0.0f});
        Vec3 r2  = localToWorldPoint({nozX + flameWidth, nozY, 0.0f});
        Vec3 r3  = localToWorldPoint({nozX, nozY, -flameWidth});
        Vec3 r4  = localToWorldPoint({nozX, nozY,  flameWidth});

        SDL_Color flareColor = {255, 235, 160, 255};
        VectorRenderer::drawLineWorld3D(renderer, cam, r1, tip, flareColor, glow * 1.5f, 2);
        VectorRenderer::drawLineWorld3D(renderer, cam, r2, tip, flareColor, glow * 1.5f, 2);
        VectorRenderer::drawLineWorld3D(renderer, cam, r3, tip, flareColor, glow * 1.5f, 2);
        VectorRenderer::drawLineWorld3D(renderer, cam, r4, tip, flareColor, glow * 1.5f, 2);
        VectorRenderer::drawLineWorld3D(renderer, cam, r1, r2,  flareColor, glow * 1.2f, 1);
    };

    if (m_throttleLeft > 0.02f && (m_unlimitedFuel || m_fuel > 0.0f)) {
        drawSteadyNozzleFlare3D(-5.5f * FT2M, m_throttleLeft);
    }
    if (m_throttleRight > 0.02f && (m_unlimitedFuel || m_fuel > 0.0f)) {
        drawSteadyNozzleFlare3D(5.5f * FT2M, m_throttleRight);
    }

    // 4. Draw 3D RCS Attitude Thruster Puffs (Roll & Pitch)
    if (m_rcsPuffLeft > 0.0f && m_state == BattleshipState::FLYING) {
        Vec3 p1 = localToWorldPoint(scaleLocalPt(-14.0f * FT2M, -8.0f * FT2M, 0.0f));
        Vec3 p2 = localToWorldPoint(scaleLocalPt(-22.0f * FT2M, -8.0f * FT2M, 0.0f));
        VectorRenderer::drawLineWorld3D(renderer, cam, p1, p2, rcsColor, glow * 1.5f, 2);
    }
    if (m_rcsPuffRight > 0.0f && m_state == BattleshipState::FLYING) {
        Vec3 p1 = localToWorldPoint(scaleLocalPt(14.0f * FT2M, -8.0f * FT2M, 0.0f));
        Vec3 p2 = localToWorldPoint(scaleLocalPt(22.0f * FT2M, -8.0f * FT2M, 0.0f));
        VectorRenderer::drawLineWorld3D(renderer, cam, p1, p2, rcsColor, glow * 1.5f, 2);
    }
    if (m_rcsPuffAft > 0.0f && m_state == BattleshipState::FLYING) {
        Vec3 p1 = localToWorldPoint(scaleLocalPt(0.0f, -8.0f * FT2M, -11.0f * FT2M));
        Vec3 p2 = localToWorldPoint(scaleLocalPt(0.0f, -8.0f * FT2M, -19.0f * FT2M));
        VectorRenderer::drawLineWorld3D(renderer, cam, p1, p2, rcsColor, glow * 1.5f, 2);
    }
    if (m_rcsPuffFore > 0.0f && m_state == BattleshipState::FLYING) {
        Vec3 p1 = localToWorldPoint(scaleLocalPt(0.0f, -8.0f * FT2M, 11.0f * FT2M));
        Vec3 p2 = localToWorldPoint(scaleLocalPt(0.0f, -8.0f * FT2M, 19.0f * FT2M));
        VectorRenderer::drawLineWorld3D(renderer, cam, p1, p2, rcsColor, glow * 1.5f, 2);
    }

    // 5. Draw 3D Deployed Air Brake Flaps
    if (m_airBrakeDeploy > 0.02f && m_state != BattleshipState::CRASHED) {
        SDL_Color brakeColor = {255, 195, 70, 255};
        float d = m_airBrakeDeploy;
        auto drawLocalSeg3D = [&](float lx1, float ly1, float lz1, float lx2, float ly2, float lz2, int thick = 2) {
            Vec3 w1 = localToWorldPoint(scaleLocalPt(lx1, ly1, lz1));
            Vec3 w2 = localToWorldPoint(scaleLocalPt(lx2, ly2, lz2));
            VectorRenderer::drawLineWorld3D(renderer, cam, w1, w2, brakeColor, glow * 1.4f, thick);
        };

        for (float z : {-4.5f * FT2M, 4.5f * FT2M}) {
            drawLocalSeg3D(-15.0f * FT2M, -7.5f * FT2M, z, (-15.0f - 8.5f * d) * FT2M, (-7.5f - 6.0f * d) * FT2M, z, 2);
            drawLocalSeg3D( 15.0f * FT2M, -7.5f * FT2M, z, ( 15.0f + 8.5f * d) * FT2M, (-7.5f - 6.0f * d) * FT2M, z, 2);
            drawLocalSeg3D(-19.5f * FT2M, -2.5f * FT2M, z, (-19.5f - 6.5f * d) * FT2M, (-2.5f + 5.0f * d) * FT2M, z, 2);
            drawLocalSeg3D( 19.5f * FT2M, -2.5f * FT2M, z, ( 19.5f + 6.5f * d) * FT2M, (-2.5f + 5.0f * d) * FT2M, z, 2);
        }
    }

    // 6. Draw 3D Ventral Ground Effect Hover Thruster Plumes
    if (m_hoverThrustersEnabled && m_hoverThrusterThrottle > 0.02f &&
        (m_unlimitedFuel || m_fuel > 0.0f) && m_state != BattleshipState::CRASHED) {
        SDL_Color hoverColor = {80, 230, 255, 255};
        float hLen = (3.5f + 10.5f * m_hoverThrusterThrottle) * FT2M;
        float hWid = 2.0f * FT2M;

        auto drawHoverCone3D = [&](float centerX, float centerZ) {
            Vec3 w1  = localToWorldPoint(scaleLocalPt(centerX - hWid, 10.5f * FT2M, centerZ));
            Vec3 w2  = localToWorldPoint(scaleLocalPt(centerX + hWid, 10.5f * FT2M, centerZ));
            Vec3 w3  = localToWorldPoint(scaleLocalPt(centerX, 10.5f * FT2M + hLen, centerZ));
            VectorRenderer::drawLineWorld3D(renderer, cam, w1, w3, hoverColor, glow * 1.6f, 2);
            VectorRenderer::drawLineWorld3D(renderer, cam, w2, w3, hoverColor, glow * 1.6f, 2);
            VectorRenderer::drawLineWorld3D(renderer, cam, w1, w2, hoverColor, glow * 1.2f, 1);
        };

        drawHoverCone3D(-14.5f * FT2M, -6.0f * FT2M);
        drawHoverCone3D(-14.5f * FT2M,  6.0f * FT2M);
        drawHoverCone3D( 14.5f * FT2M, -6.0f * FT2M);
        drawHoverCone3D( 14.5f * FT2M,  6.0f * FT2M);
    }

    // 7. Draw Real-Time 3D Force Vector Visualization Overlay
    if (m_showForceVectors && m_state != BattleshipState::CRASHED) {
        renderForceVectors(renderer, cam, glow);
    }
}

void Battleship::renderForceVectors(SDL_Renderer* renderer, const Camera2D& cam, float glow) {
    static const float ACCEL_TO_WORLD_M = 1.60f;

    // Draw 3D Center of Mass (CM) reticle
    float cmR = 1.8f * FT2M;
    SDL_Color cmColor = {255, 255, 255, 200};
    VectorRenderer::drawLineWorld3D(renderer, cam, m_x - cmR, m_y, m_z, m_x + cmR, m_y, m_z, cmColor, glow, 2);
    VectorRenderer::drawLineWorld3D(renderer, cam, m_x, m_y - cmR, m_z, m_x, m_y + cmR, m_z, cmColor, glow, 2);
    VectorRenderer::drawLineWorld3D(renderer, cam, m_x, m_y, m_z - cmR, m_x, m_y, m_z + cmR, cmColor, glow, 2);

    for (const auto& fv : m_forceVectors) {
        Vec3 aVec{fv.ax, fv.ay, fv.az};
        float mag = aVec.length();
        if (mag < 0.15f) continue;

        Vec3 dir = aVec / mag;
        Vec3 perp = Vec3{-dir.y, dir.x, 0.0f};
        if (perp.lengthSq() < 0.05f) {
            perp = Vec3{1.0f, 0.0f, 0.0f};
        } else {
            perp = perp.normalized();
        }

        float worldLen = std::clamp(mag * ACCEL_TO_WORLD_M, 4.5f, 55.0f);
        Vec3 anchor{fv.wx, fv.wy, fv.wz};
        Vec3 tip = anchor + dir * worldLen;

        // Draw diamond anchor at force application point
        float anchorR = 1.4f * FT2M;
        VectorRenderer::drawLineWorld3D(renderer, cam, anchor.x - anchorR, anchor.y, anchor.z, anchor.x, anchor.y - anchorR, anchor.z, fv.color, glow * 1.2f, 2);
        VectorRenderer::drawLineWorld3D(renderer, cam, anchor.x, anchor.y - anchorR, anchor.z, anchor.x + anchorR, anchor.y, anchor.z, fv.color, glow * 1.2f, 2);
        VectorRenderer::drawLineWorld3D(renderer, cam, anchor.x + anchorR, anchor.y, anchor.z, anchor.x, anchor.y + anchorR, anchor.z, fv.color, glow * 1.2f, 2);
        VectorRenderer::drawLineWorld3D(renderer, cam, anchor.x, anchor.y + anchorR, anchor.z, anchor.x - anchorR, anchor.y, anchor.z, fv.color, glow * 1.2f, 2);

        // Draw primary 3D force shaft
        int thickness = (fv.label == "NET") ? 3 : 2;
        VectorRenderer::drawLineWorld3D(renderer, cam, anchor, tip, fv.color, glow * 1.6f, thickness);

        // Draw 3D arrowhead barbs
        float headLen = std::clamp(worldLen * 0.22f, 1.5f, 5.5f);
        float headWid = headLen * 0.58f;
        Vec3 barb1 = tip - dir * headLen + perp * headWid;
        Vec3 barb2 = tip - dir * headLen - perp * headWid;

        VectorRenderer::drawLineWorld3D(renderer, cam, tip, barb1, fv.color, glow * 1.6f, thickness);
        VectorRenderer::drawLineWorld3D(renderer, cam, tip, barb2, fv.color, glow * 1.6f, thickness);
        VectorRenderer::drawLineWorld3D(renderer, cam, barb1, barb2, fv.color, glow * 1.3f, 2);
    }
}
