#include "cursor.h"
#include "inventory_ui.h"
#include "inventory/inventory.h"
#include "renderer/texture_loader.h"

#include "renderer/text.h"
#include "collision/polygon_utils.h"
#include "game/game.h"
#include "config.h"
#include "platform.h"
#include "tooltip.h"
#include <cstring>

namespace UI {

static CursorSystem g_cursor;



void init_cursor() {
    if (g_cursor.initialized) return;
    
    // Load cursor textures
    g_cursor.tex_default = Renderer::load_texture("cursors/crosshair.png");
    g_cursor.tex_hover = Renderer::load_texture("cursors/crosshair_hover.png");
    
    g_cursor.state = CursorState::Default;
    Tooltip::clear();
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
    
    // Convert UI-FBO (viewport) mouse position to base resolution for hotspot detection
    // Hotspot bounds are stored in BASE (320x180) coordinate space
    Vec2 mouse_base = Vec2(mouse_pos.x / UI::UI_SCALE(), mouse_pos.y / UI::UI_SCALE());
    
    // Check hotspots - set hover state if over a hotspot (don't reset, that's done by reset_cursor_state)
    const auto& hotspots = Game::g_state.scene.geometry.hotspots;
    for (const auto& hotspot : hotspots) {
        if (!hotspot.enabled) continue;
        if (Collision::point_in_polygon(mouse_base, hotspot.bounds)) {
            g_cursor.state = CursorState::Hover;
            // Only set tooltip if not already set by UI (e.g., inventory)
            if (Tooltip::get().empty()) {
                Tooltip::set(hotspot.tooltip_key.empty() ? hotspot.name : hotspot.tooltip_key);
            }
            break;  // First match wins
        }
    }
}

void render_cursor(Vec2 mouse_pos) {
    if (!g_cursor.initialized) return;
    
    // Check if we have a selected item to display on cursor
    if (has_selected_item()) {
        const std::string& item_id = get_selected_item();
        const auto* item_def = Inventory::get_item_def(item_id);
        if (item_def && item_def->icon_tex != 0) {
            // Render item texture at cursor position
            float item_size = CursorConfig::item_cursor_size();
            Vec3 item_pos = Vec3(mouse_pos.x, mouse_pos.y, ZDepth::CURSOR);
            // If hovering a hotspot, render with outline
            if (g_cursor.state == CursorState::Hover) {
                Vec4 outline_color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);  // White outline
                Renderer::render_sprite_outlined(item_def->icon_tex, item_pos, Vec2(item_size, item_size), outline_color, PivotPoint::CENTER);
            } else {
                Renderer::render_sprite(item_def->icon_tex, item_pos, Vec2(item_size, item_size), PivotPoint::CENTER);
            }
            // Still render tooltip if hovering hotspot
            if (!Tooltip::get().empty()) {
                Tooltip::render(mouse_pos, Tooltip::get().c_str());
            }
            return;  // Don't render normal cursor when item is selected
        }
    }
    
    // Select cursor texture based on state
    Renderer::TextureID cursor_tex = (g_cursor.state == CursorState::Hover) 
                                     ? g_cursor.tex_hover 
                                     : g_cursor.tex_default;
    
    // Render cursor sprite at mouse position (centered on cursor)
    // Use CENTER pivot so cursor hotspot is at mouse position
    Vec3 cursor_pos = Vec3(mouse_pos.x, mouse_pos.y, ZDepth::CURSOR);
    float cursor_size = CursorConfig::cursor_size();
    Renderer::render_sprite(cursor_tex, cursor_pos, Vec2(cursor_size, cursor_size), PivotPoint::CENTER);
    
    // Render tooltip if we have text
    if (!Tooltip::get().empty()) {
        Tooltip::render(mouse_pos, Tooltip::get().c_str());
    }
}



void set_cursor_state(CursorState state) {
    g_cursor.state = state;
}

void set_tooltip(const std::string& text) {
    Tooltip::set(text);
}

void reset_cursor_state() {
    Tooltip::clear();
    g_cursor.state = CursorState::Default;
}

CursorState get_cursor_state() {
    return g_cursor.state;
}

}
