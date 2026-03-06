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
#include <cstdio>
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
    // Background fills entire scene
    Renderer::render_sprite(g_state.scene.background, 
                           Vec2(0.0f, 0.0f),
                           Vec2((float)g_state.scene.width, (float)g_state.scene.height),
                           Layers::get_z_depth(Layer::BACKGROUND));
    
    // Props - use pixel coordinates directly
    for (const auto& prop : g_state.scene.props) {
        Renderer::render_sprite(prop.texture, prop.position, prop.size,
                               Layers::get_z_depth(Layer::MIDGROUND), prop.pivot);
    }
    
    // Player - use pixel coordinates
    const char* anim_name = (g_state.player.animation_state == AnimationState::Idle) ? "idle" : "walk";
    Renderer::SpriteAnimation* player_anim = g_state.playerAnimations.get(anim_name);
    if (player_anim) {
        // Player position and size from player struct
        Renderer::render_sprite_animated(player_anim, g_state.player.position, g_state.player.size,
                                         Layers::get_z_depth(Layer::PLAYER), g_state.player.pivot);
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

