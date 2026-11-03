#include "VectorRenderer.hpp"
#include <algorithm>

void VectorRenderer::drawLineScreen(SDL_Renderer* renderer, float x1, float y1, float x2, float y2,
                                    SDL_Color color, float glow, int thickness) {
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
    float sx1, sy1, sx2, sy2;
    cam.worldToScreen(wx1, wy1, sx1, sy1);
    cam.worldToScreen(wx2, wy2, sx2, sy2);

    // No viewport cull here — SDL clips lines natively, and culling by endpoint
    // bounding-box drops long segments whose midpoint crosses the screen edge.
    drawLineScreen(renderer, sx1, sy1, sx2, sy2, color, glow, thickness);
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
