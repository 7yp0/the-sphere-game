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
// 2.5D RENDERING ARCHITECTURE
// =============================================================================
//
// RENDERING OVERVIEW
// ------------------
// This engine renders a 2.5D scene composed of:
//   - A pre-rendered 2D background image
//   - Dynamic scene objects (props, player) rendered as textured quads
//   - Dynamic point lights that affect both background and objects
//
// The background receives lighting from dynamic light sources but does NOT
// cast shadows. Shadows are cast exclusively by scene objects.
//
// COORDINATE SYSTEMS
// ------------------
// Two coordinate systems work together:
//
// 1. 2.5D Object System (Background, Props, Player)
//    - Objects have X,Y screen position
//    - Z-depth is derived from the depth map at the object's pivot position
//    - Depth scaling: objects further from camera (higher Z) render smaller
//
// 2. 3D Light System (PointLights)
//    - Coordinates in OpenGL space (-1 to +1)
//    - X = horizontal screen position (left/right)
//    - Y = vertical screen position (bottom/top)
//    - Z = depth into screen (-1=near camera, +1=far/background)
//
// =============================================================================
// BACKGROUND LIGHTING (DEPTH MAP + NORMAL MAP)
// =============================================================================
//
// The background image is accompanied by two auxiliary textures:
//
// DEPTH MAP
//   - Encodes the approximate Z-depth of each background pixel
//   - White (255) = near camera (Z=-1), Black (0) = far (Z=+1)
//   - Used to reconstruct the 3D position of each background pixel in view space
//   - This reconstructed position is used for:
//     a) Computing the direction vector toward each light source
//     b) Computing light attenuation based on distance
//   - The depth map is NOT used as shadow-casting geometry
//
// NORMAL MAP
//   - Encodes the surface normal direction at each background pixel
//   - Used for diffuse lighting calculations (N dot L)
//   - Allows the flat background to respond to dynamic lights as if it had
//     actual surface geometry (bumps, angles, depth variation)
//
// LIGHTING CALCULATION (per background pixel):
//   1. Sample depth map → reconstruct approximate 3D position
//   2. Sample normal map → get surface normal
//   3. Compute light direction vector from pixel position to light
//   4. Apply diffuse lighting: intensity * max(0, dot(normal, light_dir))
//   5. Apply distance attenuation based on light radius
//
// =============================================================================
// SHADOW CASTING MODEL
// =============================================================================
//
// IMPORTANT: The background does NOT cast shadows. Only scene objects do.
//
// Shadow computation does NOT use ray marching through the depth map.
// Instead, shadows are computed via geometric ray-object intersection tests.
//
// SHADOW RAY ALGORITHM (per shaded pixel):
//   1. Reconstruct background pixel position from screen coords + depth map
//   2. Cast a ray from the shaded point toward the light source
//   3. Test this ray ONLY against shadow-casting scene objects
//   4. If any object blocks the ray, the pixel is in shadow
//
// Shadow-casting objects are rendered as quads (two triangles) in 3D space.
// The intersection test is a standard ray-quad intersection.
//
// =============================================================================
// OBJECT INTERSECTION AND ALPHA TESTING
// =============================================================================
//
// Scene objects are sprite-based quads with potentially transparent regions.
// To cast accurate shadows that respect transparency:
//
// RAY-QUAD INTERSECTION:
//   1. Test if the shadow ray intersects the object's quad geometry
//   2. If hit, compute UV coordinates at the intersection point
//   3. Sample the object's texture at those UV coordinates
//
// ALPHA TEST:
//   4. If sampled alpha > threshold: ray is blocked, pixel is shadowed
//   5. If sampled alpha <= threshold: ray continues to next object
//
// This allows sprites with irregular silhouettes (e.g., trees, characters)
// to cast shadows that match their visible shape rather than their bounding
// quad.
//
// =============================================================================
// LIMITATIONS AND TRADE-OFFS
// =============================================================================
//
// ADVANTAGES:
//   - Performance: No expensive ray marching through volumetric data
//   - Simplicity: Shadow casters are simple quad geometry
//   - Accuracy: Alpha testing produces pixel-accurate shadow silhouettes
//   - Flexibility: Background art can have complex painted depth/normals
//
// LIMITATIONS:
//   - Background geometry cannot cast shadows onto itself or objects
//   - Self-shadowing within the background is baked into the art
//   - Shadow casters are limited to quad geometry (no complex meshes)
//   - Soft shadows require additional techniques (not covered here)
//
// This architecture is well-suited for 2.5D adventure games where the
// background is pre-rendered artwork and dynamic objects need to cast
// shadows onto that artwork.
//
// =============================================================================

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
    // Example placements:
    //   Light at Y=0.5, Z=0.0 → upper screen, at scene mid-depth
    //   Light at Y=-0.5, Z=0.5 → lower screen, further into background
    //
    // Shadow behavior:
    //   Shadows are cast by scene objects (quads) via ray intersection tests.
    //   Shadow direction depends on relative positions of light and caster.
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
