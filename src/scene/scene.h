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

// =============================================================================
// 2.5D + 3D HYBRID RENDERING SYSTEM WITH RAY-MARCHED SHADOWS
// =============================================================================
// This engine uses TWO coordinate systems that work together:
//
// 1. 2.5D SYSTEM (Background, Props, Player)
//    - Objects have X,Y screen position only
//    - Z-DEPTH is AUTOMATICALLY derived from DEPTH MAP (Y position → sample → Z)
//    - Props/Player move on 2D plane, depth map determines their depth
//    - Depth scaling: objects further back (higher Z) render smaller
//
// 2. 3D LIGHTING SYSTEM (PointLights)
//    - All coordinates in OpenGL space (-1 to +1)
//    - X = horizontal screen position (left/right)
//    - Y = vertical screen position (bottom/top)
//    - Z = depth (near/far into screen) → affects shadow direction
//
// INTERPLAY FOR LIGHTING:
//    - Shader samples depth map → gets Z-depth for each background pixel
//    - Shader samples normal map → gets surface normal for each background pixel
//    - Light 3D position compared against pixel position + Z-depth + surface normal
//    - Result: accurate lighting and shadows based on depth map depth and normals
//
// RAY-MARCHED SHADOWS FOR OBJECTS:
//    - Objects (props, player) cast shadows onto the 2.5D background
//    - Lights exist in full 3D space, objects have Z from depth map
//    - Shadow ray marches from background fragment toward light
//    - At each step: check if an object occludes the ray
//
//    SHADOW DIRECTION RULES:
//    ┌─────────────────────────────────────────────────────────────────┐
//    │ CAMERA ◄───── Z-Axis ─────► BACKGROUND                         │
//    │   Z=-1                           Z=+1                          │
//    │                                                                │
//    │ Case 1: Caster BETWEEN camera and light (casterZ < lightZ)     │
//    │   [CAMERA] ──► [CASTER] ──► [LIGHT] ──► [BACKGROUND]          │
//    │   Shadow cast TOWARD camera (onto near surfaces)               │
//    │                                                                │
//    │ Case 2: Caster BEHIND light (casterZ > lightZ)                 │
//    │   [CAMERA] ──► [LIGHT] ──► [CASTER] ──► [BACKGROUND]          │
//    │   Shadow cast AWAY from camera (onto far surfaces)             │
//    └─────────────────────────────────────────────────────────────────┘
//
//    IMPLEMENTATION:
//    - For each lit background pixel, march ray toward light
//    - Check object bounding boxes + depth for occlusion
//    - Shadow intensity based on occluder distance and size
//

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

// PointLight: 3D positioning in OpenGL coordinate space
struct PointLight {
    // COORDINATE SYSTEM (ALL IN OPENGL SPACE -1 to +1):
    //   X = horizontal screen position (-1=left, +1=right)
    //   Y = vertical screen position (-1=bottom, +1=top)
    //   Z = depth into screen (-1=near camera, +1=far/background)
    //
    // NOTE: This matches the 2.5D system - X,Y are screen coords, Z is depth!
    //   Light at Y=0.5, Z=0.0 → center-top of screen, at scene level
    //   Light at Y=-0.5, Z=0.5 → lower screen, behind objects
    //
    // SHADOW DIRECTION (automatic from ray marching):
    //   - Caster between camera and light: shadow cast TOWARD camera
    //   - Caster behind light: shadow cast AWAY from camera
    //   - Shadow length/intensity based on occluder thickness and distance
    Vec3 position;      // OpenGL coords (-1 to +1), X,Y = screen pos, Z = depth
    Vec3 color;         // RGB color (0-1 range)
    float intensity;    // Brightness multiplier
    float radius;       // Radius in OpenGL units (0-4 range typical)
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
    std::vector<Prop> props;
    
    std::vector<PointLight> lights;     // Dynamic point lights for scene illumination
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
