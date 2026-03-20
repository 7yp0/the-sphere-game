#include "game.h"
#include "player.h"
#include "animation_bank.h"
#include "core/timing.h"
#include "core/localization.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "renderer/animation.h"
#include "platform.h"
#include "scene/scene.h"
#include "scene/scene_registry.h"
#include "scene/act_registry.h"
#include "scene/close_up.h"
#include "save/save_system.h"
#include "core/settings.h"
#include "debug/debug.h"
#include "ui/cursor.h"
#include "ui/inventory_ui.h"
#include "ui/main_menu.h"
#include "ui/speechbubble_ui.h"
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

void init() {
    // Initialize settings (loads settings.json, must be before Localization)
    Settings::init();

    // Initialize save system
    SaveSystem::init();

    // Load language from settings
    Localization::load(Settings::get_language());

    // Initialize inventory UI FIRST (so scene can register items)
    UI::init_inventory_ui();

    // Initialize cursor system
    UI::init_cursor();

    // Initialize main menu (checks for existing save to enable Continue)
    UI::init_main_menu();

    // Register all scenes and acts, then start Act 1.
    Scene::register_all_scenes();
    Scene::register_all_acts();
    Scene::load_act(1);

    // Initialize framebuffer for offscreen rendering at base resolution
    Renderer::init_framebuffer(Config::BASE_WIDTH, Config::BASE_HEIGHT);
    // Initialize UI framebuffer at letterbox size.
    // render_framebuffer_to_screen() will reinit it to the correct letterbox on each frame
    // so this just needs a valid non-zero placeholder.
    Renderer::init_ui_framebuffer(Config::VIEWPORT_WIDTH, Config::VIEWPORT_HEIGHT);
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
    if (g_state.mode == GameMode::GAMEPLAY)
        SaveSystem::update(delta_time);
    
#ifndef NDEBUG
    Debug::handle_debug_keys();
#endif
    
    // Clear tooltip at start of frame (before any UI updates)
    UI::reset_cursor_state();

    // Convert window mouse position to UI-FBO coordinates (handles letterboxing)
    Vec2 mouse_pos = Renderer::window_to_ui_coords(Platform::get_mouse_pos());

    // Main menu captures all input while active
    if (g_state.mode == GameMode::MAIN_MENU) {
        UI::update_main_menu(mouse_pos);
        // Keep player animation up-to-date so it renders correctly in the background
        update_player_sprite_animation();
        return;
    }

    // ESC opens the main menu — only from GAMEPLAY, not while debug overlay is active
    if (g_state.mode == GameMode::GAMEPLAY) {
#ifdef __APPLE__
        static constexpr int KEY_ESC = 53;
#else
        static constexpr int KEY_ESC = 0x1B;
#endif
        static bool prev_esc = false;
        bool esc = Platform::key_pressed(KEY_ESC);
        if (esc && !prev_esc) {
            UI::set_can_resume(true);
            UI::mark_just_opened();
            g_state.mode = GameMode::MAIN_MENU;
        }
        prev_esc = esc;
    }

    // Update inventory UI (may consume input) — suppressed in close-up unless config allows it,
    // and suppressed while debug overlay is active
    bool ui_consumed_input = false;
#ifndef NDEBUG
    if (!Debug::overlay_enabled)
#endif
    if (g_state.mode != GameMode::CLOSE_UP || Scene::get_close_up_config().show_inventory) {
        ui_consumed_input = UI::update_inventory_ui(mouse_pos);
    }

    // Get player transform and walker from ECS
    ECS::Transform2_5DComponent* player_transform = get_player_transform();
    ECS::WalkerComponent* player_walker = get_player_walker();
    // Close-up puzzle update (before player input so puzzle can consume mouse events)
    if (g_state.mode == GameMode::CLOSE_UP) {
        const auto& cu_cfg = Scene::get_close_up_config();
        if (cu_cfg.on_update) {
            Vec2 mouse_viewport = Platform::get_mouse_pos();
            Vec2 mouse_ui = Renderer::window_to_ui_coords(mouse_viewport);
            Vec2 mouse_base(
                mouse_ui.x * (float)Config::BASE_WIDTH  / (float)Renderer::get_ui_fbo_width(),
                mouse_ui.y * (float)Config::BASE_HEIGHT / (float)Renderer::get_ui_fbo_height()
            );
            cu_cfg.on_update(mouse_base);
        }
    }

    if (player_transform && player_walker) {
        if (g_state.mode == GameMode::CLOSE_UP) {
            // In close-up: hotspot clicks still work (via player_handle_input),
            // but the player is hidden so only IMMEDIATE hotspots make sense.
            // Walking is still computed (player is off-screen, harmless).
            if (!ui_consumed_input) {
                player_handle_input(g_state.player, *player_transform, *player_walker);
            }
            player_update(g_state.player, *player_transform, *player_walker,
                          g_state.base_width, g_state.base_height, delta_time);
        } else {
            // Only handle player input if UI didn't consume it
            if (!ui_consumed_input) {
                player_handle_input(g_state.player, *player_transform, *player_walker);
            }
            player_update(g_state.player, *player_transform, *player_walker,
                          g_state.base_width, g_state.base_height, delta_time);

            // Update sprite animation based on player state
            update_player_sprite_animation();
        }
    }
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
            
            // Calculate ground Z from depth map
            float ground_z = ECS::TransformHelpers::get_z_from_depth_map(
                g_state.scene.depth_map, transform->position.x, transform->position.y,
                g_state.scene.width, g_state.scene.height);
            
            // Calculate depth scale from GROUND Z (not z_depth)
            float depth_scale = ECS::TransformHelpers::compute_depth_scale(ground_z);
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
            
            // Apply elevation offset to shadow Y (shadow starts from elevated position)
            float elevation_opengl_y = (transform->elevation * 50.0f / (float)render_height) * 2.0f;
            
            // Shadow Z = ground level ONLY (Z-ordering stays at ground, only Y is affected by elevation)
            float shadow_z = ground_z;
            
            Renderer::ShadowCasterData sc;
            sc.position = Vec3(opengl_center.x, opengl_center.y + elevation_opengl_y, shadow_z);
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
            
            // Calculate ground Z from depth map
            float ground_z = ECS::TransformHelpers::get_z_from_depth_map(
                g_state.scene.depth_map, transform->position.x, transform->position.y,
                g_state.scene.width, g_state.scene.height);
            
            // Calculate depth scale from GROUND Z (not z_depth)
            float depth_scale = ECS::TransformHelpers::compute_depth_scale(ground_z);
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
            
            // Apply elevation offset to shadow Y (shadow starts from elevated position)
            float elevation_opengl_y = (transform->elevation * 50.0f / (float)render_height) * 2.0f;
            
            // Shadow Z = ground level ONLY (Z-ordering stays at ground, only Y is affected by elevation)
            float shadow_z = ground_z;
            
            Renderer::ShadowCasterData sc;
            sc.position = Vec3(opengl_center.x, opengl_center.y + elevation_opengl_y, shadow_z);
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
        auto* transform = g_state.ecs_world.get_component<ECS::Transform3DComponent>(proj_entity);
        auto* projector = g_state.ecs_world.get_component<ECS::ProjectorLightComponent>(proj_entity);
        
        if (!transform || !projector || !projector->enabled) continue;
        
        Renderer::ProjectorLightData pd;
        pd.position = transform->position;  // Already in OpenGL 3D coords
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
    
    // =========================================================================
    // OFFSCREEN RENDERING: Render scene at base resolution (320x180)
    // =========================================================================
    Renderer::begin_render_to_framebuffer();
    Renderer::clear_screen();
    
    // Background - use BACKGROUND depth (lit rendering WITH shadows)
    Renderer::render_sprite_lit_shadowed(g_state.scene.background, 
                           Vec3(0.0f, 0.0f, ZDepth::BACKGROUND),
                           Vec2((float)g_state.scene.width, (float)g_state.scene.height),
                           light_data, num_lights,
                           shadow_data, num_shadow_casters,
                           g_state.scene.background_normal_map);
    
    // =========================================================================
    // Render props using ECS components (from current scene)
    // Props can receive shadows from other props (but not from themselves)
    // =========================================================================
    int32_t prop_render_index = 0;
    for (ECS::EntityID prop_entity : g_state.scene.prop_entities) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(prop_entity);
        auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(prop_entity);
        
        if (!transform || !sprite || !sprite->visible) {
            prop_render_index++;
            continue;
        }
        
        // Calculate Z from depth map (ground level at this pixel)
        float ground_z = ECS::TransformHelpers::get_z_from_depth_map(
            g_state.scene.depth_map, transform->position.x, transform->position.y,
            g_state.scene.width, g_state.scene.height);
        
        // Calculate depth scaling from GROUND Z (not adjusted for elevation)
        float depth_scale = ECS::TransformHelpers::compute_depth_scale(ground_z);
        Vec2 scaled_size = Vec2(
            sprite->base_size.x * transform->scale.x * depth_scale,
            sprite->base_size.y * transform->scale.y * depth_scale
        );
        
        // Elevation moves object UP in 2D viewport: render_y -= elevation * pixels_per_unit
        // Keep render_z at ground_z for correct occlusion
        float elevation_pixels = transform->elevation * 50.0f;  // 50 pixels per elevation unit
        Vec3 render_pos(transform->position.x, transform->position.y - elevation_pixels, ground_z);
        
        // Z for shader shadow calculation: stay at ground_z (elevation doesn't change Z-ordering)
        float shadow_z = ground_z;
        
        // Render prop with lighting AND shadows - pass entity index to skip self-shadowing
        Renderer::render_sprite_lit_shadowed(sprite->texture, render_pos, scaled_size,
                                            light_data, num_lights,
                                            shadow_data, num_shadow_casters,
                                            sprite->normal_map, sprite->pivot,
                                            shadow_z, prop_render_index);
        prop_render_index++;
    }

    // =========================================================================
    // Render player using ECS components
    // Player receives shadows from props but doesn't shadow itself
    // Skipped in CLOSE_UP mode (player is off-screen / hidden)
    // =========================================================================
    if (g_state.mode != GameMode::CLOSE_UP && g_state.player_entity != ECS::INVALID_ENTITY) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(g_state.player_entity);
        auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(g_state.player_entity);
        
        if (transform && sprite && sprite->visible) {
            // Calculate Z from depth map (ground level at this pixel)
            float ground_z = ECS::TransformHelpers::get_z_from_depth_map(
                g_state.scene.depth_map, transform->position.x, transform->position.y,
                g_state.scene.width, g_state.scene.height);
            
            // Calculate depth scaling from GROUND Z (not adjusted for elevation)
            float depth_scale = ECS::TransformHelpers::compute_depth_scale(ground_z);
            Vec2 scaled_size = Vec2(
                sprite->base_size.x * transform->scale.x * depth_scale,
                sprite->base_size.y * transform->scale.y * depth_scale
            );
            
            // Elevation moves object UP in 2D viewport: render_y -= elevation * pixels_per_unit
            // Keep render_z at ground_z for correct occlusion
            float elevation_pixels = transform->elevation * 50.0f;  // 50 pixels per elevation unit
            Vec3 render_pos(transform->position.x, transform->position.y - elevation_pixels, ground_z);
            
            // Z for shader shadow calculation: stay at ground_z (elevation doesn't change Z-ordering)
            float shadow_z = ground_z;
            
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
                                                shadow_z,
                                                player_entity_index);
            } else {
                Renderer::render_sprite_lit_shadowed(sprite->texture, render_pos, scaled_size,
                                           light_data, num_lights, 
                                           shadow_data, num_shadow_casters,
                                           sprite->normal_map, sprite->pivot,
                                           shadow_z, player_entity_index);
            }
        }
    }
    
    // Foreground — in front of all props/player, hidden in close-up
    if (g_state.scene.foreground && g_state.mode != GameMode::CLOSE_UP) {
        Renderer::render_sprite_lit_shadowed(g_state.scene.foreground,
                               Vec3(0.0f, 0.0f, ZDepth::FOREGROUND),
                               Vec2((float)g_state.scene.width, (float)g_state.scene.height),
                               light_data, num_lights,
                               nullptr, 0,
                               g_state.scene.foreground_normal_map);
    }

    // Close-up puzzle render (inside FBO, at base resolution, on top of scene)
    if (g_state.mode == GameMode::CLOSE_UP) {
        const auto& cu_cfg = Scene::get_close_up_config();
        if (cu_cfg.on_render) cu_cfg.on_render();
    }

    Renderer::end_render_to_framebuffer();

    // =========================================================================
    // UPSCALE: Blit framebuffer to screen at viewport resolution (1280x720)
    // =========================================================================
    Renderer::clear_screen();
    Renderer::render_framebuffer_to_screen();

    // Debug overlay (rendered at viewport resolution, after upscaling)
    Vec2 mouse_window = Platform::get_mouse_pos();
#ifndef NDEBUG
    Debug::render_overlay(mouse_window);
#endif

    // Convert window mouse to UI-FBO coordinates for all UI rendering
    Vec2 mouse_ui = Renderer::window_to_ui_coords(mouse_window);

    // ================= UI-FBO: Gesamte UI in separates FBO zeichnen =================
    Renderer::begin_render_to_ui_framebuffer();
    if (g_state.mode == GameMode::MAIN_MENU) {
        // Main menu overlay: dims the scene, shows menu panel
        UI::render_main_menu();
    } else if (g_state.mode == GameMode::CLOSE_UP) {
        // Close-up: inventory only if config allows it; speech bubbles always
        if (Scene::get_close_up_config().show_inventory) {
            UI::render_inventory_ui();
        }
        UI::render_speechbubbles();
    } else {
        // Gameplay UI
#ifndef NDEBUG
        if (!Debug::overlay_enabled)
#endif
        UI::render_inventory_ui();
        UI::render_speechbubbles();
    }
    // Cursor always on top
    UI::update_cursor(mouse_ui);
    UI::render_cursor(mouse_ui);
    Renderer::end_render_to_ui_framebuffer();

    // Blit UI-FBO als Overlay auf das Fenster (volle Größe, kein Letterboxing)
    Renderer::render_ui_framebuffer_to_screen();
}

void set_flag(const std::string& key, bool value)        { g_state.flags[key] = value; SaveSystem::schedule_save(); }
bool get_flag(const std::string& key, bool default_val)  { auto it = g_state.flags.find(key); return it != g_state.flags.end() ? it->second : default_val; }

void set_value(const std::string& key, int value)        { g_state.values[key] = value; }
int  get_value(const std::string& key, int default_val)  { auto it = g_state.values.find(key); return it != g_state.values.end() ? it->second : default_val; }

void        set_string(const std::string& key, const std::string& value)                    { g_state.strings[key] = value; }
std::string get_string(const std::string& key, const std::string& default_val)              { auto it = g_state.strings.find(key); return it != g_state.strings.end() ? it->second : default_val; }

void start_new_game() {
    // load_act resets all state (flags, values, strings, scene_states, inventory)
    // and loads the act's starting scene.
    // Mode must be GAMEPLAY before the scene loads so the auto-save trigger fires
    // correctly on future scene changes — but we do NOT want a save right now,
    // so set mode after the act is loaded (load_scene only saves in GAMEPLAY mode).
    Scene::load_act(1);
    g_state.mode = GameMode::GAMEPLAY;
}

bool continue_game() {
    if (!SaveSystem::has_save()) return false;
    // load() restores all state and calls load_scene internally.
    // Mode is still MAIN_MENU during the restore, so no accidental save fires.
    SaveSystem::load();
    g_state.mode = GameMode::GAMEPLAY;
    return true;
}

void shutdown() {
    // Shutdown inventory UI
    UI::shutdown_inventory_ui();
    
    // Shutdown cursor system (restores system cursor)
    UI::shutdown_cursor();
    
    // Cleanup happens in Renderer::shutdown() and asset cleanup
    // Player animations cleaned up through Core::AnimationBank destructor
}

}

