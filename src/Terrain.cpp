#include "Terrain.hpp"
#include "VectorFont.hpp"
#include <algorithm>
#include <cmath>

// Length conversion factor from feet to SI meters
static constexpr float FT2M = 0.3048f; // [m/ft]

static bool lineSegmentsIntersect(float p0_x, float p0_y, float p1_x, float p1_y,
                                  float p2_x, float p2_y, float p3_x, float p3_y,
                                  float& i_x, float& i_y) {
    float s1_x = p1_x - p0_x;
    float s1_y = p1_y - p0_y;
    float s2_x = p3_x - p2_x;
    float s2_y = p3_y - p2_y;

    float denom = (-s2_x * s1_y + s1_x * s2_y);
    if (std::abs(denom) < 1e-6f) return false;

    float s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / denom;
    float t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / denom;

    if (s >= 0.0f && s <= 1.0f && t >= 0.0f && t <= 1.0f) {
        i_x = p0_x + (t * s1_x);
        i_y = p0_y + (t * s1_y);
        return true;
    }
    return false;
}

Terrain::Terrain() {
    loadSite(MissionSite::TRANQUILITY_BASE);
}

void Terrain::cycleSite() {
    int next = ((int)m_currentSite + 1) % (int)MissionSite::COUNT;
    loadSite((MissionSite)next);
}

void Terrain::loadSite(MissionSite site) {
    m_currentSite = site;
    m_vertices.clear();
    m_pads.clear();
    m_segments.clear();
    m_worldDepth = 7200.0f * FT2M; // [m] 2194.56 m total depth (-1097.28 m to +1097.28 m)

    switch (site) {
        case MissionSite::TRANQUILITY_BASE:
            generateTranquilityBase();
            break;
        case MissionSite::TYCHO_CRATER:
            generateTychoCrater();
            break;
        case MissionSite::OCEAN_OF_STORMS:
            generateOceanOfStorms();
            break;
        case MissionSite::HADLEY_RILLE:
            generateHadleyRille();
            break;
        default:
            generateTranquilityBase();
            break;
    }

    buildSegments();
}

void Terrain::generateTranquilityBase() {
    m_siteName = "TRANQUILITY BASE";
    m_worldWidth = 14400.0f * FT2M; // [m] 4389.12 m (14400.0 ft)
    m_worldHeight = 2800.0f * FT2M; // [m] 853.44 m (2800.0 ft)

    m_vertices = {
        {0.0f * FT2M, 1600.0f * FT2M},     {400.0f * FT2M, 1550.0f * FT2M},
        {800.0f * FT2M, 1680.0f * FT2M},    {1200.0f * FT2M, 1720.0f * FT2M},
        {1600.0f * FT2M, 1680.0f * FT2M},   {2000.0f * FT2M, 1620.0f * FT2M},
        {2500.0f * FT2M, 1420.0f * FT2M},   {3000.0f * FT2M, 1480.0f * FT2M},
        {3500.0f * FT2M, 1530.0f * FT2M},   {4000.0f * FT2M, 1650.0f * FT2M},
        {4600.0f * FT2M, 1780.0f * FT2M},   {5200.0f * FT2M, 1850.0f * FT2M},
        {5800.0f * FT2M, 1920.0f * FT2M},   {6400.0f * FT2M, 1880.0f * FT2M},
        {7000.0f * FT2M, 1760.0f * FT2M},   {7600.0f * FT2M, 1600.0f * FT2M},
        {8200.0f * FT2M, 1440.0f * FT2M},   {8800.0f * FT2M, 1380.0f * FT2M},
        {9400.0f * FT2M, 1450.0f * FT2M},   {10000.0f * FT2M, 1560.0f * FT2M},
        {10700.0f * FT2M, 1680.0f * FT2M},  {11400.0f * FT2M, 1760.0f * FT2M},
        {12100.0f * FT2M, 1720.0f * FT2M},  {12800.0f * FT2M, 1640.0f * FT2M},
        {13600.0f * FT2M, 1580.0f * FT2M},  {14400.0f * FT2M, 1650.0f * FT2M}
    };
    m_pads.clear();
}

void Terrain::generateTychoCrater() {
    m_siteName = "TYCHO CRATER";
    m_worldWidth = 14400.0f * FT2M; // [m] 4389.12 m
    m_worldHeight = 2800.0f * FT2M; // [m] 853.44 m

    m_vertices = {
        {0.0f * FT2M, 1400.0f * FT2M},     {600.0f * FT2M, 1320.0f * FT2M},
        {1200.0f * FT2M, 1200.0f * FT2M},   {1800.0f * FT2M, 1100.0f * FT2M},
        {2400.0f * FT2M, 1120.0f * FT2M},   {3000.0f * FT2M, 1350.0f * FT2M},
        {3800.0f * FT2M, 1680.0f * FT2M},   {4600.0f * FT2M, 1980.0f * FT2M},
        {5400.0f * FT2M, 2150.0f * FT2M},   {6000.0f * FT2M, 2180.0f * FT2M},
        {6600.0f * FT2M, 2020.0f * FT2M},   {7200.0f * FT2M, 1850.0f * FT2M}, // Central peak
        {7800.0f * FT2M, 2040.0f * FT2M},   {8400.0f * FT2M, 2180.0f * FT2M},
        {9200.0f * FT2M, 2150.0f * FT2M},   {10000.0f * FT2M, 1850.0f * FT2M},
        {10800.0f * FT2M, 1550.0f * FT2M},  {11600.0f * FT2M, 1280.0f * FT2M},
        {12400.0f * FT2M, 1180.0f * FT2M},  {13200.0f * FT2M, 1280.0f * FT2M},
        {13800.0f * FT2M, 1380.0f * FT2M},  {14400.0f * FT2M, 1450.0f * FT2M}
    };
    m_pads.clear();
}

void Terrain::generateOceanOfStorms() {
    m_siteName = "OCEAN OF STORMS";
    m_worldWidth = 14400.0f * FT2M; // [m] 4389.12 m
    m_worldHeight = 2800.0f * FT2M; // [m] 853.44 m

    m_vertices = {
        {0.0f * FT2M, 1680.0f * FT2M},     {600.0f * FT2M, 1720.0f * FT2M},
        {1300.0f * FT2M, 1660.0f * FT2M},   {2000.0f * FT2M, 1520.0f * FT2M},
        {2800.0f * FT2M, 1380.0f * FT2M},   {3500.0f * FT2M, 1440.0f * FT2M},
        {4200.0f * FT2M, 1580.0f * FT2M},   {4900.0f * FT2M, 1660.0f * FT2M},
        {5600.0f * FT2M, 1740.0f * FT2M},   {6300.0f * FT2M, 1890.0f * FT2M},
        {7000.0f * FT2M, 1940.0f * FT2M},   {7700.0f * FT2M, 1820.0f * FT2M},
        {8400.0f * FT2M, 1650.0f * FT2M},   {9100.0f * FT2M, 1480.0f * FT2M},
        {9800.0f * FT2M, 1460.0f * FT2M},   {10600.0f * FT2M, 1580.0f * FT2M},
        {11500.0f * FT2M, 1720.0f * FT2M},  {12400.0f * FT2M, 1780.0f * FT2M},
        {13400.0f * FT2M, 1680.0f * FT2M},  {14400.0f * FT2M, 1680.0f * FT2M}
    };
    m_pads.clear();
}

void Terrain::generateHadleyRille() {
    m_siteName = "HADLEY RILLE";
    m_worldWidth = 14400.0f * FT2M; // [m] 4389.12 m
    m_worldHeight = 2800.0f * FT2M; // [m] 853.44 m

    m_vertices = {
        {0.0f * FT2M, 1500.0f * FT2M},     {700.0f * FT2M, 1400.0f * FT2M},
        {1400.0f * FT2M, 1340.0f * FT2M},   {2100.0f * FT2M, 1350.0f * FT2M},
        {2900.0f * FT2M, 1560.0f * FT2M},   {3700.0f * FT2M, 1820.0f * FT2M},
        {4500.0f * FT2M, 2120.0f * FT2M},   {5300.0f * FT2M, 2320.0f * FT2M},
        {6100.0f * FT2M, 2440.0f * FT2M},   {6900.0f * FT2M, 2450.0f * FT2M}, // Deep rille
        {7600.0f * FT2M, 2320.0f * FT2M},   {8400.0f * FT2M, 2040.0f * FT2M},
        {9200.0f * FT2M, 1720.0f * FT2M},   {10000.0f * FT2M, 1480.0f * FT2M},
        {10800.0f * FT2M, 1520.0f * FT2M},  {11600.0f * FT2M, 1680.0f * FT2M},
        {12400.0f * FT2M, 1820.0f * FT2M},  {13300.0f * FT2M, 1740.0f * FT2M},
        {14400.0f * FT2M, 1600.0f * FT2M}
    };
    m_pads.clear();
}

void Terrain::buildSegments() {
    m_segments.clear();
    for (size_t i = 1; i < m_vertices.size(); ++i) {
        TerrainSegment seg;
        seg.x1 = m_vertices[i - 1].first;
        seg.y1 = m_vertices[i - 1].second;
        seg.x2 = m_vertices[i].first;
        seg.y2 = m_vertices[i].second;
        seg.isLandingPad = false;
        seg.multiplier = 0;
        m_segments.push_back(seg);
    }
}

float Terrain::getCenterlineHeightAt(float x) const {
    if (m_vertices.empty()) return m_worldHeight;
    if (x <= m_vertices.front().first) return m_vertices.front().second;
    if (x >= m_vertices.back().first) return m_vertices.back().second;

    for (size_t i = 1; i < m_vertices.size(); ++i) {
        if (x >= m_vertices[i - 1].first && x <= m_vertices[i].first) {
            float t = (x - m_vertices[i - 1].first) / (m_vertices[i].first - m_vertices[i - 1].first);
            return m_vertices[i - 1].second + t * (m_vertices[i].second - m_vertices[i - 1].second);
        }
    }
    return m_worldHeight;
}

bool Terrain::isPointOnPad(float /*x*/, float /*z*/, int& outMultiplier, float& outPadY) const {
    outMultiplier = 0;
    outPadY = 0.0f;
    return false;
}

float Terrain::getSurfaceHeightAt(float x, float z) const {
    float baseY = getCenterlineHeightAt(x);
    if (std::abs(z) < 1e-3f) {
        return baseY;
    }

    float padMask = 1.0f;

    // 3D lunar topography away from Z=0 centerline (remember -Y is upward elevation!)
    float absZ = std::abs(z);
    float normZ = std::clamp(absZ / (m_worldDepth * 0.5f), 0.0f, 1.0f);

    float ridgeWave = std::sin(x * 0.011f + z * 0.017f) * std::cos(x * 0.006f - z * 0.013f);
    float craterWave = std::sin(x * 0.023f) * std::sin(z * 0.025f);

    float deltaY = 0.0f;
    switch (m_currentSite) {
        case MissionSite::TRANQUILITY_BASE: {
            // Gently rolling mare plains rising into north/south highland crater rims
            float canyonRise = -75.0f * FT2M * (normZ * normZ);
            float hills = -32.0f * FT2M * normZ * (0.5f * ridgeWave + 0.5f * craterWave);
            deltaY = canyonRise + hills;
            break;
        }
        case MissionSite::TYCHO_CRATER: {
            // 3D Bowl impact crater with rising north/south terraced walls and central 3D peak
            float centerDistX = (x - 2510.0f * FT2M) / (1600.0f * FT2M);
            float bowlWall = -165.0f * FT2M * (normZ * normZ) * (1.0f - 0.35f * std::min(1.0f, centerDistX * centerDistX));
            float terraces = -28.0f * FT2M * std::sin(normZ * 6.28318f) * normZ;
            deltaY = bowlWall + terraces + (-22.0f * FT2M * normZ * ridgeWave);
            break;
        }
        case MissionSite::OCEAN_OF_STORMS: {
            // Basalt plains with sinuous wrinkle ridges and crater rims rising toward North/South horizons
            float ridge = -85.0f * FT2M * (normZ * normZ) * (0.75f + 0.25f * ridgeWave);
            float craterRim = -35.0f * FT2M * normZ * (0.5f + 0.5f * std::abs(craterWave));
            deltaY = ridge + craterRim;
            break;
        }
        case MissionSite::HADLEY_RILLE: {
            // Steep winding 3D lunar canyon walls flanking the central rille floor
            float steepWall = -210.0f * FT2M * std::pow(normZ, 1.5f);
            float rilleMeander = -40.0f * FT2M * normZ * std::sin(x * 0.008f + (z > 0.0f ? 0.6f : -0.6f));
            deltaY = steepWall + rilleMeander;
            break;
        }
        default:
            break;
    }

    // Smoothly blend from 0 at Z=0 so the centerline is C1 continuous
    float centerBlend = std::clamp(absZ / (65.0f * FT2M), 0.0f, 1.0f);
    centerBlend = centerBlend * centerBlend * (3.0f - 2.0f * centerBlend);

    return baseY + deltaY * centerBlend * padMask;
}

Vec3 Terrain::getSurfaceNormalAt(float x, float z) const {
    const float eps = 0.6f; // [m] Central difference step
    float dhdx = (getSurfaceHeightAt(x + eps, z) - getSurfaceHeightAt(x - eps, z)) / (2.0f * eps);
    float dhdz = (getSurfaceHeightAt(x, z + eps) - getSurfaceHeightAt(x, z - eps)) / (2.0f * eps);
    // Since +Y is down and sky is -Y, outward normal pointing into sky is (dh/dx, -1, dh/dz)
    Vec3 n{dhdx, -1.0f, dhdz};
    return n.normalized();
}

bool Terrain::checkCollision(float px1, float py1, float px2, float py2,
                             bool& outIsPad, int& outPadMultiplier, float& outHitY) const {
    outIsPad = false;
    outPadMultiplier = 0;
    outHitY = 0.0f;

    for (const auto& seg : m_segments) {
        float ix, iy;
        if (lineSegmentsIntersect(px1, py1, px2, py2, seg.x1, seg.y1, seg.x2, seg.y2, ix, iy)) {
            outHitY = iy;
            if (seg.isLandingPad) {
                outIsPad = true;
                outPadMultiplier = seg.multiplier;
            }
            return true;
        }
    }
    return false;
}

bool Terrain::checkCollision3D(const Vec3& p1, const Vec3& p2,
                               bool& outIsPad, int& outPadMultiplier, float& outHitY,
                               Vec3* outNormal) const {
    // If both endpoints are on the Z=0 plane, also check exact 2D segments first for exact compatibility
    if (std::abs(p1.z) < 1e-3f && std::abs(p2.z) < 1e-3f) {
        if (checkCollision(p1.x, p1.y, p2.x, p2.y, outIsPad, outPadMultiplier, outHitY)) {
            if (outNormal) {
                *outNormal = getSurfaceNormalAt(0.5f * (p1.x + p2.x), 0.0f);
            }
            return true;
        }
    }

    outIsPad = false;
    outPadMultiplier = 0;
    outHitY = 0.0f;

    // Sample along 3D segment [p1, p2] against heightfield y = h(x, z)
    const int samples = 8;
    for (int i = 0; i <= samples; ++i) {
        float t = (float)i / (float)samples;
        Vec3 pt = p1 + (p2 - p1) * t;
        float surfY = getSurfaceHeightAt(pt.x, pt.z);
        if (pt.y >= surfY) {
            outHitY = surfY;
            int mult = 0;
            float padY = 0.0f;
            if (isPointOnPad(pt.x, pt.z, mult, padY)) {
                outIsPad = true;
                outPadMultiplier = mult;
                outHitY = padY;
            }
            if (outNormal) {
                *outNormal = getSurfaceNormalAt(pt.x, pt.z);
            }
            return true;
        }
    }
    return false;
}

bool Terrain::castRay(float ox, float oy, float dx, float dy, float maxDist, RayHit& outHit) const {
    // Treat the ray as a line segment from origin to (origin + dir * maxDist)
    float rx2 = ox + dx * maxDist;
    float ry2 = oy + dy * maxDist;

    float bestDist = maxDist + 1.0f;
    bool anyHit = false;

    for (const auto& seg : m_segments) {
        float ix, iy;
        if (lineSegmentsIntersect(ox, oy, rx2, ry2,
                                  seg.x1, seg.y1, seg.x2, seg.y2, ix, iy)) {
            float hitDist = std::sqrt((ix - ox) * (ix - ox) + (iy - oy) * (iy - oy));
            if (hitDist < bestDist) {
                bestDist = hitDist;
                anyHit = true;

                // Compute segment normal (perpendicular to segment)
                float segDx = seg.x2 - seg.x1;
                float segDy = seg.y2 - seg.y1;
                float segLen = std::sqrt(segDx * segDx + segDy * segDy);
                if (segLen > 1e-4f) {
                    float nx = -segDy / segLen;
                    float ny =  segDx / segLen;
                    float toOx = ox - ix;
                    float toOy = oy - iy;
                    if (nx * toOx + ny * toOy < 0.0f) {
                        nx = -nx;
                        ny = -ny;
                    }
                    outHit.normalX = nx;
                    outHit.normalY = ny;
                } else {
                    outHit.normalX = 0.0f;
                    outHit.normalY = -1.0f;
                }
                outHit.dist = bestDist;
            }
        }
    }
    return anyHit;
}

bool Terrain::castRay3D(const Vec3& origin, const Vec3& dir, float maxDist, RayHit3D& outHit) const {
    // Fast path for pure Z=0 planar rays so 2D ray behavior is exact
    if (std::abs(origin.z) < 1e-4f && std::abs(dir.z) < 1e-4f) {
        RayHit hit2d;
        if (castRay(origin.x, origin.y, dir.x, dir.y, maxDist, hit2d)) {
            outHit.dist = hit2d.dist;
            outHit.point = origin + dir * hit2d.dist;
            outHit.normal = {hit2d.normalX, hit2d.normalY, 0.0f};
            int mult = 0;
            float padY = 0.0f;
            outHit.isPad = isPointOnPad(outHit.point.x, 0.0f, mult, padY);
            outHit.padMultiplier = mult;
            return true;
        }
        return false;
    }

    // 3D heightfield ray marching with binary bisection refinement
    const int numSteps = std::clamp((int)(maxDist / 2.2f), 8, 64);
    float step = maxDist / (float)numSteps;
    float prevT = 0.0f;

    for (int i = 1; i <= numSteps; ++i) {
        float currT = (float)i * step;
        Vec3 pt = origin + dir * currT;
        float surfY = getSurfaceHeightAt(pt.x, pt.z);
        if (pt.y >= surfY) {
            // Refine intersection in [prevT, currT] via 6 bisection iterations
            float lo = prevT;
            float hi = currT;
            for (int b = 0; b < 6; ++b) {
                float mid = 0.5f * (lo + hi);
                Vec3 mp = origin + dir * mid;
                if (mp.y >= getSurfaceHeightAt(mp.x, mp.z)) {
                    hi = mid;
                } else {
                    lo = mid;
                }
            }
            float hitT = 0.5f * (lo + hi);
            Vec3 hitPt = origin + dir * hitT;
            outHit.dist = hitT;
            outHit.point = hitPt;
            outHit.normal = getSurfaceNormalAt(hitPt.x, hitPt.z);
            float padY = 0.0f;
            outHit.isPad = isPointOnPad(hitPt.x, hitPt.z, outHit.padMultiplier, padY);
            return true;
        }
        prevT = currT;
    }

    return false;
}

void Terrain::render(SDL_Renderer* renderer, const Camera2D& cam, float glow) {
    SDL_Color terrainColor = {210, 240, 210, 255}; // Classic phosphor greenish-white
    SDL_Color gridSubColor = {95, 165, 120, 175};  // 3D depth contour lattice green
    SDL_Color dotColor = {120, 215, 160, 180};      // Little dots between wireframe lines

    if (cam.use3D) {
        // 1. Render 3D Wireframe Lunar Surface Lattice around the camera
        const float halfDepth = m_worldDepth * 0.5f;
        const int numZSlices = 21; // Z slices from -halfDepth to +halfDepth
        const int numXCols = 96;   // X samples across world width

        float zStep = m_worldDepth / (float)(numZSlices - 1);
        float xStep = m_worldWidth / (float)(numXCols - 1);

        // Draw longitudinal X-ridge lines for each Z slice
        for (int iz = 0; iz < numZSlices; ++iz) {
            float z = -halfDepth + iz * zStep;
            bool isCenterSlice = (std::abs(z) < 1.0f);
            SDL_Color sliceCol = isCenterSlice ? terrainColor : gridSubColor;
            int thick = isCenterSlice ? 2 : 1;
            float sliceGlow = isCenterSlice ? glow * 1.15f : glow * 0.75f;

            for (int ix = 1; ix < numXCols; ++ix) {
                float x1 = (ix - 1) * xStep;
                float x2 = ix * xStep;
                float y1 = getSurfaceHeightAt(x1, z);
                float y2 = getSurfaceHeightAt(x2, z);
                VectorRenderer::drawLineWorld3D(renderer, cam, x1, y1, z, x2, y2, z, sliceCol, sliceGlow, thick);
            }
        }

        // Draw transverse Z-rib lines connecting Z slices
        for (int ix = 0; ix < numXCols; ix += 2) {
            float x = ix * xStep;
            for (int iz = 1; iz < numZSlices; ++iz) {
                float z1 = -halfDepth + (iz - 1) * zStep;
                float z2 = -halfDepth + iz * zStep;
                float y1 = getSurfaceHeightAt(x, z1);
                float y2 = getSurfaceHeightAt(x, z2);
                VectorRenderer::drawLineWorld3D(renderer, cam, x, y1, z1, x, y2, z2, gridSubColor, glow * 0.65f, 1);
            }
        }

        // 2. Add little dots between the wire frame terrain (centered inside each lattice cell)
        for (int iz = 0; iz < numZSlices - 1; ++iz) {
            float midZ = -halfDepth + (iz + 0.5f) * zStep;
            for (int ix = 0; ix < numXCols - 1; ++ix) {
                float midX = (ix + 0.5f) * xStep;
                float midY = getSurfaceHeightAt(midX, midZ);
                VectorRenderer::drawPointWorld3D(renderer, cam, midX, midY, midZ, dotColor, glow * 0.75f, 1);
            }
        }
    }

    // 3. Draw primary centerline ridge segments (ensures crisp vertices along Z=0)
    for (const auto& seg : m_segments) {
        VectorRenderer::drawLineWorld3D(renderer, cam, seg.x1, seg.y1, 0.0f, seg.x2, seg.y2, 0.0f,
                                        terrainColor, glow, 1);
    }
}
