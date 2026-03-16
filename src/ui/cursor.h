#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include "ui.h"

#include <string>

namespace UI {

enum class CursorState {
    Default,    // Normal cursor
    Hover       // Over interactive hotspot
};

// Cursor configuration (BASE sizes in logical 320x180 pixels)
namespace CursorConfig {
    constexpr float BASE_CURSOR_SIZE       = 8.0f;
    constexpr float BASE_ITEM_CURSOR_SIZE  = 12.0f;
    constexpr float BASE_TOOLTIP_OFFSET_X  = 2.0f;
    constexpr float BASE_TOOLTIP_OFFSET_Y  = -16.0f;
    constexpr float BASE_TOOLTIP_PADDING   = UI::BASE_BACKDROP_PADDING;

    // Scaled accessors (multiply by UI_SCALE to get UI-FBO pixel values)
    inline float cursor_size()       { return BASE_CURSOR_SIZE      * UI::UI_SCALE(); }
    inline float item_cursor_size()  { return BASE_ITEM_CURSOR_SIZE * UI::UI_SCALE(); }
    inline float tooltip_offset_x()  { return BASE_TOOLTIP_OFFSET_X * UI::UI_SCALE(); }
    inline float tooltip_offset_y()  { return BASE_TOOLTIP_OFFSET_Y * UI::UI_SCALE(); }
    inline float tooltip_padding()   { return BASE_TOOLTIP_PADDING  * UI::UI_SCALE(); }
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
