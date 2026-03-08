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

// Undefine Windows macros that conflict with std::min/std::max
#undef min
#undef max

namespace Scene {

struct Prop {
    Vec3 position;
    Vec2 size;
    Renderer::TextureID texture;
    Renderer::TextureID normal_map;  // Normal map for lighting (optional)
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
    Renderer::TextureID background_normal_map;  // Normal map for background lighting
    Renderer::HeightMapData height_map;  // Height map data for depth-based sampling
    std::vector<Prop> props;
    
    std::vector<HorizonLine> horizons;  // Legacy - will be removed when height map fully works
    std::vector<PointLight> lights;     // Dynamic point lights for scene illumination
    SceneGeometry geometry;
};

struct SceneManager {
    Scene current_scene;
};

// Forward declare find_closest_horizon for get_depth_scaling
HorizonLine* find_closest_horizon(Scene& scene, float sprite_y);

// Calculate Z-depth based on height map sampling
// Convention: Z = -1 (near camera = large), Z = 1 (far from camera = small)
// White pixels (255) = near, Black pixels (0) = far
inline float get_z_from_height_map(const Scene& scene, float world_x, float world_y) {
    if (scene.height_map.is_valid()) {
        // Normalize world position to texture coordinates [0, 1]
        float tex_u = world_x / scene.width;
        float tex_v = world_y / scene.height;
        
        // Clamp to [0, 1]
        tex_u = std::max(0.0f, std::min(1.0f, tex_u));
        tex_v = std::max(0.0f, std::min(1.0f, tex_v));
        
        // Convert to pixel coordinates
        int32_t pixel_x = (int32_t)(tex_u * (scene.height_map.width - 1));
        int32_t pixel_y = (int32_t)(tex_v * (scene.height_map.height - 1));
        
        // Sample height value (0-255)
        uint32_t pixel_index = pixel_y * scene.height_map.width + pixel_x;
        uint8_t height_value = scene.height_map.pixels[pixel_index];
        
        // Normalize to [0, 1]
        float normalized_height = height_value / 255.0f;
        
        // Map to Z range: white (255) = near (-0.8), black (0) = far (0.8)
        float z = 0.8f - (normalized_height * 1.6f);
        
        return z;
    } else {
        // No height map - neutral depth
        return 0.0f;
    }
}

// Calculate depth scaling based on height map sampling
// Matches the Z-coordinate: negative Z (near) = larger scale, positive Z (far) = smaller scale
inline float get_depth_scaling(const Scene& scene, float world_x, float world_y) {
    if (scene.height_map.is_valid()) {
        // Get Z value from height map (which encodes depth)
        float z_value = get_z_from_height_map(scene, world_x, world_y);
        
        // Map Z to scale: z=-0.8 (near, large) -> scale=1.4, z=0.8 (far, small) -> scale=0.6
        // Formula: scale = 1.0 - (z_value / 0.8) * 0.4
        float scale = 1.0f - (z_value / 0.8f) * 0.4f;
        return scale;
    } else {
        // Fallback: use Y-based scaling if no height map
        float t = world_y / scene.height;
        return 0.6f + (t * 0.8f);
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
