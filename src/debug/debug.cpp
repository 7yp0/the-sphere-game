#include "debug.h"
#include "renderer/text.h"
#include "renderer/renderer.h"
#include "types.h"
#include <cstdio>
#include <cstring>

namespace Debug {

bool overlay_enabled = false;

void toggle_overlay() {
  overlay_enabled = !overlay_enabled;
}

void render_overlay(Vec2 mouse_pixel, uint32_t viewport_width, uint32_t viewport_height) {
    if (overlay_enabled) {
        char text_buffer[256];
        snprintf(text_buffer, sizeof(text_buffer), "Mouse: (%.0f, %.0f)", mouse_pixel.x, mouse_pixel.y);
        
        // Black semi-transparent background
        Vec2 bg_pos = Vec2(-0.8f, 0.88f);
        Vec2 bg_size = Vec2(1.6f, 0.2f);
        Vec4 bg_color = Vec4(0.0f, 0.0f, 0.0f, 0.7f);  // Black with 70% opacity
        Renderer::render_rect(bg_pos, bg_size, bg_color, -0.99f);
        
        // Text on top
        Vec2 text_pos = Vec2(-0.95f, 0.9f);
        Renderer::render_text(text_buffer, text_pos, 0.4f);
    }
}

}
