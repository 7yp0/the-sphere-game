#include "game.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "renderer/animation.h"

namespace Game {

// Game state
struct GameState {
    // Test textures
    Renderer::TextureID blueTex;
    Renderer::TextureID redTex;
    Renderer::TextureID greenTex;
    
    // Test animations
    Renderer::SpriteAnimation anim;
    Renderer::SpriteAnimation reverseAnim;
};

static GameState g_state;

void init() {
    // Load test textures (different colors)
    g_state.blueTex = Renderer::load_texture("test.png");
    g_state.redTex = Renderer::load_texture("test_red.png");
    g_state.greenTex = Renderer::load_texture("test_green.png");

    // Create animation: red -> green -> blue cycle, 0.5s per frame
    Renderer::TextureID animFrames[] = { g_state.redTex, g_state.greenTex, g_state.blueTex };
    g_state.anim = Renderer::create_animation(animFrames, 3, 0.5f);

    // Create reverse animation: blue -> green -> red
    Renderer::TextureID reverseFrames[] = { g_state.blueTex, g_state.greenTex, g_state.redTex };
    g_state.reverseAnim = Renderer::create_animation(reverseFrames, 3, 0.5f);
}

void update(float delta_time) {
    // Game Logic (player movement, input, collisions, etc)
    // TODO: add game logic here
}

void visual_update(float delta_time) {
    // Visual Effects (animations, particles, etc)
    Renderer::animate(&g_state.anim, delta_time);
    Renderer::animate(&g_state.reverseAnim, delta_time);
}

void render() {
    // Animated sprite at center (cycles red -> green -> blue)
    Renderer::render_sprite_animated(&g_state.anim, Vec2(0.0f, 0.0f), Vec2(0.2f, 0.2f));
    
    // Static red sprite at top-left
    Renderer::render_sprite(g_state.redTex, Vec2(-0.5f, 0.5f), Vec2(0.15f, 0.15f));
    
    // Static green sprite at top-right
    Renderer::render_sprite(g_state.greenTex, Vec2(0.5f, 0.5f), Vec2(0.1f, 0.1f));
    
    // Static blue sprite at bottom-left
    Renderer::render_sprite(g_state.blueTex, Vec2(-0.5f, -0.5f), Vec2(0.25f, 0.25f));
    
    // Reverse animated sprite at bottom-right (cycles blue -> green -> red)
    Renderer::render_sprite_animated(&g_state.reverseAnim, Vec2(0.5f, -0.5f), Vec2(0.18f, 0.18f));
}

void shutdown() {
    // Nothing to cleanup yet (texture cache handled by Renderer)
}

}

