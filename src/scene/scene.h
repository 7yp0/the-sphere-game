#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

namespace Scene {

struct Prop {
    Vec2 position;
    Vec2 size;
    Renderer::TextureID texture;
    std::string name;
    PivotPoint pivot = PivotPoint::TOP_LEFT;
};

struct HorizonLine {
    float y_position;
    float scale_gradient = 0.003f;     // Depth scaling per pixel (default 30% per 100px)
    bool depth_scale_inverted = false; // true = isometric/inverted perspective
};

struct Scene {
    std::string name;
    uint32_t width;
    uint32_t height;
    Renderer::TextureID background;
    std::vector<Prop> props;
    
    std::vector<HorizonLine> horizons;
};

struct SceneManager {
    Scene current_scene;
};

inline HorizonLine* find_closest_horizon(Scene& scene, float sprite_y) {
    if (scene.horizons.empty()) return nullptr;
    if (scene.horizons.size() == 1) return &scene.horizons[0];
    
    // Sort horizons by y_position (for zone detection)
    std::vector<HorizonLine*> sorted;
    for (auto& h : scene.horizons) {
        sorted.push_back(&h);
    }
    std::sort(sorted.begin(), sorted.end(), 
        [](HorizonLine* a, HorizonLine* b) { return a->y_position < b->y_position; });
    
    // Check if sprite is between two horizons (zone = no scaling)
    for (size_t i = 0; i < sorted.size() - 1; i++) {
        if (sprite_y >= sorted[i]->y_position && sprite_y <= sorted[i + 1]->y_position) {
            return nullptr; // Sprite is in transition zone - no scaling
        }
    }
    
    // Sprite is outside zones - return the closest horizon
    HorizonLine* closest = sorted[0];
    float min_dist = std::abs(sprite_y - sorted[0]->y_position);
    
    for (auto* horizon : sorted) {
        float dist = std::abs(sprite_y - horizon->y_position);
        if (dist < min_dist) {
            min_dist = dist;
            closest = horizon;
        }
    }
    
    return closest;
}

void init_scene_test();

}
