#include "debug.h"
#include "renderer/text.h"
#include "renderer/renderer.h"
#include "types.h"
#include "game/game.h"
#include "platform.h"
#include "scene/scene.h"
#include "collision/polygon_utils.h"
#include <cstdio>
#include <cstring>

namespace Debug {

bool overlay_enabled = false;

void toggle_overlay() {
  overlay_enabled = !overlay_enabled;
}

void handle_debug_keys() {
#ifndef NDEBUG
    // D key: macOS = 2, Windows = 0x44 (68)
    #ifdef __APPLE__
        const int KEY_D = 2;  // macOS virtual key code for D
    #else
        const int KEY_D = 0x44;  // Windows/Linux virtual key code for D
    #endif
    
    bool key_d = Platform::key_pressed(KEY_D);
    static bool prev_d = false;
    
    if (key_d && !prev_d) {
        toggle_overlay();
    }
    prev_d = key_d;
#endif
}

static void render_geometry_debug() {
    const Scene::Scene& scene = Game::g_state.scene;
    
    // Draw walkable areas (green)
    Vec4 walkable_color = Vec4(0.0f, 1.0f, 0.0f, 0.7f);
    for (const auto& walkable : scene.geometry.walkable_areas) {
        size_t n = walkable.points.size();
        for (size_t i = 0; i < n; i++) {
            Vec2 p1 = walkable.points[i];
            Vec2 p2 = walkable.points[(i + 1) % n];
            
            // Draw edge as a line
            Renderer::render_line(p1, p2, walkable_color, 2.0f, Layers::get_z_depth(Layer::UI));
            
            // Draw vertex as a small circle
            Renderer::render_rect(p1, Vec2(6.0f, 6.0f), walkable_color, Layers::get_z_depth(Layer::UI));
        }
    }
    
    // Draw hotspots (red)
    Vec4 hotspot_color = Vec4(1.0f, 0.0f, 0.0f, 0.7f);
    for (const auto& hotspot : scene.geometry.hotspots) {
        size_t n = hotspot.bounds.points.size();
        for (size_t i = 0; i < n; i++) {
            Vec2 p1 = hotspot.bounds.points[i];
            Vec2 p2 = hotspot.bounds.points[(i + 1) % n];
            
            Renderer::render_line(p1, p2, hotspot_color, 2.0f, Layers::get_z_depth(Layer::UI));
            Renderer::render_rect(p1, Vec2(6.0f, 6.0f), hotspot_color, Layers::get_z_depth(Layer::UI));
        }
    }
}

void render_overlay(Vec2 mouse_pixel) {
    if (overlay_enabled) {
        // Draw geometry first (behind text)
        render_geometry_debug();
        
        char text_buffer[256];
        snprintf(text_buffer, sizeof(text_buffer), "Mouse: (%.0f, %.0f)", mouse_pixel.x, mouse_pixel.y);
        
        // Black semi-transparent background - pixel coordinates, top-left
        Vec2 bg_pos = Vec2(0.0f, 0.0f);
        Vec2 bg_size = Vec2(245.0f, 35.0f);
        Vec4 bg_color = Vec4(0.0f, 0.0f, 0.0f, 0.7f);
        Renderer::render_rect(bg_pos, bg_size, bg_color, Layers::get_z_depth(Layer::UI));
        
        // Text on top - pixel coordinates
        Vec2 text_pos = Vec2(10.0f, 10.0f);
        Renderer::render_text(text_buffer, text_pos, 1.0f);
    }
}
}
