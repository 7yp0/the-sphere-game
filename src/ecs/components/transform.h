#pragma once

// Transform Components for ECS
// Separates 2.5D game logic coordinates from 3D OpenGL rendering coordinates

#include "../../types.h"
#include "../../renderer/texture_loader.h"
#include <cmath>
#include <algorithm>

namespace ECS {

// ============================================================================
// Transform2_5DComponent - Position in game scene (2.5D space)
// ============================================================================
// This transform describes the entity's position within the game scene.
// Z-depth is typically derived from a depth map based on X,Y position.
//
// Coordinate System:
//   X = horizontal pixel position (0 = left, scene_width = right)
//   Y = vertical pixel position (0 = top, scene_height = bottom)
//   z_depth = depth into screen (-1 = near camera, +1 = far/background)
// ============================================================================

struct Transform2_5DComponent {
    Vec2 position;      // Pixel coordinates in scene
    float z_depth;      // Depth into screen (-1 = near camera, +1 = far/background)
    float elevation;    // Height above ground (0 = on ground, >0 = floating)
    float rotation;     // Rotation in radians
    Vec2 scale;         // Scale factors (1.0 = base size)
    
    Transform2_5DComponent() 
        : position(0, 0), z_depth(0), elevation(0), rotation(0), scale(1, 1) {}
    
    Transform2_5DComponent(Vec2 pos, float z = 0.0f, float elev = 0.0f)
        : position(pos), z_depth(z), elevation(elev), rotation(0), scale(1, 1) {}
    
    Transform2_5DComponent(float x, float y, float z = 0.0f, float elev = 0.0f)
        : position(x, y), z_depth(z), elevation(elev), rotation(0), scale(1, 1) {}
};

// ============================================================================
// Transform3DComponent - Position in OpenGL render space
// ============================================================================
// This transform describes the object's position in actual OpenGL rendering space.
// Usually derived from Transform2_5DComponent during rendering.
//
// Coordinate System:
//   position.x = -1 (left) to +1 (right)
//   position.y = -1 (bottom) to +1 (top)  
//   position.z = -1 (near camera) to +1 (far/background)
//   size = OpenGL normalized dimensions
// ============================================================================

struct Transform3DComponent {
    Vec3 position;      // OpenGL coordinates (-1 to +1)
    Vec2 size;          // OpenGL size (normalized)
    float rotation;     // Rotation in radians
    
    Transform3DComponent() 
        : position(0, 0, 0), size(1, 1), rotation(0) {}
    
    Transform3DComponent(Vec3 pos, Vec2 sz)
        : position(pos), size(sz), rotation(0) {}
};

// ============================================================================
// Helper Functions
// ============================================================================

namespace TransformHelpers {

// Sample Z-depth from depth map at given pixel position
// Depth map encoding: White (255) = near (Z=-1), Black (0) = far (Z=+1)
inline float get_z_from_depth_map(const Renderer::DepthMapData& depth_map, 
                                   float pixel_x, float pixel_y,
                                   uint32_t scene_width, uint32_t scene_height) {
    if (!depth_map.is_valid()) {
        return 0.0f;  // Neutral depth if no depth map
    }
    
    // Normalize world position to texture coordinates [0, 1]
    float tex_u = pixel_x / static_cast<float>(scene_width);
    float tex_v = pixel_y / static_cast<float>(scene_height);
    
    // Clamp to [0, 1]
    tex_u = std::max(0.0f, std::min(1.0f, tex_u));
    tex_v = std::max(0.0f, std::min(1.0f, tex_v));
    
    // Convert to pixel coordinates in depth map
    int32_t map_x = static_cast<int32_t>(tex_u * (depth_map.width - 1));
    int32_t map_y = static_cast<int32_t>(tex_v * (depth_map.height - 1));
    
    // Sample depth value (0-255)
    uint32_t pixel_index = map_y * depth_map.width + map_x;
    uint8_t depth_value = depth_map.pixels[pixel_index];
    
    // Normalize to [0, 1]
    float normalized_depth = depth_value / 255.0f;
    
    // Map to Z range: white (255) = near (-1.0), black (0) = far (+1.0)
    float z = 1.0f - (normalized_depth * 2.0f);
    
    return z;
}

// Update Transform2_5D z_depth from depth map
inline void update_z_from_depth_map(Transform2_5DComponent& transform,
                                     const Renderer::DepthMapData& depth_map,
                                     uint32_t scene_width, uint32_t scene_height) {
    transform.z_depth = get_z_from_depth_map(depth_map, 
                                              transform.position.x, 
                                              transform.position.y,
                                              scene_width, scene_height);
}

// Calculate depth-based scaling factor
// Objects further away (positive Z) appear smaller, closer objects (negative Z) appear larger
// Returns scale multiplier (typically 0.6 to 1.4 range)
inline float compute_depth_scale(float z_depth) {
    // Map Z to scale: z=-1 (near) -> larger scale, z=+1 (far) -> smaller scale
    // Using similar formula to existing code:
    // z=-0.8 -> scale=1.4, z=0 -> scale=1.0, z=0.8 -> scale=0.6
    float scale = 1.0f - (z_depth / 0.8f) * 0.4f;
    return std::max(0.2f, std::min(2.0f, scale));  // Clamp to reasonable range
}

// Convert pixel coordinates to OpenGL normalized coordinates
// Pixel space: X=(0 to width), Y=(0=top to height=bottom)
// OpenGL space: X=(-1 to +1), Y=(-1=bottom to +1=top)
inline Vec2 pixel_to_opengl(Vec2 pixel_pos, uint32_t viewport_width, uint32_t viewport_height) {
    // Flip Y: pixel space has Y=0 at top, OpenGL has Y=-1 at bottom
    float y_flipped = viewport_height - pixel_pos.y;
    return Vec2(
        (pixel_pos.x / static_cast<float>(viewport_width)) * 2.0f - 1.0f,
        (y_flipped / static_cast<float>(viewport_height)) * 2.0f - 1.0f
    );
}

// Convert pixel size to OpenGL normalized size
inline Vec2 pixel_size_to_opengl(Vec2 pixel_size, uint32_t viewport_width, uint32_t viewport_height) {
    return Vec2(
        (pixel_size.x / static_cast<float>(viewport_width)) * 2.0f,
        (pixel_size.y / static_cast<float>(viewport_height)) * 2.0f
    );
}

// Derive Transform3D from Transform2_5D
// Converts 2.5D game coordinates to OpenGL render coordinates
// base_size = original sprite size in pixels
// pivot = sprite pivot point for positioning
inline Transform3DComponent derive_3d_from_2_5d(const Transform2_5DComponent& transform_2_5d,
                                                  Vec2 base_size,
                                                  uint32_t scene_width, uint32_t scene_height,
                                                  bool apply_depth_scaling = true) {
    Transform3DComponent result;
    
    // Convert pixel position to OpenGL coordinates
    Vec2 gl_pos = pixel_to_opengl(transform_2_5d.position, scene_width, scene_height);
    result.position = Vec3(gl_pos.x, gl_pos.y, transform_2_5d.z_depth);
    
    // Apply depth scaling to size if enabled
    float scale_factor = 1.0f;
    if (apply_depth_scaling) {
        scale_factor = compute_depth_scale(transform_2_5d.z_depth);
    }
    
    // Apply entity scale and depth scaling
    Vec2 scaled_size = base_size;
    scaled_size.x *= transform_2_5d.scale.x * scale_factor;
    scaled_size.y *= transform_2_5d.scale.y * scale_factor;
    
    // Convert to OpenGL size
    result.size = pixel_size_to_opengl(scaled_size, scene_width, scene_height);
    
    // Copy rotation
    result.rotation = transform_2_5d.rotation;
    
    return result;
}

// Apply pivot offset to OpenGL position
// Returns the center position accounting for pivot
inline Vec3 apply_pivot_offset(Vec3 position, Vec2 size, PivotPoint pivot) {
    Vec2 offset = Coords::get_pivot_offset(pivot, size.x, size.y);
    return Vec3(position.x + offset.x, position.y + offset.y, position.z);
}

} // namespace TransformHelpers

} // namespace ECS
