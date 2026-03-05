#include "game.h"
#include "player.h"
#include "core/animation_bank.h"
#include "core/timing.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "renderer/animation.h"
#include "platform/mac/mac_window.h"
#include "scene/scene.h"
#include "debug/debug.h"
#include "types.h"
#include <cmath>
#include <cstdio>

namespace Game {

GameState g_state;
#ifndef NDEBUG
static bool g_prev_key_d = false;
#endif

static void init_player() {
    Core::animation_bank_load_player(g_state.playerAnimations);
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
    // Toggle debug overlay on D key press (with edge detection)
    bool key_d = Platform::key_pressed(2);
    if (key_d && !g_prev_key_d) {
        Debug::toggle_overlay();
    }
    g_prev_key_d = key_d;
#endif
    
    player_handle_input(g_state.player);
    player_update(g_state.player, g_state.viewport_width, g_state.viewport_height, delta_time);
}

void render() {
    Renderer::render_sprite(g_state.scene.background, Vec2(0.0f, 0.0f), Vec2(2.0f, 2.0f),
                           Layers::get_z_depth(Layer::BACKGROUND));
    
    for (const auto& prop : g_state.scene.props) {
        Vec2 opengl_pos = Coords::pixel_to_opengl(prop.position, g_state.viewport_width, g_state.viewport_height);
        Vec2 opengl_size = Vec2(
            (prop.size.x / (float)g_state.viewport_width) * 2.0f,
            (prop.size.y / (float)g_state.viewport_height) * 2.0f
        );
        Renderer::render_sprite(prop.texture, opengl_pos, opengl_size,
                               Layers::get_z_depth(Layer::MIDGROUND));
    }
    
    Vec2 player_render_pos = player_get_render_position(g_state.player, g_state.viewport_width, g_state.viewport_height);
    const char* anim_name = (g_state.player.animation_state == AnimationState::Idle) ? "idle" : "walk";
    Renderer::SpriteAnimation* player_anim = g_state.playerAnimations.get(anim_name);
    if (player_anim) {
        Renderer::render_sprite_animated(player_anim, player_render_pos, Vec2(0.1f, 0.1f),
                                         Layers::get_z_depth(Layer::PLAYER));
    }
    
    Vec2 mouse_pixel = Platform::get_mouse_pos();

    #ifndef NDEBUG
    Debug::render_overlay(mouse_pixel, g_state.viewport_width, g_state.viewport_height);
#endif
}

void shutdown() {
}

}

