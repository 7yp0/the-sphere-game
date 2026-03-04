#include "game.h"
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

void init() {
    g_state.blueTex = Renderer::load_texture("test.png");
    g_state.redTex = Renderer::load_texture("test_red.png");
    g_state.greenTex = Renderer::load_texture("test_green.png");

    Renderer::TextureID animFrames[] = { g_state.redTex, g_state.greenTex, g_state.blueTex };
    g_state.anim = Renderer::create_animation(animFrames, 3, 0.5f);

    Renderer::TextureID reverseFrames[] = { g_state.blueTex, g_state.greenTex, g_state.redTex };
    g_state.reverseAnim = Renderer::create_animation(reverseFrames, 3, 0.5f);
    
    // Initialize player
    g_state.player.position = Vec2(0.0f, 0.0f);
    g_state.player.target_position = Vec2(0.0f, 0.0f);
    g_state.player.speed = 200.0f;
    g_state.player.texture = g_state.blueTex;
}

void set_viewport(uint32_t width, uint32_t height) {
    g_state.viewport_width = width;
    g_state.viewport_height = height;
}

// Convert from mouse window space (pixels) to render space (normalized)
static Vec2 mouse_to_render_space(Vec2 mouse_pos) {
    // Window (0, 0) is TOP-LEFT in pixels
    // Render (0, 0) is CENTER of screen [-1, 1]
    // Y: window 0 (top) → render -1 (top), window 768 (bottom) → render 1 (bottom)
    
    float norm_x = (mouse_pos.x / (float)g_state.viewport_width) * 2.0f - 1.0f;
    float norm_y = (mouse_pos.y / (float)g_state.viewport_height) * 2.0f - 1.0f;
    
    return Vec2(norm_x, norm_y);
}

void update(float delta_time) {
    // Update animations
    Renderer::animate(&g_state.anim, delta_time);
    Renderer::animate(&g_state.reverseAnim, delta_time);
    
    // Check for mouse clicks
    if (Platform::mouse_clicked()) {
        Vec2 click_pos_pixels = Platform::get_mouse_pos();
        g_state.player.target_position = mouse_to_render_space(click_pos_pixels);
        printf("=== CLICK ===\n");
        printf("Window dimensions: %u x %u\n", g_state.viewport_width, g_state.viewport_height);
        printf("Mouse pixels: (%.0f, %.0f)\n", click_pos_pixels.x, click_pos_pixels.y);
        printf("Player target (render): (%.3f, %.3f)\n", 
               g_state.player.target_position.x, g_state.player.target_position.y);
        printf("Player current (render): (%.3f, %.3f)\n",
               g_state.player.position.x, g_state.player.position.y);
        printf("=============\n");
    }
    
    // Move player towards target
    Vec2 direction = Vec2(
        g_state.player.target_position.x - g_state.player.position.x,
        g_state.player.target_position.y - g_state.player.position.y
    );
    
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    
    if (distance > 0.01f) {  // Use smaller threshold for normalized space
        // Normalize direction
        direction.x /= distance;
        direction.y /= distance;
        
        // Move towards target (speed in normalized space units per second)
        Vec2 movement = Vec2(direction.x * g_state.player.speed * delta_time * 0.001f,
                            direction.y * g_state.player.speed * delta_time * 0.001f);
        
        g_state.player.position.x += movement.x;
        g_state.player.position.y += movement.y;
    }
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
    
    // Render player (normalized coordinates: center at 0.5, 0.5)
    Renderer::render_sprite(g_state.player.texture, g_state.player.position, Vec2(0.1f, 0.1f),
                           Layers::get_z_depth(Layer::PLAYER));
}

void shutdown() {
}

}

