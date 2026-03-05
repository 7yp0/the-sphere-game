#pragma once

#include <cstdint>

struct Vec2 {
    float x, y;
    
    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}
};

struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

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

enum class Layer : int {
    BACKGROUND = 0,
    MIDGROUND  = 10,
    ENTITIES   = 20,
    PLAYER     = 30,
    OCCLUSION  = 40,
    FOREGROUND = 50,
    UI         = 100
};

namespace Coords {

inline Vec2 pixel_to_opengl(Vec2 pixel_pos, uint32_t viewport_width, uint32_t viewport_height) {
    return Vec2(
        (pixel_pos.x / (float)viewport_width) * 2.0f - 1.0f,
        (pixel_pos.y / (float)viewport_height) * 2.0f - 1.0f
    );
}

inline Vec2 opengl_to_pixel(Vec2 opengl_pos, uint32_t viewport_width, uint32_t viewport_height) {
    return Vec2(
        (opengl_pos.x + 1.0f) * 0.5f * viewport_width,
        (opengl_pos.y + 1.0f) * 0.5f * viewport_height
    );
}

}

namespace Layers {
    inline float get_parallax(Layer layer) {
        switch (layer) {
            case Layer::BACKGROUND:  return 0.1f;
            case Layer::MIDGROUND:   return 1.0f;
            case Layer::ENTITIES:    return 1.0f;
            case Layer::PLAYER:      return 1.0f;
            case Layer::OCCLUSION:   return 1.0f;
            case Layer::FOREGROUND:  return 1.2f;
            case Layer::UI:          return 0.0f;
            default:                 return 1.0f;
        }
    }
    
    inline float get_z_depth(Layer layer) {
        // With GL_LESS: smaller z = closer to camera (rendered on top)
        // layer 0 (BACKGROUND) → z = 1.0 (far)
        // layer 100 (UI) → z = -1.0 (near)
        return 1.0f - (static_cast<float>(layer) / 100.0f) * 2.0f;
    }
}
