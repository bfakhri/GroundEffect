#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#include <algorithm>

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    constexpr Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    constexpr float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    constexpr Vec3 cross(const Vec3& o) const {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }
    float lengthSq() const { return x * x + y * y + z * z; }
    float length() const { return std::sqrt(lengthSq()); }
    Vec3 normalized() const {
        float len = length();
        return (len > 1e-6f) ? Vec3{x / len, y / len, z / len} : Vec3{0.0f, 0.0f, 0.0f};
    }
};

enum class CameraViewMode {
    CHASE_3D = 0,
    ORBIT_3D,
    ISOMETRIC_3D,
    CLASSIC_2D,
    COUNT
};

// Rocket League ProfileCameraSettings parameters
struct RocketLeagueCameraSettings {
    float fov = 110.0f;            // [deg] Horizontal Field of View (Rocket League range: 60..110, pro meta: 110)
    float distance = 270.0f;       // [uu] Base camera distance behind vehicle (pro meta: 270 -> ~82.3 m)
    float height = 100.0f;         // [uu] Vertical height offset above vehicle center (pro meta: 100 -> ~9.1 m)
    float pitch = -4.0f;           // [deg] Downward pitch angle tilt (Rocket League range: -15..0, pro meta: -4.0)
    float stiffness = 0.40f;       // [0.0..1.0] Spring stiffness & supersonic distance stretch factor (pro meta: 0.40)
    float swivelSpeed = 5.00f;     // [1.0..10.0] Right-stick camera swivel & auto-center speed (pro meta: 5.0)
    float transitionSpeed = 1.20f; // [1.0..2.0] Camera mode transition slerp speed (pro meta: 1.20)
};

struct Camera2D {
    float x = 0.0f;          // [m] World target center position X
    float y = 0.0f;          // [m] World target center position Y (+Y is downward)
    float z = 0.0f;          // [m] World target center position Z (+Z is north/forward into screen)
    float zoom = 3.28084f;   // [px/m] Camera magnification (pixels per meter at target plane)
    int screenWidth = 1280;  // [px] Viewport width in pixels
    int screenHeight = 720;  // [px] Viewport height in pixels

    // 3D Perspective Camera Parameters
    bool use3D = true;       // [flag] True = 3D perspective projection, False = 2D orthographic
    CameraViewMode viewMode = CameraViewMode::CHASE_3D;
    float camYaw = 0.0f;     // [rad] Orbit yaw angle in XZ plane (0 = looking from -Z toward +Z)
    float camPitch = 0.30f;  // [rad] Elevation pitch angle above horizon (~17.2 deg looking down)
    float fovY = 0.92f;      // [rad] Vertical field of view (~52.7 deg)
    float nearPlane = 1.5f;  // [m] Near clipping plane distance
    float farPlane = 2600.0f;// [m] Far reference distance for phosphor depth shading
    RocketLeagueCameraSettings rlSettings{};

    // Full 6-DOF custom orthonormal camera basis (used when anchored 1:1 to ship's 3D rotation matrix)
    bool hasCustomBasis = false;
    Vec3 customRight{1.0f, 0.0f, 0.0f};
    Vec3 customCamDown{0.0f, 1.0f, 0.0f};
    Vec3 customForward{0.0f, 0.0f, 1.0f};

    float getFocalLengthPx() const {
        return (screenHeight * 0.5f) / std::tan(fovY * 0.5f);
    }

    float getDistance() const {
        return getFocalLengthPx() / std::max(0.25f, zoom);
    }

    void getBasis(Vec3& outCamPos, Vec3& outRight, Vec3& outCamDown, Vec3& outForward) const {
        if (hasCustomBasis) {
            outRight   = customRight;
            outCamDown = customCamDown;
            outForward = customForward;
        } else {
            float cosY = std::cos(camYaw);
            float sinY = std::sin(camYaw);
            float cosP = std::cos(camPitch);
            float sinP = std::sin(camPitch);

            outForward = { sinY * cosP, sinP, cosY * cosP };
            outRight   = { cosY, 0.0f, -sinY };
            outCamDown = { -sinP * sinY, cosP, -sinP * cosY };
        }

        Vec3 target{x, y, z};
        outCamPos = target - outForward * getDistance();
    }

    // Transform world point (wx, wy, wz) into camera-space coordinates (cx, cy, cz)
    Vec3 worldToCameraSpace(const Vec3& w) const {
        Vec3 camPos, right, camDown, forward;
        getBasis(camPos, right, camDown, forward);
        Vec3 rel = w - camPos;
        return { rel.dot(right), rel.dot(camDown), rel.dot(forward) };
    }

    // Project 3D world point (wx, wy, wz) to 2D screen coordinates (sx, sy).
    // Returns false if the point is behind the near clipping plane.
    bool worldToScreen3D(float wx, float wy, float wz, float& sx, float& sy, float* outDepth = nullptr) const {
        if (!use3D) {
            sx = (wx - x) * zoom + screenWidth * 0.5f;
            sy = (wy - y) * zoom + screenHeight * 0.5f;
            if (outDepth) *outDepth = getDistance();
            return true;
        }
        Vec3 c = worldToCameraSpace({wx, wy, wz});
        if (outDepth) *outDepth = c.z;
        if (c.z < nearPlane) {
            return false;
        }
        float f = getFocalLengthPx();
        sx = screenWidth * 0.5f + (c.x / c.z) * f;
        sy = screenHeight * 0.5f + (c.y / c.z) * f;
        return true;
    }

    void worldToScreen(float wx, float wy, float& sx, float& sy) const {
        if (use3D) {
            float d = 0.0f;
            if (worldToScreen3D(wx, wy, 0.0f, sx, sy, &d)) {
                return;
            }
        }
        sx = (wx - x) * zoom + screenWidth * 0.5f;
        sy = (wy - y) * zoom + screenHeight * 0.5f;
    }

    void screenToWorld(float sx, float sy, float& wx, float& wy) const {
        wx = (sx - screenWidth * 0.5f) / zoom + x;
        wy = (sy - screenHeight * 0.5f) / zoom + y;
    }
};

using Camera3D = Camera2D;

class VectorRenderer {
public:
    static void drawLineScreen(SDL_Renderer* renderer, float x1, float y1, float x2, float y2,
                               SDL_Color color, float glow = 1.0f, int thickness = 1); // coordinates in [px]

    static void drawLineWorld(SDL_Renderer* renderer, const Camera2D& cam,
                              float wx1, float wy1, float wx2, float wy2,
                              SDL_Color color, float glow = 1.0f, int thickness = 1); // coordinates in [m]

    static void drawLineWorld3D(SDL_Renderer* renderer, const Camera2D& cam,
                                const Vec3& p1, const Vec3& p2,
                                SDL_Color color, float glow = 1.0f, int thickness = 1); // coordinates in [m]

    static void drawLineWorld3D(SDL_Renderer* renderer, const Camera2D& cam,
                                float wx1, float wy1, float wz1,
                                float wx2, float wy2, float wz2,
                                SDL_Color color, float glow = 1.0f, int thickness = 1) {
        drawLineWorld3D(renderer, cam, Vec3{wx1, wy1, wz1}, Vec3{wx2, wy2, wz2}, color, glow, thickness);
    }

    static void drawPointWorld3D(SDL_Renderer* renderer, const Camera2D& cam,
                                 float wx, float wy, float wz,
                                 SDL_Color color, float glow = 1.0f, int size = 1);

    static void drawPolylineWorld(SDL_Renderer* renderer, const Camera2D& cam,
                                  const std::vector<std::pair<float, float>>& points,
                                  SDL_Color color, float glow = 1.0f, bool closed = false); // points in [m]

    static void drawPolylineWorld3D(SDL_Renderer* renderer, const Camera2D& cam,
                                    const std::vector<Vec3>& points,
                                    SDL_Color color, float glow = 1.0f, bool closed = false); // points in [m]

    static void drawPolylineScreen(SDL_Renderer* renderer,
                                  const std::vector<std::pair<float, float>>& points,
                                  SDL_Color color, float glow = 1.0f, bool closed = false); // points in [px]

    static void drawRectScreen(SDL_Renderer* renderer, float x, float y, float w, float h,
                               SDL_Color color, float glow = 1.0f); // x, y, w, h in [px]

    static void drawScanlines(SDL_Renderer* renderer, int width, int height, Uint8 alpha = 18); // width, height in [px]
};
