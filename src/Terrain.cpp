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
    m_worldWidth = 4800.0f * FT2M;  // [m] 1463.04 m (converted from 4800.0 ft)
    m_worldHeight = 2200.0f * FT2M; // [m] 670.56 m (converted from 2200.0 ft)

    // Base ground level around y = 1700 ft (518.16 m)
    m_vertices = {
        {0.0f * FT2M, 1600.0f * FT2M}, {300.0f * FT2M, 1550.0f * FT2M}, {500.0f * FT2M, 1680.0f * FT2M}, {700.0f * FT2M, 1720.0f * FT2M},
        // 2X Pad (Wide, safe, x: 243.84 m to 350.52 m)
        {800.0f * FT2M, 1720.0f * FT2M}, {1150.0f * FT2M, 1720.0f * FT2M},
        {1250.0f * FT2M, 1620.0f * FT2M}, {1450.0f * FT2M, 1420.0f * FT2M}, {1600.0f * FT2M, 1480.0f * FT2M},
        // 3X Pad (Medium, x: 518.16 m to 594.36 m)
        {1700.0f * FT2M, 1530.0f * FT2M}, {1950.0f * FT2M, 1530.0f * FT2M},
        {2100.0f * FT2M, 1680.0f * FT2M}, {2300.0f * FT2M, 1820.0f * FT2M}, {2450.0f * FT2M, 1920.0f * FT2M},
        // 5X Pad (Very narrow in deep chasm, x: 762.00 m to 798.58 m)
        {2500.0f * FT2M, 1920.0f * FT2M}, {2620.0f * FT2M, 1920.0f * FT2M},
        {2750.0f * FT2M, 1780.0f * FT2M}, {2900.0f * FT2M, 1650.0f * FT2M}, {3100.0f * FT2M, 1450.0f * FT2M}, {3300.0f * FT2M, 1380.0f * FT2M},
        // 4X Pad (Narrow peak ledge, x: 1030.22 m to 1075.94 m)
        {3380.0f * FT2M, 1380.0f * FT2M}, {3530.0f * FT2M, 1380.0f * FT2M},
        {3700.0f * FT2M, 1520.0f * FT2M}, {3950.0f * FT2M, 1680.0f * FT2M}, {4200.0f * FT2M, 1720.0f * FT2M},
        // 2X Pad (Far eastern plain, x: 1310.64 m to 1402.08 m)
        {4300.0f * FT2M, 1720.0f * FT2M}, {4600.0f * FT2M, 1720.0f * FT2M},
        {4800.0f * FT2M, 1650.0f * FT2M}
    };

    m_pads = {
        {800.0f * FT2M, 1150.0f * FT2M, 1720.0f * FT2M, 2, "2X"}, // [m] 243.84 m - 350.52 m, alt 524.26 m
        {1700.0f * FT2M, 1950.0f * FT2M, 1530.0f * FT2M, 3, "3X"},
        {2500.0f * FT2M, 2620.0f * FT2M, 1920.0f * FT2M, 5, "5X"},
        {3380.0f * FT2M, 3530.0f * FT2M, 1380.0f * FT2M, 4, "4X"},
        {4300.0f * FT2M, 4600.0f * FT2M, 1720.0f * FT2M, 2, "2X"}
    };
}

void Terrain::generateTychoCrater() {
    m_siteName = "TYCHO CRATER";
    m_worldWidth = 4800.0f * FT2M;  // [m] 1463.04 m (converted from 4800.0 ft)
    m_worldHeight = 2400.0f * FT2M; // [m] 731.52 m (converted from 2400.0 ft)

    m_vertices = {
        {0.0f * FT2M, 1400.0f * FT2M}, {350.0f * FT2M, 1300.0f * FT2M}, {600.0f * FT2M, 1200.0f * FT2M}, {850.0f * FT2M, 1120.0f * FT2M},
        // 4X Rim ledge
        {920.0f * FT2M, 1120.0f * FT2M}, {1080.0f * FT2M, 1120.0f * FT2M},
        {1200.0f * FT2M, 1350.0f * FT2M}, {1450.0f * FT2M, 1650.0f * FT2M}, {1700.0f * FT2M, 1980.0f * FT2M}, {1900.0f * FT2M, 2150.0f * FT2M},
        // 3X Crater floor West
        {2000.0f * FT2M, 2150.0f * FT2M}, {2250.0f * FT2M, 2150.0f * FT2M},
        {2350.0f * FT2M, 2020.0f * FT2M}, {2420.0f * FT2M, 1900.0f * FT2M},
        // 5X Central peak pinnacle
        {2460.0f * FT2M, 1900.0f * FT2M}, {2560.0f * FT2M, 1900.0f * FT2M},
        {2600.0f * FT2M, 2020.0f * FT2M}, {2700.0f * FT2M, 2150.0f * FT2M},
        // 2X Crater floor East
        {2800.0f * FT2M, 2150.0f * FT2M}, {3150.0f * FT2M, 2150.0f * FT2M},
        {3400.0f * FT2M, 1850.0f * FT2M}, {3650.0f * FT2M, 1550.0f * FT2M}, {3900.0f * FT2M, 1300.0f * FT2M}, {4100.0f * FT2M, 1220.0f * FT2M},
        // 3X High eastern ledge
        {4180.0f * FT2M, 1220.0f * FT2M}, {4400.0f * FT2M, 1220.0f * FT2M},
        {4600.0f * FT2M, 1380.0f * FT2M}, {4800.0f * FT2M, 1450.0f * FT2M}
    };

    m_pads = {
        {920.0f * FT2M, 1080.0f * FT2M, 1120.0f * FT2M, 4, "4X"}, // [m]
        {2000.0f * FT2M, 2250.0f * FT2M, 2150.0f * FT2M, 3, "3X"},
        {2460.0f * FT2M, 2560.0f * FT2M, 1900.0f * FT2M, 5, "5X"},
        {2800.0f * FT2M, 3150.0f * FT2M, 2150.0f * FT2M, 2, "2X"},
        {4180.0f * FT2M, 4400.0f * FT2M, 1220.0f * FT2M, 3, "3X"}
    };
}

void Terrain::generateOceanOfStorms() {
    m_siteName = "OCEAN OF STORMS";
    m_worldWidth = 4800.0f * FT2M;  // [m] 1463.04 m (converted from 4800.0 ft)
    m_worldHeight = 2200.0f * FT2M; // [m] 670.56 m (converted from 2200.0 ft)

    m_vertices = {
        {0.0f * FT2M, 1680.0f * FT2M}, {250.0f * FT2M, 1720.0f * FT2M}, {450.0f * FT2M, 1680.0f * FT2M},
        // 2X Pad
        {500.0f * FT2M, 1680.0f * FT2M}, {800.0f * FT2M, 1680.0f * FT2M},
        {950.0f * FT2M, 1520.0f * FT2M}, {1150.0f * FT2M, 1380.0f * FT2M}, {1300.0f * FT2M, 1420.0f * FT2M}, {1500.0f * FT2M, 1580.0f * FT2M},
        // 3X Pad
        {1600.0f * FT2M, 1620.0f * FT2M}, {1820.0f * FT2M, 1620.0f * FT2M},
        {1950.0f * FT2M, 1750.0f * FT2M}, {2100.0f * FT2M, 1920.0f * FT2M}, {2250.0f * FT2M, 1920.0f * FT2M},
        // 5X Ravine bottom
        {2290.0f * FT2M, 1920.0f * FT2M}, {2390.0f * FT2M, 1920.0f * FT2M},
        {2500.0f * FT2M, 1750.0f * FT2M}, {2700.0f * FT2M, 1550.0f * FT2M}, {2900.0f * FT2M, 1460.0f * FT2M},
        // 4X Pad
        {2980.0f * FT2M, 1460.0f * FT2M}, {3140.0f * FT2M, 1460.0f * FT2M},
        {3300.0f * FT2M, 1600.0f * FT2M}, {3550.0f * FT2M, 1750.0f * FT2M}, {3750.0f * FT2M, 1780.0f * FT2M},
        // 3X Pad
        {3850.0f * FT2M, 1780.0f * FT2M}, {4080.0f * FT2M, 1780.0f * FT2M},
        {4250.0f * FT2M, 1700.0f * FT2M}, {4550.0f * FT2M, 1620.0f * FT2M}, {4800.0f * FT2M, 1680.0f * FT2M}
    };

    m_pads = {
        {500.0f * FT2M, 800.0f * FT2M, 1680.0f * FT2M, 2, "2X"}, // [m]
        {1600.0f * FT2M, 1820.0f * FT2M, 1620.0f * FT2M, 3, "3X"},
        {2290.0f * FT2M, 2390.0f * FT2M, 1920.0f * FT2M, 5, "5X"},
        {2980.0f * FT2M, 3140.0f * FT2M, 1460.0f * FT2M, 4, "4X"},
        {3850.0f * FT2M, 4080.0f * FT2M, 1780.0f * FT2M, 3, "3X"}
    };
}

void Terrain::generateHadleyRille() {
    m_siteName = "HADLEY RILLE";
    m_worldWidth = 4800.0f * FT2M;  // [m] 1463.04 m (converted from 4800.0 ft)
    m_worldHeight = 2500.0f * FT2M; // [m] 762.00 m (converted from 2500.0 ft)

    m_vertices = {
        {0.0f * FT2M, 1500.0f * FT2M}, {300.0f * FT2M, 1420.0f * FT2M}, {550.0f * FT2M, 1350.0f * FT2M},
        // 3X Plateau
        {650.0f * FT2M, 1350.0f * FT2M}, {900.0f * FT2M, 1350.0f * FT2M},
        {1100.0f * FT2M, 1550.0f * FT2M}, {1350.0f * FT2M, 1800.0f * FT2M}, {1600.0f * FT2M, 2100.0f * FT2M},
        // 4X Step
        {1700.0f * FT2M, 2100.0f * FT2M}, {1880.0f * FT2M, 2100.0f * FT2M},
        {2000.0f * FT2M, 2320.0f * FT2M}, {2200.0f * FT2M, 2400.0f * FT2M},
        // 5X Deep Rille canyon
        {2280.0f * FT2M, 2400.0f * FT2M}, {2380.0f * FT2M, 2400.0f * FT2M},
        {2500.0f * FT2M, 2320.0f * FT2M}, {2700.0f * FT2M, 2050.0f * FT2M}, {2950.0f * FT2M, 1750.0f * FT2M}, {3200.0f * FT2M, 1500.0f * FT2M},
        // 5X High Pinnacle
        {3280.0f * FT2M, 1500.0f * FT2M}, {3380.0f * FT2M, 1500.0f * FT2M},
        {3550.0f * FT2M, 1680.0f * FT2M}, {3800.0f * FT2M, 1820.0f * FT2M},
        // 2X Broad shelf
        {3900.0f * FT2M, 1820.0f * FT2M}, {4350.0f * FT2M, 1820.0f * FT2M},
        {4550.0f * FT2M, 1700.0f * FT2M}, {4800.0f * FT2M, 1600.0f * FT2M}
    };

    m_pads = {
        {650.0f * FT2M, 900.0f * FT2M, 1350.0f * FT2M, 3, "3X"}, // [m]
        {1700.0f * FT2M, 1880.0f * FT2M, 2100.0f * FT2M, 4, "4X"},
        {2280.0f * FT2M, 2380.0f * FT2M, 2400.0f * FT2M, 5, "5X"},
        {3280.0f * FT2M, 3380.0f * FT2M, 1500.0f * FT2M, 5, "5X"},
        {3900.0f * FT2M, 4350.0f * FT2M, 1820.0f * FT2M, 2, "2X"}
    };
}

void Terrain::buildSegments() {
    m_segments.clear();
    for (size_t i = 1; i < m_vertices.size(); ++i) {
        TerrainSegment seg;
        seg.x1 = m_vertices[i - 1].first;  // [m]
        seg.y1 = m_vertices[i - 1].second; // [m]
        seg.x2 = m_vertices[i].first;      // [m]
        seg.y2 = m_vertices[i].second;     // [m]

        // Check if this segment corresponds to any landing pad
        seg.isLandingPad = false;
        seg.multiplier = 0;
        const float tolerance = 0.35f; // [m]
        for (const auto& pad : m_pads) {
            if (std::abs(seg.y1 - pad.y) < tolerance && std::abs(seg.y2 - pad.y) < tolerance &&
                seg.x1 >= pad.x1 - tolerance && seg.x2 <= pad.x2 + tolerance) {
                seg.isLandingPad = true;
                seg.multiplier = pad.multiplier;
                break;
            }
        }
        m_segments.push_back(seg);
    }
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

float Terrain::getSurfaceHeightAt(float x) const {
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
                    // One of two perpendiculars: (-segDy, segDx) / len
                    float nx = -segDy / segLen;
                    float ny =  segDx / segLen;
                    // Ensure the normal points toward the ray origin (away from terrain face)
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
                    outHit.normalY = -1.0f; // degenerate segment: point straight up
                }
                outHit.dist = bestDist;
            }
        }
    }
    return anyHit;
}

void Terrain::render(SDL_Renderer* renderer, const Camera2D& cam, float glow) {

    // 1. Draw rugged terrain segments
    SDL_Color terrainColor = {210, 240, 210, 255}; // Classic phosphor greenish-white
    SDL_Color padColor = {255, 255, 100, 255};     // Vibrant glowing yellow for pads

    for (const auto& seg : m_segments) {
        if (seg.isLandingPad) {
            // Draw landing pad with double line and intense vector brightness
            VectorRenderer::drawLineWorld(renderer, cam, seg.x1, seg.y1, seg.x2, seg.y2, 
                                          padColor, glow * 1.5f, 2);
            VectorRenderer::drawLineWorld(renderer, cam, seg.x1, seg.y1 + 4.0f * FT2M, seg.x2, seg.y2 + 4.0f * FT2M, 
                                          padColor, glow, 1);
            // End caps / corner ticks [m]
            VectorRenderer::drawLineWorld(renderer, cam, seg.x1, seg.y1 - 6.0f * FT2M, seg.x1, seg.y1 + 6.0f * FT2M, padColor, glow, 2);
            VectorRenderer::drawLineWorld(renderer, cam, seg.x2, seg.y2 - 6.0f * FT2M, seg.x2, seg.y2 + 6.0f * FT2M, padColor, glow, 2);
        } else {
            VectorRenderer::drawLineWorld(renderer, cam, seg.x1, seg.y1, seg.x2, seg.y2, 
                                          terrainColor, glow, 1);
        }
    }

    // 2. Render Multiplier text under each pad
    for (const auto& pad : m_pads) {
        float centerX = (pad.x1 + pad.x2) * 0.5f; // [m]
        float centerY = pad.y + 18.0f * FT2M;     // [m] slightly below pad surface (5.49 m)

        float sx, sy; // [px]
        cam.worldToScreen(centerX, centerY, sx, sy);

        // Scale font proportional to camera zoom, with reasonable limits
        float fontScale = std::clamp((cam.zoom * FT2M) * 2.2f, 1.2f, 3.8f);
        VectorFont::drawTextCentered(renderer, pad.label, sx, sy, fontScale, padColor, glow);
    }
}
