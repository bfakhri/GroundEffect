#include "VectorFont.hpp"
#include <map>
#include <cctype>
#include <algorithm>

static std::map<char, std::vector<VectorStroke>> s_glyphs;
static bool s_initialized = false;

static void addStroke(char c, const std::vector<VectorPoint>& pts) {
    s_glyphs[c].push_back(VectorStroke{pts});
}

void VectorFont::init() {
    if (s_initialized) return;
    s_initialized = true;

    // Grid coordinates: width = 4.0f, height = 6.0f
    // Top is y = 0, Bottom is y = 6

    // Digits
    addStroke('0', {{0,0}, {4,0}, {4,6}, {0,6}, {0,0}, {4,6}});
    addStroke('1', {{1,1}, {2,0}, {2,6}, {0,6}, {4,6}});
    addStroke('2', {{0,1}, {1,0}, {3,0}, {4,1}, {4,2}, {0,6}, {4,6}});
    addStroke('3', {{0,0}, {4,0}, {2,2.8f}, {4,3.2f}, {4,5}, {3,6}, {0,6}});
    addStroke('4', {{0,0}, {0,3}, {4,3}});
    addStroke('4', {{3,0}, {3,6}});
    addStroke('5', {{4,0}, {0,0}, {0,2.8f}, {3.5f,2.8f}, {4,3.5f}, {4,5.2f}, {3,6}, {0,6}});
    addStroke('6', {{3,0}, {0,2}, {0,6}, {4,6}, {4,3}, {0,3}});
    addStroke('7', {{0,0}, {4,0}, {1.5f,6}});
    addStroke('8', {{0,0}, {4,0}, {4,3}, {0,3}, {0,0}});
    addStroke('8', {{0,3}, {4,3}, {4,6}, {0,6}, {0,3}});
    addStroke('9', {{4,3}, {0,3}, {0,0}, {4,0}, {4,6}, {1,6}});

    // Letters
    addStroke('A', {{0,6}, {0,2}, {2,0}, {4,2}, {4,6}});
    addStroke('A', {{0,3.5f}, {4,3.5f}});
    addStroke('B', {{0,0}, {3,0}, {4,1}, {4,2}, {3,3}, {0,3}, {0,6}, {3,6}, {4,5}, {4,4}, {3,3}});
    addStroke('C', {{4,1}, {3,0}, {1,0}, {0,1}, {0,5}, {1,6}, {3,6}, {4,5}});
    addStroke('D', {{0,0}, {2.8f,0}, {4,1.5f}, {4,4.5f}, {2.8f,6}, {0,6}, {0,0}});
    addStroke('E', {{4,0}, {0,0}, {0,6}, {4,6}});
    addStroke('E', {{0,3}, {3,3}});
    addStroke('F', {{0,6}, {0,0}, {4,0}});
    addStroke('F', {{0,3}, {3,3}});
    addStroke('G', {{4,1}, {3,0}, {1,0}, {0,1}, {0,5}, {1,6}, {3,6}, {4,5}, {4,3}, {2,3}});
    addStroke('H', {{0,0}, {0,6}});
    addStroke('H', {{4,0}, {4,6}});
    addStroke('H', {{0,3}, {4,3}});
    addStroke('I', {{1,0}, {3,0}});
    addStroke('I', {{2,0}, {2,6}});
    addStroke('I', {{1,6}, {3,6}});
    addStroke('J', {{3,0}, {3,5}, {2,6}, {0,6}, {0,4}});
    addStroke('K', {{0,0}, {0,6}});
    addStroke('K', {{4,0}, {0,3.2f}, {4,6}});
    addStroke('L', {{0,0}, {0,6}, {4,6}});
    addStroke('M', {{0,6}, {0,0}, {2,3}, {4,0}, {4,6}});
    addStroke('N', {{0,6}, {0,0}, {4,6}, {4,0}});
    addStroke('O', {{1,0}, {3,0}, {4,1}, {4,5}, {3,6}, {1,6}, {0,5}, {0,1}, {1,0}});
    addStroke('P', {{0,6}, {0,0}, {3,0}, {4,1}, {4,2.5f}, {3,3.5f}, {0,3.5f}});
    addStroke('Q', {{1,0}, {3,0}, {4,1}, {4,5}, {3,6}, {1,6}, {0,5}, {0,1}, {1,0}});
    addStroke('Q', {{2.5f,4.5f}, {4.5f,6.5f}});
    addStroke('R', {{0,6}, {0,0}, {3,0}, {4,1}, {4,2.5f}, {3,3.5f}, {0,3.5f}});
    addStroke('R', {{2,3.5f}, {4,6}});
    addStroke('S', {{4,1}, {3,0}, {1,0}, {0,1}, {0,2.5f}, {4,3.5f}, {4,5}, {3,6}, {1,6}, {0,5}});
    addStroke('T', {{0,0}, {4,0}});
    addStroke('T', {{2,0}, {2,6}});
    addStroke('U', {{0,0}, {0,5}, {1,6}, {3,6}, {4,5}, {4,0}});
    addStroke('V', {{0,0}, {2,6}, {4,0}});
    addStroke('W', {{0,0}, {1,6}, {2,3}, {3,6}, {4,0}});
    addStroke('X', {{0,0}, {4,6}});
    addStroke('X', {{4,0}, {0,6}});
    addStroke('Y', {{0,0}, {2,2.8f}, {4,0}});
    addStroke('Y', {{2,2.8f}, {2,6}});
    addStroke('Z', {{0,0}, {4,0}, {0,6}, {4,6}});

    // Symbols
    addStroke('-', {{0.5f,3}, {3.5f,3}});
    addStroke('+', {{2,1}, {2,5}});
    addStroke('+', {{0.5f,3}, {3.5f,3}});
    addStroke(':', {{2,1.5f}, {2,2}});
    addStroke(':', {{2,4.5f}, {2,5}});
    addStroke('.', {{1.5f,5.5f}, {2.5f,5.5f}});
    addStroke('/', {{0,6}, {4,0}});
    addStroke('\\', {{0,0}, {4,6}});
    addStroke('!', {{2,0}, {2,4}});
    addStroke('!', {{2,5.2f}, {2,6}});
    addStroke('?', {{0,1}, {1,0}, {3,0}, {4,1}, {4,2.5f}, {2,3.5f}, {2,4.5f}});
    addStroke('?', {{2,5.4f}, {2,6}});
    addStroke('<', {{3.5f,0.5f}, {0.5f,3}, {3.5f,5.5f}});
    addStroke('>', {{0.5f,0.5f}, {3.5f,3}, {0.5f,5.5f}});
    addStroke('^', {{0.5f,4}, {2,1}, {3.5f,4}});
    addStroke('v', {{0.5f,2}, {2,5}, {3.5f,2}});
    addStroke('*', {{2,1}, {2,5}});
    addStroke('*', {{0.5f,2}, {3.5f,4}});
    addStroke('*', {{0.5f,4}, {3.5f,2}});
    addStroke('[', {{3,0}, {1,0}, {1,6}, {3,6}});
    addStroke(']', {{1,0}, {3,0}, {3,6}, {1,6}});
    addStroke('(', {{3,0}, {1,2}, {1,4}, {3,6}});
    addStroke(')', {{1,0}, {3,2}, {3,4}, {1,6}});
    addStroke('%', {{0,1}, {1,0}, {1,1}, {0,1}});
    addStroke('%', {{0,6}, {4,0}});
    addStroke('%', {{3,5}, {4,5}, {4,6}, {3,6}});
}

const std::vector<VectorStroke>& VectorFont::getGlyph(char c) {
    init();
    char upper = (char)std::toupper((unsigned char)c);
    auto it = s_glyphs.find(upper);
    if (it != s_glyphs.end()) {
        return it->second;
    }
    static std::vector<VectorStroke> empty;
    return empty;
}

void VectorFont::renderStrokes(SDL_Renderer* renderer, const std::vector<VectorStroke>& strokes, 
                               float x, float y, float scale, 
                               SDL_Color color, float glow) {
    if (strokes.empty()) return;

    // Optional vector CRT glow effect:
    // First pass: softer, translucent glow lines with small offsets
    if (glow > 0.0f) {
        Uint8 glowAlpha = (Uint8)std::clamp((int)(color.a * 0.28f * glow), 0, 255);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, glowAlpha);
        for (const auto& stroke : strokes) {
            for (size_t i = 1; i < stroke.points.size(); ++i) {
                int x1 = (int)(x + stroke.points[i - 1].x * scale);
                int y1 = (int)(y + stroke.points[i - 1].y * scale);
                int x2 = (int)(x + stroke.points[i].x * scale);
                int y2 = (int)(y + stroke.points[i].y * scale);
                
                SDL_RenderDrawLine(renderer, x1 - 1, y1, x2 - 1, y2);
                SDL_RenderDrawLine(renderer, x1 + 1, y1, x2 + 1, y2);
                SDL_RenderDrawLine(renderer, x1, y1 - 1, x2, y2 - 1);
                SDL_RenderDrawLine(renderer, x1, y1 + 1, x2, y2 + 1);
            }
        }
    }

    // Core bright vector line
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (const auto& stroke : strokes) {
        for (size_t i = 1; i < stroke.points.size(); ++i) {
            int x1 = (int)(x + stroke.points[i - 1].x * scale);
            int y1 = (int)(y + stroke.points[i - 1].y * scale);
            int x2 = (int)(x + stroke.points[i].x * scale);
            int y2 = (int)(y + stroke.points[i].y * scale);
            SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        }
    }
}

void VectorFont::drawText(SDL_Renderer* renderer, const std::string& text, 
                          float x, float y, float scale, 
                          SDL_Color color, float glow) {
    init();
    float currentX = x;
    const float charWidth = 4.0f * scale;
    const float spacing = 2.0f * scale;

    for (char c : text) {
        if (c == ' ') {
            currentX += charWidth * 0.8f;
            continue;
        }
        const auto& glyph = getGlyph(c);
        renderStrokes(renderer, glyph, currentX, y, scale, color, glow);
        currentX += charWidth + spacing;
    }
}

void VectorFont::drawTextCentered(SDL_Renderer* renderer, const std::string& text, 
                                  float centerX, float y, float scale, 
                                  SDL_Color color, float glow) {
    float width = getTextWidth(text, scale);
    drawText(renderer, text, centerX - width * 0.5f, y, scale, color, glow);
}

float VectorFont::getTextWidth(const std::string& text, float scale) {
    if (text.empty()) return 0.0f;
    const float charWidth = 4.0f * scale;
    const float spacing = 2.0f * scale;
    float width = 0.0f;
    for (char c : text) {
        if (c == ' ') {
            width += charWidth * 0.8f;
        } else {
            width += charWidth + spacing;
        }
    }
    return width;
}

float VectorFont::getCharHeight(float scale) {
    return 6.0f * scale;
}
