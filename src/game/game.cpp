#include "game.h"
#include "player.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "renderer/animation.h"
#include "platform/mac/mac_window.h"
#include "types.h"
#include <cmath>
#include <cstdio>

namespace Game {

struct GameState {
    Renderer::TextureID blueTex;
    Renderer::TextureID redTex;
    Renderer::TextureID greenTex;
    
    Renderer::SpriteAnimation anim;
    Renderer::SpriteAnimation reverseAnim;
    
    Player player;
    
    uint32_t viewport_width = 1024;
    uint32_t viewport_height = 768;
};

static GameState g_state;

const Vec2 ANIM_POS = Vec2(0.0f, 0.0f);
const Vec2 ANIM_SIZE = Vec2(0.2f, 0.2f);
const Vec2 RED_POS = Vec2(-0.5f, 0.5f);
const Vec2 RED_SIZE = Vec2(0.15f, 0.15f);
const Vec2 GREEN_POS = Vec2(0.5f, 0.5f);
const Vec2 GREEN_SIZE = Vec2(0.1f, 0.1f);
const Vec2 BLUE_POS = Vec2(-0.5f, -0.5f);
const Vec2 BLUE_SIZE = Vec2(0.25f, 0.25f);
const Vec2 REVERSE_ANIM_POS = Vec2(0.5f, -0.5f);
const Vec2 REVERSE_ANIM_SIZE = Vec2(0.18f, 0.18f);
const Vec2 PLAYER_SIZE = Vec2(0.1f, 0.1f);


void init() {
    g_state.blueTex = Renderer::load_texture("test.png");
    g_state.redTex = Renderer::load_texture("test_red.png");
    g_state.greenTex = Renderer::load_texture("test_green.png");

    Renderer::TextureID animFrames[] = { g_state.redTex, g_state.greenTex, g_state.blueTex };
    g_state.anim = Renderer::create_animation(animFrames, 3, 0.5f);

    Renderer::TextureID reverseFrames[] = { g_state.blueTex, g_state.greenTex, g_state.redTex };
    g_state.reverseAnim = Renderer::create_animation(reverseFrames, 3, 0.5f);
    
    // Initialize player
    player_init(g_state.player, g_state.viewport_width, g_state.viewport_height, g_state.blueTex);
}

void set_viewport(uint32_t width, uint32_t height) {
    g_state.viewport_width = width;
    g_state.viewport_height = height;
}

// Update all animations
static void update_animations(float delta_time) {
    Renderer::animate(&g_state.anim, delta_time);
    Renderer::animate(&g_state.reverseAnim, delta_time);
}

void update(float delta_time) {
    update_animations(delta_time);
    player_handle_input(g_state.player);
    player_update(g_state.player, g_state.viewport_width, g_state.viewport_height, delta_time);
}

void render() {
    Renderer::render_sprite_animated(&g_state.anim, ANIM_POS, ANIM_SIZE, 
                                      Layers::get_z_depth(Layer::BACKGROUND));
    
    Renderer::render_sprite(g_state.redTex, RED_POS, RED_SIZE, 
                           Layers::get_z_depth(Layer::MIDGROUND));
    
    Renderer::render_sprite(g_state.greenTex, GREEN_POS, GREEN_SIZE, 
                           Layers::get_z_depth(Layer::MIDGROUND));
    
    Renderer::render_sprite(g_state.blueTex, BLUE_POS, BLUE_SIZE, 
                           Layers::get_z_depth(Layer::MIDGROUND));
    
    Renderer::render_sprite_animated(&g_state.reverseAnim, REVERSE_ANIM_POS, REVERSE_ANIM_SIZE, 
                                     Layers::get_z_depth(Layer::UI));
    
    // Render player
    Vec2 player_render_pos = player_get_render_position(g_state.player, g_state.viewport_width, g_state.viewport_height);
    Renderer::render_sprite(g_state.player.texture, 
                           player_render_pos, 
                           PLAYER_SIZE,
                           Layers::get_z_depth(Layer::PLAYER));
}

void shutdown() {
    // Cleanup animations and textures
    // Note: Renderer::clear_texture_cache() is called in engine.cpp
}

}

