#pragma once

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
        return (static_cast<float>(layer) / 100.0f) * 2.0f - 1.0f;
    }
}
