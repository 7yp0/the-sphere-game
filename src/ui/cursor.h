#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include <string>

namespace UI {

enum class CursorState {
    Default,    // Normal cursor
    Hover       // Over interactive hotspot
};

struct CursorSystem {
    // Textures
    Renderer::TextureID tex_default = 0;
    Renderer::TextureID tex_hover = 0;
    
    // State
    CursorState state = CursorState::Default;
    std::string tooltip_text;
    bool initialized = false;
    
    // Cursor dimensions (in viewport pixels)
    Vec2 cursor_size = Vec2(32.0f, 32.0f);
    
    // Tooltip settings
    Vec2 tooltip_offset = Vec2(20.0f, 28.0f);  // Offset from cursor position
    float tooltip_padding = 6.0f;              // Background padding around text
};

// Initialize cursor system (loads textures)
void init_cursor();

// Shutdown cursor system
void shutdown_cursor();

// Update cursor state based on hotspot hover
// mouse_pos: viewport pixel coordinates
void update_cursor(Vec2 mouse_pos);

// Render cursor and tooltip (call after upscale, at viewport resolution)
// mouse_pos: viewport pixel coordinates  
void render_cursor(Vec2 mouse_pos);

// Set cursor state directly (for external control)
void set_cursor_state(CursorState state);

// Set tooltip text (empty string = no tooltip)
void set_tooltip(const std::string& text);
void reset_cursor_state();  // Call once per frame before UI updates

// Get current cursor state
CursorState get_cursor_state();

}
