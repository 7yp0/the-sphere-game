#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include "inventory_ui.h"  // For get_ui_scale()
#include <string>

namespace UI {

enum class CursorState {
    Default,    // Normal cursor
    Hover       // Over interactive hotspot
};

// Cursor configuration (BASE sizes at 1x scale)
namespace CursorConfig {
    constexpr float BASE_CURSOR_SIZE = 8.0f;        // Cursor size at 1x
    constexpr float BASE_ITEM_CURSOR_SIZE = 12.0f;  // Item on cursor size at 1x
    constexpr float BASE_TOOLTIP_OFFSET_X = 5.0f;   // Tooltip X offset at 1x
    constexpr float BASE_TOOLTIP_OFFSET_Y = 7.0f;   // Tooltip Y offset at 1x
    constexpr float BASE_TOOLTIP_PADDING = 1.5f;    // Tooltip padding at 1x
    
    // Scaled accessors
    inline float cursor_size() { return BASE_CURSOR_SIZE * get_ui_scale(); }
    inline float item_cursor_size() { return BASE_ITEM_CURSOR_SIZE * get_ui_scale(); }
    inline float tooltip_offset_x() { return BASE_TOOLTIP_OFFSET_X * get_ui_scale(); }
    inline float tooltip_offset_y() { return BASE_TOOLTIP_OFFSET_Y * get_ui_scale(); }
    inline float tooltip_padding() { return BASE_TOOLTIP_PADDING * get_ui_scale(); }
}

struct CursorSystem {
    // Textures
    Renderer::TextureID tex_default = 0;
    Renderer::TextureID tex_hover = 0;
    
    // State
    CursorState state = CursorState::Default;
    std::string tooltip_text;
    bool initialized = false;
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
