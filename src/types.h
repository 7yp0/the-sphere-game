#pragma once

#include <cstdint>

// Forward declarations
struct Vec2;
struct Vec3;

struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3(const Vec2& v, float z = 0.0f);  // Forward declare, defined below
};

struct Vec2 {
    float x, y;
    
    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}
    Vec2(const Vec3& v) : x(v.x), y(v.y) {}  // Extract x, y from Vec3
};

// Define Vec3 constructor from Vec2
inline Vec3::Vec3(const Vec2& v, float z) : x(v.x), y(v.y), z(z) {}

struct Vec4 {
    float x, y, z, w;
    
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

struct Color {
    float r, g, b, a;
    
    Color() : r(1), g(1), b(1), a(1) {}
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
};

enum class PivotPoint : int {
    TOP_LEFT = 0,
    TOP_CENTER = 1,
    TOP_RIGHT = 2,
    CENTER_LEFT = 3,
    CENTER = 4,
    CENTER_RIGHT = 5,
    BOTTOM_LEFT = 6,
    BOTTOM_CENTER = 7,
    BOTTOM_RIGHT = 8
};

namespace Coords {

// Calculate the offset from a pivot point to the center of a rect
// Returns the offset in normalized space relative to the rect
inline Vec2 get_pivot_offset(PivotPoint pivot, float width, float height) {
    float x_offset = 0.0f, y_offset = 0.0f;
    
    switch (pivot) {
        case PivotPoint::TOP_LEFT:     x_offset = 0.5f;  y_offset = 0.5f;  break;
        case PivotPoint::TOP_CENTER:   x_offset = 0.0f;  y_offset = 0.5f;  break;
        case PivotPoint::TOP_RIGHT:    x_offset = -0.5f; y_offset = 0.5f;  break;
        case PivotPoint::CENTER_LEFT:  x_offset = 0.5f;  y_offset = 0.0f;  break;
        case PivotPoint::CENTER:       x_offset = 0.0f;  y_offset = 0.0f;  break;
        case PivotPoint::CENTER_RIGHT: x_offset = -0.5f; y_offset = 0.0f;  break;
        case PivotPoint::BOTTOM_LEFT:  x_offset = 0.5f;  y_offset = -0.5f; break;
        case PivotPoint::BOTTOM_CENTER:x_offset = 0.0f;  y_offset = -0.5f; break;
        case PivotPoint::BOTTOM_RIGHT: x_offset = -0.5f; y_offset = -0.5f; break;
    }
    
    return Vec2(x_offset * width, y_offset * height);
}

inline Vec2 pixel_to_opengl(Vec2 pixel_pos, uint32_t viewport_width, uint32_t viewport_height) {
    // Flip Y: pixel space has Y=0 at top, OpenGL has Y=-1 at bottom
    float y_flipped = viewport_height - pixel_pos.y;
    return Vec2(
        (pixel_pos.x / (float)viewport_width) * 2.0f - 1.0f,
        (y_flipped / (float)viewport_height) * 2.0f - 1.0f
    );
}

inline Vec2 opengl_to_pixel(Vec2 opengl_pos, uint32_t viewport_width, uint32_t viewport_height) {
    return Vec2(
        (opengl_pos.x + 1.0f) * 0.5f * viewport_width,
        (opengl_pos.y + 1.0f) * 0.5f * viewport_height
    );
}

}

// =============================================================================
// Z-Depth Constants
// =============================================================================
// With GL_LESS: smaller z = closer to camera (rendered on top)
// Range: -1.0 (nearest) to +1.0 (farthest)
//
// Game world objects (props, player) use DEPTH MAP for z-ordering.
// These constants are for elements that don't use depth maps.
// =============================================================================

namespace ZDepth {
    // Scene layers (rendered to FBO at base resolution)
    constexpr float BACKGROUND = 0.99f;     // Scene background (farthest)
    
    // Game world: -1.0 to +1.0 range controlled by depth map
    // Props and player z-depth calculated from depth_map sampling
    
    // UI layers (rendered after upscale at viewport resolution)
    // Ordered from back to front:
    constexpr float GAME_HUD   = -0.90f;    // HUD elements, inventory icon
    constexpr float PANELS     = -0.93f;    // Inventory panel, close-up overlays
    constexpr float MAIN_MENU  = -0.94f;    // Main menu overlay (above panels)
    constexpr float DIALOGUE   = -0.95f;    // Speech bubbles, text boxes
    constexpr float TOOLTIP    = -0.97f;    // Hover tooltips
    constexpr float CURSOR     = -0.99f;    // Mouse cursor (always on top)
}

// =============================================================================
// Parallax Factors (for future parallax scrolling system)
// =============================================================================
// Factor determines how layer moves relative to camera:
// < 1.0 = moves slower (appears distant)
// = 1.0 = moves with camera (game world)
// > 1.0 = moves faster (appears close, inverse parallax)
// = 0.0 = static (doesn't move with camera)
// =============================================================================

namespace Parallax {
    constexpr float FAR        = 0.3f;      // Distant elements (sky, mountains)
    constexpr float MID        = 0.6f;      // Mid-distance scenery  
    constexpr float NEAR       = 0.85f;     // Close background details
    constexpr float WORLD      = 1.0f;      // Game world (props, player)
    constexpr float FOREGROUND = 1.15f;     // Decorative foreground elements
    constexpr float STATIC     = 0.0f;      // UI, fixed elements
}
