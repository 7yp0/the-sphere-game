#pragma once

#include "ecs.h"
#include "types.h"
#include "renderer/renderer.h"
#include <string>

namespace ECS {

// Entity Factory Functions
// These create complete entities with appropriate components pre-configured

// ============================================================================
// PROP FACTORIES
// ============================================================================

/**
 * Create a static prop entity (receives lighting but does NOT cast shadows)
 * Use for: background decorations, distant objects, transparent overlays
 */
EntityID create_static_prop(
    Vec2 position,
    Vec2 size,
    Renderer::TextureID texture,
    Renderer::TextureID normal_map = 0,
    PivotPoint pivot = PivotPoint::TOP_LEFT
);

/**
 * Create a shadow-casting prop entity (receives lighting and CASTS shadows)
 * Use for: furniture, trees, characters, any solid objects
 */
EntityID create_shadow_casting_prop(
    Vec2 position,
    Vec2 size,
    Renderer::TextureID texture,
    Renderer::TextureID normal_map = 0,
    PivotPoint pivot = PivotPoint::TOP_LEFT,
    float alpha_threshold = 0.3f,
    float shadow_intensity = 0.7f
);

// ============================================================================
// LIGHT FACTORIES
// ============================================================================

/**
 * Create a point light entity (3D positioned)
 * Position is in OpenGL coordinates (-1 to +1)
 */
EntityID create_point_light(
    Vec3 position,
    Vec3 color,
    float intensity,
    float radius,
    bool casts_shadows = true
);

/**
 * Create a point light at pixel position
 * Automatically converts pixel coords to OpenGL coords
 * z_depth: -1 = near camera, +1 = far (background)
 */
EntityID create_point_light_at_pixel(
    Vec2 pixel_position,
    float z_depth,
    Vec3 color,
    float intensity,
    float radius,
    bool casts_shadows = true,
    uint32_t scene_width = 320,
    uint32_t scene_height = 180
);

// ============================================================================
// COMPOSITE FACTORIES
// ============================================================================

/**
 * Create a lamp entity (visible sprite + light source)
 * Returns the sprite entity ID; light is attached as child
 */
EntityID create_lamp(
    Vec2 position,
    Vec2 sprite_size,
    Renderer::TextureID sprite_texture,
    Renderer::TextureID sprite_normal_map,
    Vec3 light_offset,          // Offset from sprite center in OpenGL coords
    Vec3 light_color,
    float light_intensity,
    float light_radius,
    bool lamp_casts_shadow = false  // Usually lamps don't cast shadows
);

/**
 * Create an emissive object (glows but doesn't illuminate others)
 * Use for: screens, magic effects, glowing items
 */
EntityID create_emissive_object(
    Vec2 position,
    Vec2 size,
    Renderer::TextureID texture,
    Vec3 emissive_color,        // RGB, values > 1.0 for HDR glow
    PivotPoint pivot = PivotPoint::TOP_LEFT
);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Update entity's z_depth from depth map based on current position
 */
void update_entity_z_from_depth_map(
    EntityID entity,
    const Renderer::DepthMapData& depth_map,
    uint32_t scene_width,
    uint32_t scene_height
);

} // namespace ECS
