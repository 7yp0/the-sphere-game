#include "debug.h"
#include "renderer/text.h"
#include "renderer/renderer.h"
#include "types.h"
#include "config.h"
#include "game/game.h"
#include "platform.h"
#include "scene/scene.h"
#include "collision/polygon_utils.h"
#include "core/timing.h"
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
    
    // Scale factor from base (320x180) to viewport (1280x720)
    float scale_x = (float)Config::VIEWPORT_WIDTH / (float)Config::BASE_WIDTH;
    float scale_y = (float)Config::VIEWPORT_HEIGHT / (float)Config::BASE_HEIGHT;
    
    // Draw walkable areas (green)
    Vec4 walkable_color = Vec4(0.0f, 1.0f, 0.0f, 0.7f);
    float ui_z = Layers::get_z_depth(Layer::UI);
    for (const auto& walkable : scene.geometry.walkable_areas) {
        size_t n = walkable.points.size();
        for (size_t i = 0; i < n; i++) {
            // Scale from base to viewport coordinates
            Vec3 p1 = Vec3(walkable.points[i].x * scale_x, walkable.points[i].y * scale_y, ui_z);
            Vec3 p2 = Vec3(walkable.points[(i + 1) % n].x * scale_x, walkable.points[(i + 1) % n].y * scale_y, ui_z);
            
            // Draw edge as a line
            Renderer::render_line(p1, p2, walkable_color, 2.0f);
            
            // Draw vertex as a small circle
            Renderer::render_rect(p1, Vec2(6.0f, 6.0f), walkable_color);
        }
    }
    
    // Draw hotspots (red)
    Vec4 hotspot_color = Vec4(1.0f, 0.0f, 0.0f, 0.7f);
    for (const auto& hotspot : scene.geometry.hotspots) {
        size_t n = hotspot.bounds.points.size();
        for (size_t i = 0; i < n; i++) {
            // Scale from base to viewport coordinates
            Vec3 p1 = Vec3(hotspot.bounds.points[i].x * scale_x, hotspot.bounds.points[i].y * scale_y, ui_z);
            Vec3 p2 = Vec3(hotspot.bounds.points[(i + 1) % n].x * scale_x, hotspot.bounds.points[(i + 1) % n].y * scale_y, ui_z);
            
            Renderer::render_line(p1, p2, hotspot_color, 2.0f);
            Renderer::render_rect(p1, Vec2(6.0f, 6.0f), hotspot_color);
        }
    }
}

void render_overlay(Vec2 mouse_pixel) {
    if (overlay_enabled) {
        // Draw geometry first (behind text)
        render_geometry_debug();
        
        // Calculate FPS from delta time
        float dt = Core::get_delta_time();
        float fps = (dt > 0.0001f) ? (1.0f / dt) : 0.0f;
        float ms = dt * 1000.0f;
        
        // Get entity counts
        size_t prop_count = Game::g_state.scene.prop_entities.size();
        size_t light_count = Game::g_state.scene.light_entities.size();
        
        // Build debug text
        char text_buffer[512];
        snprintf(text_buffer, sizeof(text_buffer), 
                 "Mouse: (%.0f, %.0f)\n"
                 "Frame: %.2f ms (%.0f FPS)\n"
                 "Props: %zu  Lights: %zu",
                 mouse_pixel.x, mouse_pixel.y,
                 ms, fps,
                 prop_count, light_count);
        
        // Black semi-transparent background - pixel coordinates, top-left
        Vec3 bg_pos = Vec3(0.0f, 0.0f, Layers::get_z_depth(Layer::UI));
        Vec2 bg_size = Vec2(260.0f, 65.0f);  // Taller for more lines
        Vec4 bg_color = Vec4(0.0f, 0.0f, 0.0f, 0.7f);
        Renderer::render_rect(bg_pos, bg_size, bg_color);
        
        // Text on top - pixel coordinates
        Vec2 text_pos = Vec2(10.0f, 10.0f);
        Renderer::render_text(text_buffer, text_pos, 1.0f);
    }
}
}
