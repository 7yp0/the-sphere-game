#include "game.h"
#include "player.h"
#include "core/animation_bank.h"
#include "core/timing.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "renderer/animation.h"
#include "platform.h"
#include "scene/scene.h"
#include "debug/debug.h"
#include "types.h"
#include "ecs/ecs.h"
#include "ecs/entity_factory.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstdio>

namespace Game {

GameState g_state;

// Helper: Get player transform (for update loop)
static ECS::Transform2_5DComponent* get_player_transform() {
    if (g_state.player_entity == ECS::INVALID_ENTITY) return nullptr;
    return g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(g_state.player_entity);
}

// Helper: Get player walker component  
static ECS::WalkerComponent* get_player_walker() {
    if (g_state.player_entity == ECS::INVALID_ENTITY) return nullptr;
    return g_state.ecs_world.get_component<ECS::WalkerComponent>(g_state.player_entity);
}

// Helper: Update sprite animation from player state
static void update_player_sprite_animation() {
    if (g_state.player_entity == ECS::INVALID_ENTITY) return;
    
    auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(g_state.player_entity);
    if (sprite) {
        const char* anim_name = player_get_animation_name(g_state.player);
        Renderer::SpriteAnimation* anim = g_state.player.animations.get(anim_name);
        sprite->set_animation(anim);
    }
}

// Static light position for testing (behind box prop)
// Animate light through room corners
// Visits 8 corners in 3D space for full room illumination coverage
static void update_animated_light(float delta_time) {
    if (g_state.scene.light_entities.empty()) return;
    
    static float light_time = 0.0f;
    light_time += delta_time;
    
    ECS::EntityID light_entity = g_state.scene.light_entities[0];
    auto* transform = g_state.ecs_world.get_component<ECS::Transform3DComponent>(light_entity);
    if (!transform) return;
    
    // 8 corner waypoints with margin (0.6 from edges)
    // OpenGL coords: X (-1=left, +1=right), Y (-1=bottom, +1=top), Z (-1=near, +1=far)
    const float margin = 0.6f;
    const Vec3 corners[8] = {
        Vec3(-margin,  margin, -margin),  // 0: top-left-near
        Vec3( margin,  margin, -margin),  // 1: top-right-near
        Vec3( margin, -margin, -margin),  // 2: bottom-right-near
        Vec3(-margin, -margin, -margin),  // 3: bottom-left-near
        Vec3(-margin, -margin + 0.6f,  margin),  // 4: bottom-left-far
        Vec3( margin, -margin + 0.6f,  margin),  // 5: bottom-right-far
        Vec3( margin,  margin,  margin),  // 6: top-right-far
        Vec3(-margin,  margin,  margin),  // 7: top-left-far
    };
    
    // Time per corner segment
    const float segment_time = 2.0f;  // 2 seconds per segment
    const float total_cycle = segment_time * 8.0f;  // 16 seconds full cycle
    
    // Calculate which segment we're in and interpolation factor
    float cycle_pos = fmod(light_time, total_cycle);
    int segment = (int)(cycle_pos / segment_time);
    float t = (cycle_pos - segment * segment_time) / segment_time;
    
    // Smooth interpolation (ease in/out)
    t = t * t * (3.0f - 2.0f * t);  // Smoothstep
    
    // Get current and next corner
    int next_segment = (segment + 1) % 8;
    Vec3 from = corners[segment];
    Vec3 to = corners[next_segment];
    
    // Lerp position
    transform->position = Vec3(
        from.x + (to.x - from.x) * t,
        from.y + (to.y - from.y) * t,
        from.z + (to.z - from.z) * t
    );
}

void init() {    
    // Initialize scene (this also creates prop ECS entities)
    Scene::init_scene_test();
    
    // Try to load geometry from JSON (if exists, will override hardcoded geometry)
    Debug::load_scene_geometry();
    
    // Create player entity (this initializes player and its ECS transform)
    g_state.player_entity = player_create_entity(g_state.player, g_state.ecs_world,
                                                  g_state.base_width, g_state.base_height);
    
    // Initialize framebuffer for offscreen rendering at base resolution
    Renderer::init_framebuffer(Config::BASE_WIDTH, Config::BASE_HEIGHT);
}

void set_viewport(uint32_t width, uint32_t height) {
    g_state.viewport_width = width;
    g_state.viewport_height = height;
    
    // Store base dimensions for game logic (all coordinates in base resolution)
    g_state.base_width = Config::BASE_WIDTH;
    g_state.base_height = Config::BASE_HEIGHT;
    
    // Calculate 2.5D depth scaling factor
    // scale_factor = viewport_height / base_height
    // Used in depth scaling formula: scale = 1.0 + (horizon_y - sprite.y) * scale_factor
    g_state.scale_factor = (float)height / (float)Config::BASE_HEIGHT;
}

void update(float delta_time) {
    Core::update_delta_time(delta_time);
    
#ifndef NDEBUG
    Debug::handle_debug_keys();
#endif
    
    // Get player transform and walker from ECS
    ECS::Transform2_5DComponent* player_transform = get_player_transform();
    ECS::WalkerComponent* player_walker = get_player_walker();
    if (player_transform && player_walker) {
        player_handle_input(g_state.player, *player_transform, *player_walker);
        player_update(g_state.player, *player_transform, *player_walker, 
                      g_state.base_width, g_state.base_height, delta_time);
        
        // Update sprite animation based on player state
        update_player_sprite_animation();
    }
    
    // Animate light through room corners
    update_animated_light(delta_time);
}

void render() {
    // =========================================================================
    // GATHER LIGHT DATA FROM ECS
    // =========================================================================
    std::vector<Renderer::LightData> lights;
    for (ECS::EntityID light_entity : g_state.scene.light_entities) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform3DComponent>(light_entity);
        auto* light = g_state.ecs_world.get_component<ECS::LightComponent>(light_entity);
        
        if (!transform || !light || !light->enabled) continue;
        
        Renderer::LightData ld;
        ld.position = transform->position;
        ld.color = light->color;
        ld.intensity = light->intensity;
        ld.radius = light->radius;
        lights.push_back(ld);
    }
    
    const Renderer::LightData* light_data = lights.empty() ? nullptr : lights.data();
    uint32_t num_lights = (uint32_t)lights.size();
    
    // =========================================================================
    // GATHER SHADOW CASTER DATA FROM ECS
    // =========================================================================
    std::vector<Renderer::ShadowCasterData> shadow_casters;
    int32_t entity_index = 0;
    
    for (ECS::EntityID prop_entity : g_state.scene.prop_entities) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(prop_entity);
        auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(prop_entity);
        auto* shadow = g_state.ecs_world.get_component<ECS::ShadowCasterComponent>(prop_entity);
        
        if (!transform || !sprite || !sprite->visible) {
            entity_index++;
            continue;
        }
        
        // Only add if entity has ShadowCasterComponent and it's enabled
        if (shadow && shadow->enabled) {
            // IMPORTANT: Use BASE resolution (FBO dimensions) for OpenGL conversion
            // NOT get_render_width() which returns viewport dims before FBO is bound!
            uint32_t render_width = Config::BASE_WIDTH;
            uint32_t render_height = Config::BASE_HEIGHT;
            
            // Calculate depth scale and size (same as rendering)
            float depth_scale = ECS::TransformHelpers::compute_depth_scale(transform->z_depth);
            Vec2 scaled_size = Vec2(
                sprite->base_size.x * transform->scale.x * depth_scale,
                sprite->base_size.y * transform->scale.y * depth_scale
            );
            
            // Convert pixel position to OpenGL coords
            Vec2 opengl_pos = Coords::pixel_to_opengl(transform->position, render_width, render_height);
            Vec2 opengl_size = Vec2(
                (scaled_size.x / (float)render_width) * 2.0f,
                (scaled_size.y / (float)render_height) * 2.0f
            );
            
            // Calculate center position based on pivot
            Vec2 pivot_offset = Coords::get_pivot_offset(sprite->pivot, scaled_size.x, scaled_size.y);
            Vec2 pivot_offset_opengl = Vec2(
                (pivot_offset.x / (float)render_width) * 2.0f,
                -(pivot_offset.y / (float)render_height) * 2.0f
            );
            Vec2 opengl_center = Vec2(
                opengl_pos.x + pivot_offset_opengl.x,
                opengl_pos.y + pivot_offset_opengl.y
            );
            
            Renderer::ShadowCasterData sc;
            sc.position = Vec3(opengl_center.x, opengl_center.y, transform->z_depth);
            sc.size = opengl_size;
            sc.uv_range = sprite->uv_range;
            sc.texture = sprite->texture;
            sc.alpha_threshold = shadow->alpha_threshold;
            sc.shadow_intensity = shadow->shadow_intensity;
            sc.entity_index = entity_index;
            
            shadow_casters.push_back(sc);
        }
        entity_index++;
    }
    
    // Also add player as shadow caster if it has the component
    if (g_state.player_entity != ECS::INVALID_ENTITY) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(g_state.player_entity);
        auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(g_state.player_entity);
        auto* shadow = g_state.ecs_world.get_component<ECS::ShadowCasterComponent>(g_state.player_entity);
        
        if (transform && sprite && sprite->visible && shadow && shadow->enabled) {
            // IMPORTANT: Use BASE resolution for OpenGL conversion
            uint32_t render_width = Config::BASE_WIDTH;
            uint32_t render_height = Config::BASE_HEIGHT;
            
            float depth_scale = ECS::TransformHelpers::compute_depth_scale(transform->z_depth);
            Vec2 scaled_size = Vec2(
                sprite->base_size.x * transform->scale.x * depth_scale,
                sprite->base_size.y * transform->scale.y * depth_scale
            );
            
            Vec2 opengl_pos = Coords::pixel_to_opengl(transform->position, render_width, render_height);
            Vec2 opengl_size = Vec2(
                (scaled_size.x / (float)render_width) * 2.0f,
                (scaled_size.y / (float)render_height) * 2.0f
            );
            
            Vec2 pivot_offset = Coords::get_pivot_offset(sprite->pivot, scaled_size.x, scaled_size.y);
            Vec2 pivot_offset_opengl = Vec2(
                (pivot_offset.x / (float)render_width) * 2.0f,
                -(pivot_offset.y / (float)render_height) * 2.0f
            );
            Vec2 opengl_center = Vec2(
                opengl_pos.x + pivot_offset_opengl.x,
                opengl_pos.y + pivot_offset_opengl.y
            );
            
            Renderer::ShadowCasterData sc;
            sc.position = Vec3(opengl_center.x, opengl_center.y, transform->z_depth);
            sc.size = opengl_size;
            // For animated sprites, use the current frame's UV range
            if (sprite->is_animated() && sprite->animation && !sprite->animation->frames.empty()) {
                const auto& frame = sprite->animation->frames[sprite->animation->current_frame];
                sc.uv_range = Vec4(frame.u0, frame.v0, frame.u1, frame.v1);
                sc.texture = sprite->animation->texture;
            } else {
                sc.uv_range = sprite->uv_range;
                sc.texture = sprite->texture;
            }
            sc.alpha_threshold = shadow->alpha_threshold;
            sc.shadow_intensity = shadow->shadow_intensity;
            sc.entity_index = entity_index;  // Player gets next index after props
            
            shadow_casters.push_back(sc);
        }
    }
    
    const Renderer::ShadowCasterData* shadow_data = shadow_casters.empty() ? nullptr : shadow_casters.data();
    uint32_t num_shadow_casters = (uint32_t)shadow_casters.size();
    g_state.shadow_caster_count = num_shadow_casters;  // For debug display
    
    // =========================================================================
    // GATHER PROJECTOR LIGHT DATA FROM ECS
    // =========================================================================
    std::vector<Renderer::ProjectorLightData> projector_lights;
    for (ECS::EntityID proj_entity : g_state.scene.projector_light_entities) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(proj_entity);
        auto* projector = g_state.ecs_world.get_component<ECS::ProjectorLightComponent>(proj_entity);
        
        if (!transform || !projector || !projector->enabled) continue;
        
        Renderer::ProjectorLightData pd;
        pd.position = Vec3(transform->position.x, transform->position.y, transform->z_depth);
        pd.direction = projector->direction;
        pd.up = projector->up;
        pd.color = projector->color;
        pd.intensity = projector->intensity;
        pd.fov = projector->fov;
        pd.aspect_ratio = projector->aspect_ratio;
        pd.range = projector->range;
        pd.cookie = projector->cookie;
        projector_lights.push_back(pd);
    }
    
    // Set projector lights (global state used by all lit renders)
    if (!projector_lights.empty()) {
        Renderer::set_projector_lights(projector_lights.data(), (uint32_t)projector_lights.size());
    } else {
        Renderer::clear_projector_lights();
    }
    
    // Debug: Print shadow caster count once
    static bool shadow_debug_printed = false;
    if (!shadow_debug_printed && num_shadow_casters > 0) {
        printf("[SHADOW] Collected %u shadow casters:\n", num_shadow_casters);
        printf("[SHADOW] Starting position dump...\n");
        for (uint32_t i = 0; i < num_shadow_casters; i++) {
            const auto& sc = shadow_casters[i];
            printf("[SHADOW]   [%u] pos=(%.3f,%.3f,%.3f) size=(%.3f,%.3f) tex=%u alpha=%.2f\n",
                   i, sc.position.x, sc.position.y, sc.position.z,
                   sc.size.x, sc.size.y, sc.texture, sc.alpha_threshold);
        }
        printf("[SHADOW] Done.\n");
        fflush(stdout);
        shadow_debug_printed = true;
    }
    
    // =========================================================================
    // OFFSCREEN RENDERING: Render scene at base resolution (320x180)
    // =========================================================================
    Renderer::begin_render_to_framebuffer();
    Renderer::clear_screen();
    
    // Background - use BACKGROUND layer for depth (lit rendering WITH shadows)
    Renderer::render_sprite_lit_shadowed(g_state.scene.background, 
                           Vec3(0.0f, 0.0f, Layers::get_z_depth(Layer::BACKGROUND)),
                           Vec2((float)g_state.scene.width, (float)g_state.scene.height),
                           light_data, num_lights,
                           shadow_data, num_shadow_casters,
                           g_state.scene.background_normal_map);
    
    // =========================================================================
    // Render props using ECS components (from current scene)
    // Props can receive shadows from other props (but not from themselves)
    // =========================================================================
    static bool props_printed = false;
    int32_t prop_render_index = 0;
    for (ECS::EntityID prop_entity : g_state.scene.prop_entities) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(prop_entity);
        auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(prop_entity);
        
        if (!transform || !sprite || !sprite->visible) {
            prop_render_index++;
            continue;
        }
        
        // Debug: print prop z_depth once
        if (!props_printed) {
            printf("Prop z_depth: %.2f (pos: %.0f, %.0f)\n", 
                   transform->z_depth, transform->position.x, transform->position.y);
        }
        
        // Calculate depth scaling from ECS transform
        float depth_scale = ECS::TransformHelpers::compute_depth_scale(transform->z_depth);
        Vec2 scaled_size = Vec2(
            sprite->base_size.x * transform->scale.x * depth_scale,
            sprite->base_size.y * transform->scale.y * depth_scale
        );
        
        // Create 3D position for rendering (pixel coords + z_depth)
        Vec3 render_pos(transform->position.x, transform->position.y, transform->z_depth);
        
        // Render prop with lighting AND shadows - pass entity index to skip self-shadowing
        Renderer::render_sprite_lit_shadowed(sprite->texture, render_pos, scaled_size,
                                            light_data, num_lights,
                                            shadow_data, num_shadow_casters,
                                            sprite->normal_map, sprite->pivot,
                                            transform->z_depth, prop_render_index);
        prop_render_index++;
    }
    props_printed = true;
    
    // =========================================================================
    // Render player using ECS components
    // Player receives shadows from props but doesn't shadow itself
    // =========================================================================
    if (g_state.player_entity != ECS::INVALID_ENTITY) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(g_state.player_entity);
        auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(g_state.player_entity);
        
        if (transform && sprite && sprite->visible) {
            // Calculate depth scaling
            float depth_scale = ECS::TransformHelpers::compute_depth_scale(transform->z_depth);
            Vec2 scaled_size = Vec2(
                sprite->base_size.x * transform->scale.x * depth_scale,
                sprite->base_size.y * transform->scale.y * depth_scale
            );
            
            // Create 3D position for rendering
            Vec3 render_pos(transform->position.x, transform->position.y, transform->z_depth);
            
            // Player's entity index for self-shadow skip (after all props)
            int32_t player_entity_index = (int32_t)g_state.scene.prop_entities.size();
            
            // Render animated sprite if animation is set (with lighting and shadows)
            if (sprite->is_animated() && sprite->animation) {
                Renderer::render_sprite_animated_lit_shadowed(sprite->animation, 
                                                render_pos, 
                                                scaled_size,
                                                light_data, num_lights,
                                                shadow_data, num_shadow_casters,
                                                sprite->normal_map,
                                                sprite->pivot,
                                                transform->z_depth,
                                                player_entity_index);
            } else {
                Renderer::render_sprite_lit_shadowed(sprite->texture, render_pos, scaled_size,
                                           light_data, num_lights, 
                                           shadow_data, num_shadow_casters,
                                           sprite->normal_map, sprite->pivot,
                                           transform->z_depth, player_entity_index);
            }
        }
    }
    
    Renderer::end_render_to_framebuffer();
    
    // =========================================================================
    // UPSCALE: Blit framebuffer to screen at viewport resolution (1280x720)
    // =========================================================================
    Renderer::clear_screen();
    Renderer::render_framebuffer_to_screen();
    
    // Debug overlay (rendered at viewport resolution, after upscaling)
    Vec2 mouse_pixel = Platform::get_mouse_pos();
#ifndef NDEBUG
    Debug::render_overlay(mouse_pixel);
#endif
}

void shutdown() {
    // Cleanup happens in Renderer::shutdown() and asset cleanup
    // Player animations cleaned up through Core::AnimationBank destructor
}

}

