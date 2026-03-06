#include "debug.h"
#include "renderer/text.h"
#include "renderer/renderer.h"
#include "types.h"
#include "game/game.h"
#include "platform.h"
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

void render_overlay(Vec2 mouse_pixel) {
    if (overlay_enabled) {
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
