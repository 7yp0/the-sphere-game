#include "cursor.h"
#include "renderer/texture_loader.h"
#include "renderer/text.h"
#include "collision/polygon_utils.h"
#include "game/game.h"
#include "config.h"
#include "platform.h"
#include <cstring>

namespace UI {

static CursorSystem g_cursor;

// Forward declarations
static void render_tooltip(Vec2 mouse_pos, const char* text);
static float calculate_text_width(const char* text, float scale);

void init_cursor() {
    if (g_cursor.initialized) return;
    
    // Load cursor textures
    g_cursor.tex_default = Renderer::load_texture("cursors/crosshair.png");
    g_cursor.tex_hover = Renderer::load_texture("cursors/crosshair_hover.png");
    
    g_cursor.state = CursorState::Default;
    g_cursor.tooltip_text.clear();
    g_cursor.initialized = true;
    
    // Hide system cursor
    Platform::show_system_cursor(false);
}

void shutdown_cursor() {
    // Show system cursor again
    Platform::show_system_cursor(true);
    
    g_cursor.initialized = false;
    g_cursor.tex_default = 0;
    g_cursor.tex_hover = 0;
}

void update_cursor(Vec2 mouse_pos) {
    if (!g_cursor.initialized) return;
    
    // Convert viewport mouse position to base resolution for hotspot detection
    float scale_x = (float)Config::BASE_WIDTH / (float)Config::VIEWPORT_WIDTH;
    float scale_y = (float)Config::BASE_HEIGHT / (float)Config::VIEWPORT_HEIGHT;
    Vec2 mouse_base = Vec2(mouse_pos.x * scale_x, mouse_pos.y * scale_y);
    
    // Check hotspots for hover
    g_cursor.state = CursorState::Default;
    g_cursor.tooltip_text.clear();
    
    const auto& hotspots = Game::g_state.scene.geometry.hotspots;
    for (const auto& hotspot : hotspots) {
        if (!hotspot.enabled) continue;
        
        if (Collision::point_in_polygon(mouse_base, hotspot.bounds)) {
            g_cursor.state = CursorState::Hover;
            // Use tooltip_key if set, otherwise fall back to name
            g_cursor.tooltip_text = hotspot.tooltip_key.empty() ? hotspot.name : hotspot.tooltip_key;
            break;  // First match wins
        }
    }
}

// Calculate text width in pixels (simplified - assumes fixed width font)
static float calculate_text_width(const char* text, float scale) {
    if (!text) return 0.0f;
    
    float glyph_width = 12.0f * scale;   // From text.cpp
    float spacing = glyph_width * 1.1f;
    
    int char_count = 0;
    for (const char* p = text; *p; p++) {
        if (*p != '\n') char_count++;
    }
    
    return char_count * spacing;
}

void render_cursor(Vec2 mouse_pos) {
    if (!g_cursor.initialized) return;
    
    // Select cursor texture based on state
    Renderer::TextureID cursor_tex = (g_cursor.state == CursorState::Hover) 
                                     ? g_cursor.tex_hover 
                                     : g_cursor.tex_default;
    
    // Render cursor sprite at mouse position (centered on cursor)
    // Use CENTER pivot so cursor hotspot is at mouse position
    Vec3 cursor_pos = Vec3(mouse_pos.x, mouse_pos.y, ZDepth::CURSOR);
    Renderer::render_sprite(cursor_tex, cursor_pos, g_cursor.cursor_size, PivotPoint::CENTER);
    
    // Render tooltip if we have text
    if (!g_cursor.tooltip_text.empty()) {
        render_tooltip(mouse_pos, g_cursor.tooltip_text.c_str());
    }
}

static void render_tooltip(Vec2 mouse_pos, const char* text) {
    if (!text || text[0] == '\0') return;
    
    float scale = 1.0f;
    float text_width = calculate_text_width(text, scale);
    float text_height = 16.0f * scale;  // Single line height
    float padding = g_cursor.tooltip_padding;
    
    // Calculate tooltip position (smart positioning)
    float tooltip_x = mouse_pos.x + g_cursor.tooltip_offset.x;
    float tooltip_y = mouse_pos.y + g_cursor.tooltip_offset.y;
    
    float bg_width = text_width + padding * 2;
    float bg_height = text_height + padding * 2;
    
    // Smart positioning: keep tooltip on screen
    float right_edge = tooltip_x + bg_width;
    float bottom_edge = tooltip_y + bg_height;
    
    // If tooltip would go off right edge, flip to left of cursor
    if (right_edge > Config::VIEWPORT_WIDTH) {
        tooltip_x = mouse_pos.x - g_cursor.tooltip_offset.x - bg_width;
    }
    
    // If tooltip would go off bottom edge, flip to above cursor
    if (bottom_edge > Config::VIEWPORT_HEIGHT) {
        tooltip_y = mouse_pos.y - g_cursor.tooltip_offset.y - bg_height;
    }
    
    // Clamp to screen bounds (failsafe)
    if (tooltip_x < 0) tooltip_x = 0;
    if (tooltip_y < 0) tooltip_y = 0;
    
    // Render background (semi-transparent black)
    Vec3 bg_pos = Vec3(tooltip_x, tooltip_y, ZDepth::TOOLTIP);
    Vec4 bg_color = Vec4(0.0f, 0.0f, 0.0f, 0.8f);
    Renderer::render_rect(bg_pos, Vec2(bg_width, bg_height), bg_color);
    
    // Render text
    Vec2 text_pos = Vec2(tooltip_x + padding, tooltip_y + padding);
    Renderer::render_text(text, text_pos, scale);
}

void set_cursor_state(CursorState state) {
    g_cursor.state = state;
}

void set_tooltip(const std::string& text) {
    g_cursor.tooltip_text = text;
}

CursorState get_cursor_state() {
    return g_cursor.state;
}

}
