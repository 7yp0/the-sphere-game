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
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstdio>

namespace Game {

GameState g_state;

static void init_player() {
    player_init(g_state.player, g_state.viewport_width, g_state.viewport_height, 
                &g_state.playerAnimations);
}

void init() {
    Scene::init_scene_test();
    init_player();
}

void set_viewport(uint32_t width, uint32_t height) {
    g_state.viewport_width = width;
    g_state.viewport_height = height;
    
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
    
    player_handle_input(g_state.player);
    player_update(g_state.player, g_state.viewport_width, g_state.viewport_height, delta_time);
}

void render() {
    // Background - use BACKGROUND layer for depth
    Renderer::render_sprite_lit(g_state.scene.background, 
                               Vec3(0.0f, 0.0f, Layers::get_z_depth(Layer::BACKGROUND)),
                               Vec2((float)g_state.scene.width, (float)g_state.scene.height),
                               g_state.scene.lights,
                               g_state.scene.background_normal_map);
    
    // Render all props with their z-depth from scene definition
    for (size_t i = 0; i < g_state.scene.props.size(); ++i) {
        const Scene::Prop& prop = g_state.scene.props[i];
        
        // Calculate depth scaling for 2.5D effect (based on height map at position)
        float depth_scale = Scene::get_depth_scaling(g_state.scene, prop.position.x, prop.position.y);
        Vec2 scaled_size = Vec2(
            prop.size.x * depth_scale,
            prop.size.y * depth_scale
        );
        
        // Use actual 3D position from scene (z-coordinate from scene definition)
        Renderer::render_sprite_lit(prop.texture, prop.position, scaled_size,
                                   g_state.scene.lights, prop.normal_map, prop.pivot);
    }
    
    // Render player with its actual 3D position from game state
    const char* anim_name = player_get_animation_name(g_state.player);
    Renderer::SpriteAnimation* player_anim = g_state.playerAnimations.get(anim_name);
    if (player_anim) {
        // Calculate depth scaling for 2.5D effect (based on height map at position)
        float depth_scale = Scene::get_depth_scaling(g_state.scene, g_state.player.position.x, g_state.player.position.y);
        Vec2 scaled_size = Vec2(
            g_state.player.size.x * depth_scale,
            g_state.player.size.y * depth_scale
        );
        
        // Use actual 3D position from game state (z-coordinate is set during initialization)
        Renderer::render_sprite_animated_lit(player_anim, 
                                            g_state.player.position, 
                                            scaled_size,
                                            g_state.scene.lights,
                                            0,  // normal_map: use default
                                            g_state.player.pivot);
    }
    
    // Debug overlay
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

