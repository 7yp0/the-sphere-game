#include "entity_factory.h"
#include "game/game.h"
#include "config.h"
#include <cstdio>

using Game::g_state;

namespace ECS {

// ============================================================================
// PROP FACTORIES
// ============================================================================

EntityID create_static_prop(
    Vec2 position,
    Vec2 size,
    Renderer::TextureID texture,
    Renderer::TextureID normal_map,
    PivotPoint pivot
) {
    EntityID entity = g_state.ecs_world.create_entity();
    
    // Add Transform2_5D
    auto& transform = g_state.ecs_world.add_component<Transform2_5DComponent>(entity);
    transform.position = position;
    transform.z_depth = 0.0f;  // Will be updated from depth map
    transform.scale = Vec2(1.0f, 1.0f);
    
    // Add SpriteComponent
    auto& sprite = g_state.ecs_world.add_component<SpriteComponent>(entity);
    sprite.texture = texture;
    sprite.normal_map = normal_map;
    sprite.base_size = size;
    sprite.pivot = pivot;
    sprite.visible = true;
    
    // NO ShadowCasterComponent - static props don't cast shadows
    
    return entity;
}

EntityID create_shadow_casting_prop(
    Vec2 position,
    Vec2 size,
    Renderer::TextureID texture,
    Renderer::TextureID normal_map,
    PivotPoint pivot,
    float alpha_threshold,
    float shadow_intensity
) {
    EntityID entity = g_state.ecs_world.create_entity();
    
    // Add Transform2_5D
    auto& transform = g_state.ecs_world.add_component<Transform2_5DComponent>(entity);
    transform.position = position;
    transform.z_depth = 0.0f;  // Will be updated from depth map
    transform.scale = Vec2(1.0f, 1.0f);
    
    // Add SpriteComponent
    auto& sprite = g_state.ecs_world.add_component<SpriteComponent>(entity);
    sprite.texture = texture;
    sprite.normal_map = normal_map;
    sprite.base_size = size;
    sprite.pivot = pivot;
    sprite.visible = true;
    
    // Add ShadowCasterComponent
    auto& shadow = g_state.ecs_world.add_component<ShadowCasterComponent>(entity);
    shadow.enabled = true;
    shadow.alpha_threshold = alpha_threshold;
    shadow.shadow_intensity = shadow_intensity;
    
    return entity;
}

// ============================================================================
// LIGHT FACTORIES
// ============================================================================

EntityID create_point_light(
    Vec3 position,
    Vec3 color,
    float intensity,
    float radius,
    bool casts_shadows
) {
    EntityID entity = g_state.ecs_world.create_entity();
    
    // Lights use Transform3D for free 3D positioning (OpenGL coords)
    auto& transform = g_state.ecs_world.add_component<Transform3DComponent>(entity);
    transform.position = position;
    
    // Add LightComponent
    auto& light = g_state.ecs_world.add_component<LightComponent>(entity);
    light.color = color;
    light.intensity = intensity;
    light.radius = radius;
    light.casts_shadows = casts_shadows;
    light.enabled = true;
    
    return entity;
}

EntityID create_point_light_at_pixel(
    Vec2 pixel_position,
    float z_depth,
    Vec3 color,
    float intensity,
    float radius,
    bool casts_shadows,
    uint32_t scene_width,
    uint32_t scene_height
) {
    // Convert pixel position to OpenGL coordinates
    Vec2 opengl_pos = TransformHelpers::pixel_to_opengl(pixel_position, scene_width, scene_height);
    Vec3 position(opengl_pos.x, opengl_pos.y, z_depth);
    
    return create_point_light(position, color, intensity, radius, casts_shadows);
}

// ============================================================================
// COMPOSITE FACTORIES
// ============================================================================

EntityID create_lamp(
    Vec2 position,
    Vec2 sprite_size,
    Renderer::TextureID sprite_texture,
    Renderer::TextureID sprite_normal_map,
    Vec3 light_offset,
    Vec3 light_color,
    float light_intensity,
    float light_radius,
    bool lamp_casts_shadow
) {
    // Create the sprite entity
    EntityID sprite_entity;
    if (lamp_casts_shadow) {
        sprite_entity = create_shadow_casting_prop(
            position, sprite_size, sprite_texture, sprite_normal_map,
            PivotPoint::BOTTOM_CENTER);
    } else {
        sprite_entity = create_static_prop(
            position, sprite_size, sprite_texture, sprite_normal_map,
            PivotPoint::BOTTOM_CENTER);
    }
    
    // Get sprite's transform to calculate light position
    auto* transform = g_state.ecs_world.get_component<Transform2_5DComponent>(sprite_entity);
    if (transform) {
        // Convert sprite position to OpenGL coords and add offset
        Vec2 opengl_pos = TransformHelpers::pixel_to_opengl(
            transform->position, Config::BASE_WIDTH, Config::BASE_HEIGHT);
        Vec3 light_pos(
            opengl_pos.x + light_offset.x,
            opengl_pos.y + light_offset.y,
            transform->z_depth + light_offset.z
        );
        
        // Create the light entity
        create_point_light(light_pos, light_color, light_intensity, light_radius, true);
    }
    
    return sprite_entity;
}

EntityID create_emissive_object(
    Vec2 position,
    Vec2 size,
    Renderer::TextureID texture,
    Vec3 emissive_color,
    PivotPoint pivot
) {
    // Create base prop (no shadow casting)
    EntityID entity = create_static_prop(position, size, texture, 0, pivot);
    
    // Add EmissiveComponent
    auto& emissive = g_state.ecs_world.add_component<EmissiveComponent>(entity);
    emissive.emissive_color = emissive_color;
    
    return entity;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void update_entity_z_from_depth_map(
    EntityID entity,
    const Renderer::DepthMapData& depth_map,
    uint32_t scene_width,
    uint32_t scene_height
) {
    auto* transform = g_state.ecs_world.get_component<Transform2_5DComponent>(entity);
    if (transform && depth_map.is_valid()) {
        TransformHelpers::update_z_from_depth_map(
            *transform, depth_map, scene_width, scene_height);
    }
}

} // namespace ECS
