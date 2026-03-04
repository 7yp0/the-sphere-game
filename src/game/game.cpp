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
    
    // Initialize player in window center
    g_state.player.position = Vec2(512.0f, 384.0f);        // Center of 1024x768
    g_state.player.target_position = Vec2(512.0f, 384.0f);
    g_state.player.speed = 300.0f;                         // pixels per second
    g_state.player.texture = g_state.blueTex;
}

void set_viewport(uint32_t width, uint32_t height) {
    g_state.viewport_width = width;
    g_state.viewport_height = height;
}

// Convert from pixel space (window coords) to render space (normalized [-1, 1])
static Vec2 pixel_to_render_space(Vec2 pixel_pos) {
    float norm_x = (pixel_pos.x / (float)g_state.viewport_width) * 2.0f - 1.0f;
    float norm_y = (pixel_pos.y / (float)g_state.viewport_height) * 2.0f - 1.0f;
    return Vec2(norm_x, norm_y);
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
    
    // Check for mouse clicks (already in pixel coordinates)
    if (Platform::mouse_clicked()) {
        g_state.player.target_position = Platform::get_mouse_pos();
    }
    
    // Move player towards target (all in pixel space)
    Vec2 direction = Vec2(
        g_state.player.target_position.x - g_state.player.position.x,
        g_state.player.target_position.y - g_state.player.position.y
    );
    
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    
    if (distance > 1.0f) {  // 1 pixel threshold
        // Normalize direction
        direction.x /= distance;
        direction.y /= distance;
        
        // Move towards target (speed in pixels per second)
        Vec2 movement = Vec2(direction.x * g_state.player.speed * delta_time,
                            direction.y * g_state.player.speed * delta_time);
        
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
    
    // Render player (convert from pixel to render space)
    Renderer::render_sprite(g_state.player.texture, 
                           pixel_to_render_space(g_state.player.position), 
                           Vec2(0.1f, 0.1f),
                           Layers::get_z_depth(Layer::PLAYER));
}

void shutdown() {
}

}

