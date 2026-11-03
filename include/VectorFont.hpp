#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>

struct VectorPoint {
    float x; // [px] Relative glyph stroke point X coordinate
    float y; // [px] Relative glyph stroke point Y coordinate
};

struct VectorStroke {
    std::vector<VectorPoint> points; // Connected line strip in [px] font unit space
};

class VectorFont {
public:
    static void init();
    static void drawText(SDL_Renderer* renderer, const std::string& text, 
                         float x, float y, float scale, 
                         SDL_Color color, float glow = 1.0f); // x, y in [px], scale [dimensionless]
    static void drawTextCentered(SDL_Renderer* renderer, const std::string& text, 
                                 float centerX, float y, float scale, 
                                 SDL_Color color, float glow = 1.0f); // centerX, y in [px], scale [dimensionless]
    static float getTextWidth(const std::string& text, float scale);  // returns [px]
    static float getCharHeight(float scale);                          // returns [px]

private:
    static const std::vector<VectorStroke>& getGlyph(char c);
    static void renderStrokes(SDL_Renderer* renderer, const std::vector<VectorStroke>& strokes, 
                              float x, float y, float scale, 
                              SDL_Color color, float glow); // x, y in [px], scale [dimensionless]
};
