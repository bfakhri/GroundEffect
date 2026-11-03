#include "VectorRenderer.hpp"
#include <algorithm>

void VectorRenderer::drawLineScreen(SDL_Renderer* renderer, float x1, float y1, float x2, float y2,
                                    SDL_Color color, float glow, int thickness) {
    // Guard against extreme screen coordinates after steep perspective projection
    if (std::max(x1, x2) < -2000.0f || std::min(x1, x2) > 6000.0f ||
        std::max(y1, y2) < -2000.0f || std::min(y1, y2) > 6000.0f) {
        return;
    }

    // Phosphor glow halo
    if (glow > 0.05f) {
        Uint8 glowAlpha = (Uint8)std::clamp((int)(color.a * 0.32f * glow), 0, 255);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, glowAlpha);

        int offset = (thickness > 1) ? 2 : 1;
        SDL_RenderDrawLine(renderer, (int)(x1 - offset), (int)y1, (int)(x2 - offset), (int)y2);
        SDL_RenderDrawLine(renderer, (int)(x1 + offset), (int)y1, (int)(x2 + offset), (int)y2);
        SDL_RenderDrawLine(renderer, (int)x1, (int)(y1 - offset), (int)x2, (int)(y2 - offset));
        SDL_RenderDrawLine(renderer, (int)x1, (int)(y1 + offset), (int)x2, (int)(y2 + offset));
    }

    // Core vector stroke
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(renderer, (int)x1, (int)y1, (int)x2, (int)y2);
    if (thickness > 1) {
        SDL_RenderDrawLine(renderer, (int)(x1 + 1), (int)y1, (int)(x2 + 1), (int)y2);
        SDL_RenderDrawLine(renderer, (int)x1, (int)(y1 + 1), (int)x2, (int)(y2 + 1));
    }
}

void VectorRenderer::drawLineWorld(SDL_Renderer* renderer, const Camera2D& cam,
                                   float wx1, float wy1, float wx2, float wy2,
                                   SDL_Color color, float glow, int thickness) {
    if (cam.use3D) {
        drawLineWorld3D(renderer, cam, Vec3{wx1, wy1, 0.0f}, Vec3{wx2, wy2, 0.0f}, color, glow, thickness);
        return;
    }
    float sx1, sy1, sx2, sy2;
    cam.worldToScreen(wx1, wy1, sx1, sy1);
    cam.worldToScreen(wx2, wy2, sx2, sy2);

    drawLineScreen(renderer, sx1, sy1, sx2, sy2, color, glow, thickness);
}

void VectorRenderer::drawLineWorld3D(SDL_Renderer* renderer, const Camera2D& cam,
                                     const Vec3& p1, const Vec3& p2,
                                     SDL_Color color, float glow, int thickness) {
    if (!cam.use3D) {
        float sx1 = (p1.x - cam.x) * cam.zoom + cam.screenWidth * 0.5f;
        float sy1 = (p1.y - cam.y) * cam.zoom + cam.screenHeight * 0.5f;
        float sx2 = (p2.x - cam.x) * cam.zoom + cam.screenWidth * 0.5f;
        float sy2 = (p2.y - cam.y) * cam.zoom + cam.screenHeight * 0.5f;
        drawLineScreen(renderer, sx1, sy1, sx2, sy2, color, glow, thickness);
        return;
    }

    Vec3 c1 = cam.worldToCameraSpace(p1);
    Vec3 c2 = cam.worldToCameraSpace(p2);

    // Near-plane clipping in 3D camera space
    const float nearZ = cam.nearPlane;
    if (c1.z < nearZ && c2.z < nearZ) {
        return;
    }
    if (c1.z < nearZ) {
        float t = (nearZ - c1.z) / (c2.z - c1.z);
        c1.x = c1.x + t * (c2.x - c1.x);
        c1.y = c1.y + t * (c2.y - c1.y);
        c1.z = nearZ;
    } else if (c2.z < nearZ) {
        float t = (nearZ - c2.z) / (c1.z - c2.z);
        c2.x = c2.x + t * (c1.x - c2.x);
        c2.y = c2.y + t * (c1.y - c2.y);
        c2.z = nearZ;
    }

    float f = cam.getFocalLengthPx();
    float halfW = cam.screenWidth * 0.5f;
    float halfH = cam.screenHeight * 0.5f;

    float sx1 = halfW + (c1.x / c1.z) * f;
    float sy1 = halfH + (c1.y / c1.z) * f;
    float sx2 = halfW + (c2.x / c2.z) * f;
    float sy2 = halfH + (c2.y / c2.z) * f;

    // Subtle depth-based phosphor attenuation for 3D depth perception
    float avgZ = 0.5f * (c1.z + c2.z);
    float refDist = cam.getDistance();
    float relDepth = std::max(0.0f, avgZ - refDist);
    float depthFade = std::clamp(1.0f - (relDepth / std::max(400.0f, cam.farPlane)), 0.28f, 1.0f);

    SDL_Color shadedColor = color;
    shadedColor.a = (Uint8)std::clamp((int)(color.a * depthFade), 0, 255);

    drawLineScreen(renderer, sx1, sy1, sx2, sy2, shadedColor, glow * depthFade, thickness);
}

void VectorRenderer::drawPolylineWorld(SDL_Renderer* renderer, const Camera2D& cam,
                                       const std::vector<std::pair<float, float>>& points,
                                       SDL_Color color, float glow, bool closed) {
    if (points.size() < 2) return;
    for (size_t i = 1; i < points.size(); ++i) {
        drawLineWorld(renderer, cam,
                      points[i - 1].first, points[i - 1].second,
                      points[i].first, points[i].second,
                      color, glow);
    }
    if (closed && points.size() > 2) {
        drawLineWorld(renderer, cam,
                      points.back().first, points.back().second,
                      points.front().first, points.front().second,
                      color, glow);
    }
}

void VectorRenderer::drawPolylineWorld3D(SDL_Renderer* renderer, const Camera2D& cam,
                                         const std::vector<Vec3>& points,
                                         SDL_Color color, float glow, bool closed) {
    if (points.size() < 2) return;
    for (size_t i = 1; i < points.size(); ++i) {
        drawLineWorld3D(renderer, cam, points[i - 1], points[i], color, glow);
    }
    if (closed && points.size() > 2) {
        drawLineWorld3D(renderer, cam, points.back(), points.front(), color, glow);
    }
}

void VectorRenderer::drawPolylineScreen(SDL_Renderer* renderer,
                                       const std::vector<std::pair<float, float>>& points,
                                       SDL_Color color, float glow, bool closed) {
    if (points.size() < 2) return;
    for (size_t i = 1; i < points.size(); ++i) {
        drawLineScreen(renderer,
                       points[i - 1].first, points[i - 1].second,
                       points[i].first, points[i].second,
                       color, glow);
    }
    if (closed && points.size() > 2) {
        drawLineScreen(renderer,
                       points.back().first, points.back().second,
                       points.front().first, points.front().second,
                       color, glow);
    }
}

void VectorRenderer::drawRectScreen(SDL_Renderer* renderer, float x, float y, float w, float h,
                                    SDL_Color color, float glow) {
    drawLineScreen(renderer, x, y, x + w, y, color, glow);
    drawLineScreen(renderer, x + w, y, x + w, y + h, color, glow);
    drawLineScreen(renderer, x + w, y + h, x, y + h, color, glow);
    drawLineScreen(renderer, x, y + h, x, y, color, glow);
}

void VectorRenderer::drawScanlines(SDL_Renderer* renderer, int width, int height, Uint8 alpha) {
    if (alpha == 0) return;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
    for (int y = 0; y < height; y += 3) {
        SDL_RenderDrawLine(renderer, 0, y, width, y);
    }
}
