#pragma once

#include "VectorRenderer.hpp"
#include <vector>
#include <string>

struct LandingPad {
    float x1, x2;   // [m] Pad horizontal bounds (left and right)
    float y;        // [m] Pad surface height / altitude datum
    int multiplier; // [dimensionless] Score multiplier (2x, 3x, 4x, 5x)
    std::string label;
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

    // Collision detection
    // Returns true if segment (px1, py1) -> (px2, py2) collides with terrain.
    // Coordinates in [m].
    // If collides on a landing pad, outPadMultiplier is populated.
    bool checkCollision(float px1, float py1, float px2, float py2, 
                        bool& outIsPad, int& outPadMultiplier, float& outHitY) const;

    float getSurfaceHeightAt(float x) const; // x in [m], returns [m]

    // Ray casting for ground effect plume impingement.
    // Fires a ray from (ox,oy) in direction (dx,dy) up to maxDist.
    // Coordinates and maxDist in [m].
    // Returns true and populates outHit if any terrain segment is struck.
    struct RayHit {
        float dist;               // [m] Distance along ray to intersection
        float normalX, normalY;   // [dimensionless] Surface normal unit vector pointing toward ray origin
    };
    bool castRay(float ox, float oy, float dx, float dy, float maxDist, RayHit& outHit) const;
    const std::vector<LandingPad>& getLandingPads() const { return m_pads; }
    std::string getSiteName() const { return m_siteName; }
    MissionSite getCurrentSite() const { return m_currentSite; }

    float getWorldWidth() const { return m_worldWidth; }   // [m] World width
    float getWorldHeight() const { return m_worldHeight; } // [m] World height

private:
    void generateTranquilityBase();
    void generateTychoCrater();
    void generateOceanOfStorms();
    void generateHadleyRille();
    void buildSegments();

    MissionSite m_currentSite = MissionSite::TRANQUILITY_BASE;
    std::string m_siteName;
    std::vector<std::pair<float, float>> m_vertices; // [m] Terrain vertices
    std::vector<TerrainSegment> m_segments;          // [m] Terrain segments
    std::vector<LandingPad> m_pads;                  // [m] Landing pad targets

    float m_worldWidth = 1463.04f;   // [m] Terrain world width: 1463.04 m (converted from 4800.0 ft)
    float m_worldHeight = 670.56f;   // [m] Terrain world height: 670.56 m (converted from 2200.0 ft)
};
