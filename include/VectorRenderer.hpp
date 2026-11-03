#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <cmath>

struct Camera2D {
    float x = 0.0f;       // [m] World center position X
    float y = 0.0f;       // [m] World center position Y
    float zoom = 3.28084f;// [px/m] Camera magnification (pixels per meter; base zoom 1.0 px/ft = 3.28084 px/m)
    int screenWidth = 1280;  // [px] Viewport width in pixels
    int screenHeight = 720;  // [px] Viewport height in pixels

    void worldToScreen(float wx, float wy, float& sx, float& sy) const {
        sx = (wx - x) * zoom + screenWidth * 0.5f;
        sy = (wy - y) * zoom + screenHeight * 0.5f;
    }

    void screenToWorld(float sx, float sy, float& wx, float& wy) const {
        wx = (sx - screenWidth * 0.5f) / zoom + x;
        wy = (sy - screenHeight * 0.5f) / zoom + y;
    }
};

class VectorRenderer {
public:
    static void drawLineScreen(SDL_Renderer* renderer, float x1, float y1, float x2, float y2,
                               SDL_Color color, float glow = 1.0f, int thickness = 1); // coordinates in [px]

    static void drawLineWorld(SDL_Renderer* renderer, const Camera2D& cam,
                              float wx1, float wy1, float wx2, float wy2,
                              SDL_Color color, float glow = 1.0f, int thickness = 1); // coordinates in [m]

    static void drawPolylineWorld(SDL_Renderer* renderer, const Camera2D& cam,
                                  const std::vector<std::pair<float, float>>& points,
                                  SDL_Color color, float glow = 1.0f, bool closed = false); // points in [m]

    static void drawPolylineScreen(SDL_Renderer* renderer,
                                  const std::vector<std::pair<float, float>>& points,
                                  SDL_Color color, float glow = 1.0f, bool closed = false); // points in [px]

    static void drawRectScreen(SDL_Renderer* renderer, float x, float y, float w, float h,
                               SDL_Color color, float glow = 1.0f); // x, y, w, h in [px]

    static void drawScanlines(SDL_Renderer* renderer, int width, int height, Uint8 alpha = 18); // width, height in [px]
};
