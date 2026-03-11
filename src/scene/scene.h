#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include "collision/polygon_utils.h"
#include "ecs/ecs.h"
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

// Prop: 2.5D object - Z-depth derived from DEPTH MAP at X,Y position
struct Prop {
    Vec3 position;      // X,Y = screen position; Z = depth (auto-calculated from depth map)
    Vec2 size;
    Renderer::TextureID texture;
    Renderer::TextureID normal_map;  // Normal map for lighting (optional)
    std::string name;
    PivotPoint pivot = PivotPoint::TOP_LEFT;
};

struct Hotspot {
    std::string name;
    Collision::Polygon bounds;
    float interaction_distance = 0.0f;
    bool enabled = true;
    std::function<void()> callback;
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
    Renderer::DepthMapData depth_map;  // Depth map data for Z-depth sampling
    
    // ECS entity IDs for props in this scene (created when scene is loaded)
    std::vector<ECS::EntityID> prop_entities;
    
    // ECS entity IDs for lights in this scene
    std::vector<ECS::EntityID> light_entities;
    
    SceneGeometry geometry;
};

struct SceneManager {
    Scene current_scene;
};

// Calculate Z-DEPTH based on depth map sampling
// Coordinate convention:
//   Z = -1.0: NEAR camera (objects appear larger, in front)
//   Z = +1.0: FAR from camera (objects appear smaller, behind)
// Depth map encoding:
//   White pixels (255) = near (Z=-1), Black pixels (0) = far (Z=+1)
inline float get_z_from_depth_map(const Scene& scene, float world_x, float world_y) {
    if (scene.depth_map.is_valid()) {
        // Normalize world position to texture coordinates [0, 1]
        float tex_u = world_x / scene.width;
        float tex_v = world_y / scene.height;
        
        // Clamp to [0, 1]
        tex_u = std::max(0.0f, std::min(1.0f, tex_u));
        tex_v = std::max(0.0f, std::min(1.0f, tex_v));
        
        // Convert to pixel coordinates
        int32_t pixel_x = (int32_t)(tex_u * (scene.depth_map.width - 1));
        int32_t pixel_y = (int32_t)(tex_v * (scene.depth_map.height - 1));
        
        // Sample depth value (0-255)
        uint32_t pixel_index = pixel_y * scene.depth_map.width + pixel_x;
        uint8_t depth_value = scene.depth_map.pixels[pixel_index];
        
        // Normalize to [0, 1]
        float normalized_depth = depth_value / 255.0f;
        
        // Map to Z range: white (255) = near (-1.0), black (0) = far (+1.0)
        // Full range from -1 to +1
        float z = 1.0f - (normalized_depth * 2.0f);
        
        return z;
    } else {
        // No depth map - neutral depth
        return 0.0f;
    }
}

// Calculate depth scaling based on depth map sampling
// Matches the Z-coordinate: negative Z (near) = larger scale, positive Z (far) = smaller scale
inline float get_depth_scaling(const Scene& scene, float world_x, float world_y) {
    if (scene.depth_map.is_valid()) {
        // Get Z value from depth map
        float z_value = get_z_from_depth_map(scene, world_x, world_y);
        
        // Map Z to scale: z=-0.8 (near, large) -> scale=1.4, z=0.8 (far, small) -> scale=0.6
        // Formula: scale = 1.0 - (z_value / 0.8) * 0.4
        float scale = 1.0f - (z_value / 0.8f) * 0.4f;
        return scale;
    } else {
        // Fallback: use Y-based scaling if no depth map
        float t = world_y / scene.height;
        return 0.6f + (t * 0.8f);
    }
}

void init_scene_test();

}
