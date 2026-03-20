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
#include <unordered_map>

// Undefine Windows macros that conflict with std::min/std::max
#undef min
#undef max

namespace Scene {

// How the player interacts with a hotspot
enum class InteractionType {
    IMMEDIATE,        // Trigger callback immediately on click (no walking)
    WALK_TO_HOTSPOT,  // Walk to hotspot centroid (pivot point), then trigger
    WALK_TO_TARGET,   // Walk to specific target_position, then trigger
    TRIGGER           // Fire callback once when player enters the polygon (no click needed)
};

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
    std::string name;           // Internal identifier
    std::string tooltip_key;    // Localization key for tooltip (displayed as-is until localization system exists)
    Collision::Polygon bounds;
    bool enabled = true;
    std::function<void()> callback;  // Default interaction (no item)

    // Item-on-hotspot callbacks: item_id -> callback
    std::unordered_map<std::string, std::function<void()>> item_callbacks;

    // Interaction type determines how player triggers this hotspot
    InteractionType interaction_type = InteractionType::WALK_TO_HOTSPOT;

    // Target position for WALK_TO_TARGET interaction type
    Vec2 target_position = Vec2(0.0f, 0.0f);

    // If true, double-clicking this hotspot fires the callback immediately (no walking)
    bool double_click_immediate = false;

    // Runtime state for TRIGGER type: edge-detect player entry (not persisted to JSON)
    bool was_inside = false;
};

struct SpawnPoint {
    std::string name;        // e.g. "default", "from_kitchen", "from_outside"
    Vec2 position;
    std::string direction;   // "down" (default), "up", "left", "right"
};

struct SceneGeometry {
    std::vector<Collision::Polygon> walkable_areas;
    std::vector<Collision::Polygon> obstacles;  // Blocked areas within walkable areas
    std::vector<Hotspot> hotspots;
    std::vector<SpawnPoint> spawn_points;
};

struct Scene {
    std::string name;
    uint32_t width;
    uint32_t height;

    // Background (midground) — rendered behind props/player
    Renderer::TextureID background;
    Renderer::TextureID background_normal_map;
    Renderer::DepthMapData depth_map;

    // Foreground — rendered in front of props/player
    Renderer::TextureID foreground            = 0;
    Renderer::TextureID foreground_normal_map = 0;

    // Z range for depth map: white pixels map to z_near, black pixels map to z_far.
    // Props range from -0.97 (nearest) to +0.97 (farthest), leaving room for foreground (-0.99) and background (0.99).
    float depth_z_near = -0.97f;  // Z value for white (255) pixels
    float depth_z_far  =  0.97f;  // Z value for black (0) pixels
    
    // ECS entity IDs for props in this scene (created when scene is loaded)
    std::vector<ECS::EntityID> prop_entities;
    
    // ECS entity IDs for lights in this scene
    std::vector<ECS::EntityID> light_entities;
    
    // ECS entity IDs for projector lights (window lights)
    std::vector<ECS::EntityID> projector_light_entities;
    
    // Named entity lookup (name -> entity ID) - works for all entity types
    std::unordered_map<std::string, ECS::EntityID> named_entities;
    
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

        // white (255) -> z_near,  black (0) -> z_far
        float t = depth_value / 255.0f;
        return scene.depth_z_far + t * (scene.depth_z_near - scene.depth_z_far);
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

// Register a callback for a hotspot by name (called after loading geometry from JSON)
// Returns true if hotspot found and callback registered
bool register_hotspot_callback(const std::string& hotspot_name, std::function<void()> callback);

// Register an item-on-hotspot callback (called when player uses item on hotspot)
// Returns true if hotspot found and callback registered
bool register_hotspot_item_callback(const std::string& hotspot_name, const std::string& item_id, std::function<void()> callback);

// Enable or disable a hotspot by name (disabled hotspots don't respond to clicks)
// Returns true if hotspot found
bool set_hotspot_enabled(const std::string& hotspot_name, bool enabled);

// Get a hotspot by name (returns nullptr if not found)
Hotspot* get_hotspot(const std::string& hotspot_name);

// Register an entity with a name for later lookup (works for any entity type)
void register_entity(const std::string& name, ECS::EntityID entity);

// Get an entity by name (returns INVALID_ENTITY if not found)
ECS::EntityID get_entity(const std::string& name);

// Hide/show an entity's sprite by name
bool set_entity_visible(const std::string& name, bool visible);

// Get spawn point by name (returns false if not found)
bool get_spawn_point(const std::string& name, Vec2& out_pos, std::string& out_direction);

}
