#pragma once

#include "VectorRenderer.hpp"
#include <vector>
#include <string>

struct LandingPad {
    float x1, x2;   // [m] Pad horizontal bounds (East/West)
    float y;        // [m] Pad surface height / altitude datum (+Y is down)
    int multiplier; // [dimensionless] Score multiplier (2x, 3x, 4x, 5x)
    std::string label;
    float z1 = -33.528f; // [m] Pad depth bounds South (-110 ft)
    float z2 =  33.528f; // [m] Pad depth bounds North (+110 ft)
};

struct TerrainSegment {
    float x1, y1;       // [m] Segment start coordinates
    float x2, y2;       // [m] Segment end coordinates
    bool isLandingPad = false;
    int multiplier = 0; // [dimensionless] Score multiplier
};

enum class MissionSite {
    TRANQUILITY_BASE = 0,
    TYCHO_CRATER,
    OCEAN_OF_STORMS,
    HADLEY_RILLE,
    COUNT
};

class Terrain {
public:
    Terrain();
    void loadSite(MissionSite site);
    void cycleSite();

    void render(SDL_Renderer* renderer, const Camera2D& cam, float glow = 1.0f);

    // 2D Collision detection (Z = 0 slice)
    bool checkCollision(float px1, float py1, float px2, float py2,
                        bool& outIsPad, int& outPadMultiplier, float& outHitY) const;

    // 3D Collision detection against the 2D heightfield y = h(x, z)
    bool checkCollision3D(const Vec3& p1, const Vec3& p2,
                          bool& outIsPad, int& outPadMultiplier, float& outHitY,
                          Vec3* outNormal = nullptr) const;

    // Heightfield & normal queries (z defaults to 0.0f for 2D callers)
    float getSurfaceHeightAt(float x, float z = 0.0f) const; // x, z in [m], returns [m]
    Vec3 getSurfaceNormalAt(float x, float z = 0.0f) const;  // returns unit normal pointing into sky (ny < 0)
    bool isPointOnPad(float x, float z, int& outMultiplier, float& outPadY) const;

    // 2D Ray casting for ground effect plume impingement
    struct RayHit {
        float dist;               // [m] Distance along ray to intersection
        float normalX, normalY;   // [dimensionless] Surface normal unit vector pointing toward ray origin
    };
    bool castRay(float ox, float oy, float dx, float dy, float maxDist, RayHit& outHit) const;

    // 3D Ray casting for 3D ground effect cone & 3D particle ricochets
    struct RayHit3D {
        float dist = 0.0f;        // [m] Distance along ray to intersection
        Vec3 point{0.0f, 0.0f, 0.0f};  // [m] World-space hit point
        Vec3 normal{0.0f, -1.0f, 0.0f};// [dimensionless] Unit surface normal pointing into sky
        bool isPad = false;
        int padMultiplier = 0;
    };
    bool castRay3D(const Vec3& origin, const Vec3& dir, float maxDist, RayHit3D& outHit) const;

    const std::vector<LandingPad>& getLandingPads() const { return m_pads; }
    std::string getSiteName() const { return m_siteName; }
    MissionSite getCurrentSite() const { return m_currentSite; }

    float getWorldWidth() const { return m_worldWidth; }   // [m] World East-West width (X)
    float getWorldHeight() const { return m_worldHeight; } // [m] World vertical height (Y)
    float getWorldDepth() const { return m_worldDepth; }   // [m] World North-South depth (Z)

private:
    void generateTranquilityBase();
    void generateTychoCrater();
    void generateOceanOfStorms();
    void generateHadleyRille();
    void buildSegments();
    float getCenterlineHeightAt(float x) const;

    MissionSite m_currentSite = MissionSite::TRANQUILITY_BASE;
    std::string m_siteName;
    std::vector<std::pair<float, float>> m_vertices; // [m] Centerline (Z=0) terrain vertices
    std::vector<TerrainSegment> m_segments;          // [m] Centerline terrain segments
    std::vector<LandingPad> m_pads;                  // [m] 3D Landing pad targets

    float m_worldWidth = 4389.12f;   // [m] Terrain world width: 4389.12 m (14400.0 ft)
    float m_worldHeight = 853.44f;   // [m] Terrain world height: 853.44 m (2800.0 ft)
    float m_worldDepth = 2194.56f;   // [m] Terrain world depth: 2194.56 m (7200.0 ft, z in [-1097.28, +1097.28])
};
