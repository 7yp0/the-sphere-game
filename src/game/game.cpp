#include "game.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "renderer/animation.h"
#include "types.h"

namespace Game {

struct GameState {
    Renderer::TextureID blueTex;
    Renderer::TextureID redTex;
    Renderer::TextureID greenTex;
    
    Renderer::SpriteAnimation anim;
    Renderer::SpriteAnimation reverseAnim;
};

static GameState g_state;

void init() {
    g_state.blueTex = Renderer::load_texture("test.png");
    g_state.redTex = Renderer::load_texture("test_red.png");
    g_state.greenTex = Renderer::load_texture("test_green.png");

    Renderer::TextureID animFrames[] = { g_state.redTex, g_state.greenTex, g_state.blueTex };
    g_state.anim = Renderer::create_animation(animFrames, 3, 0.5f);

    Renderer::TextureID reverseFrames[] = { g_state.blueTex, g_state.greenTex, g_state.redTex };
    g_state.reverseAnim = Renderer::create_animation(reverseFrames, 3, 0.5f);
}

void update(float delta_time) {
    // TODO: add game logic (player movement, input, collisions)
}

void visual_update(float delta_time) {
    Renderer::animate(&g_state.anim, delta_time);
    Renderer::animate(&g_state.reverseAnim, delta_time);
}

void render() {
    Renderer::render_sprite_animated(&g_state.anim, Vec2(0.0f, 0.0f), Vec2(0.2f, 0.2f), 
                                      Layers::get_z_depth(Layer::BACKGROUND));
    
    Renderer::render_sprite(g_state.redTex, Vec2(-0.5f, 0.5f), Vec2(0.15f, 0.15f), 
                           Layers::get_z_depth(Layer::MIDGROUND));
    
    Renderer::render_sprite(g_state.greenTex, Vec2(0.5f, 0.5f), Vec2(0.1f, 0.1f), 
                           Layers::get_z_depth(Layer::MIDGROUND));
    
    Renderer::render_sprite(g_state.blueTex, Vec2(-0.5f, -0.5f), Vec2(0.25f, 0.25f), 
                           Layers::get_z_depth(Layer::MIDGROUND));
    
    Renderer::render_sprite_animated(&g_state.reverseAnim, Vec2(0.5f, -0.5f), Vec2(0.18f, 0.18f), 
                                     Layers::get_z_depth(Layer::UI));
}

void shutdown() {
}

}

