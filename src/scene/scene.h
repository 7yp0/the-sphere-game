#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include "collision/polygon_utils.h"
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

namespace Scene {

struct Prop {
    Vec3 position;
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

struct Hotspot {
    std::string name;
    Collision::Polygon bounds;
    float interaction_distance = 0.0f;
    bool enabled = true;
    std::function<void()> callback;
};

struct PointLight {
    Vec3 position;      // World position (screen space + z-depth)
    Vec3 color;         // RGB color (0-1 range)
    float intensity;    // Brightness multiplier
    float radius;       // Maximum distance from light where it has effect
};

struct SceneGeometry {
    std::vector<Collision::Polygon> walkable_areas;
    std::vector<Hotspot> hotspots;
};

struct Scene {
    std::string name;
    uint32_t width;
    uint32_t height;
    Renderer::TextureID background;
    Renderer::TextureID height_map;  // Height map for depth-based sprite scaling
    std::vector<Prop> props;
    
    std::vector<HorizonLine> horizons;  // Legacy - will be removed when height map fully works
    std::vector<PointLight> lights;     // Dynamic point lights for scene illumination
    SceneGeometry geometry;
};

struct SceneManager {
    Scene current_scene;
};

// Forward declare find_closest_horizon for get_depth_scaling
inline HorizonLine* find_closest_horizon(Scene& scene, float sprite_y);

// Calculate depth scaling based on sprite Y position
// Uses height map if available, otherwise uses legacy horizon lines
inline float get_depth_scaling(Scene& scene, float sprite_y) {
    if (scene.height_map) {
        // Height map available - calculate scale based on Y position
        // Map sprite_y from screen space (0-720) to depth (0.8 to 1.2 scale)
        float t = sprite_y / scene.height;  // Normalize to 0-1 (0=top, 1=bottom)
        // Deeper (bottom) = larger scale, higher (top) = smaller scale
        return 0.8f + (t * 0.4f);  // Scales from 0.8 (top) to 1.2 (bottom)
    } else {
        // Fallback to horizon line system
        HorizonLine* closest_horizon = find_closest_horizon(scene, sprite_y);
        if (!closest_horizon) return 1.0f;  // No scaling
        
        float scale = 1.0f + (closest_horizon->depth_scale_inverted ? -1.0f : 1.0f) * 
                      (sprite_y - closest_horizon->y_position) * closest_horizon->scale_gradient;
        return scale;
    }
}

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
