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

static void update_animated_test_light(float delta_time) {
    // Animate the first light horizontally (left to right) with fixed Y and Z
    // ALL COORDINATES IN OPENGL SPACE (-1 to +1):
    //   X = -1 (left) to +1 (right)
    //   Y = -1 (bottom) to +1 (top) - height affects shadow length
    //   Z = -1 (near camera) to +1 (far/background)
    static float light_time = 0.0f;
    light_time += delta_time;
    
    if (g_state.scene.lights.size() > 0) {
        float cycle_time = 6.0f;  // 6 seconds für einen Durchlauf
        float t = fmod(light_time, cycle_time) / cycle_time;  // 0 to 1
        
        // X bewegt sich von links (-1) nach rechts (+1) in OpenGL-Koordinaten
        float x = -1.0f + t * 2.0f;  // -1 to +1
        float y = 0.0f;   // Mitte (OpenGL Y: -1=unten, +1=oben)
        float z = 0.0f;  // Leicht vor der Szene (näher zur Kamera)
        
        g_state.scene.lights[0].position = Vec3(x, y, z);
        
        printf("[LIGHT TEST] Progress: %.2f - OpenGL Position: (%.2f, %.2f, %.2f)\n", t, x, y, z);
    }
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
    
    update_animated_test_light(delta_time);
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
        
        // Calculate depth scaling for 2.5D effect (based on depth map at position)
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
        // Calculate depth scaling for 2.5D effect (based on depth map at position)
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
    
    // Debug: Rotes Rechteck an der Lichtposition
    if (g_state.scene.lights.size() > 0) {
        Vec3 light_pos = g_state.scene.lights[0].position;
        // Konvertiere OpenGL-Koordinaten (-1 bis +1) zu Pixel-Koordinaten
        // OpenGL X: -1=links, +1=rechts → Pixel: 0 bis viewport_width
        // OpenGL Y: -1=unten, +1=oben → Pixel: viewport_height bis 0 (invertiert!)
        float pixel_x = (light_pos.x + 1.0f) * 0.5f * g_state.viewport_width;
        float pixel_y = (1.0f - light_pos.y) * 0.5f * g_state.viewport_height;
        
        // Zeichne rotes Rect (10x10 Pixel) zentriert auf der Lichtposition
        Renderer::render_rect(Vec3(pixel_x - 5.0f, pixel_y - 5.0f, Layers::get_z_depth(Layer::UI)),
                             Vec2(10.0f, 10.0f),
                             Vec4(1.0f, 0.0f, 0.0f, 1.0f));  // Rot
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

