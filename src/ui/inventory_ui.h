#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include "ui.h"
#include "platform.h"
#include <string>
#include <algorithm>

namespace UI {

// Inventory UI configuration (BASE sizes in logical 320x180 pixels)
namespace InventoryConfig {
    constexpr int GRID_COLS = 4;
    constexpr int GRID_ROWS = 4;

    constexpr float BASE_SLOT_SIZE         = 20.0f;
    constexpr float BASE_SLOT_PADDING      = 1.5f;
    constexpr float BASE_PANEL_PADDING     = 5.0f;
    constexpr float BASE_ICON_BUTTON_SIZE  = 24.0f;
    constexpr float BASE_ICON_BUTTON_MARGIN = 4.0f;

    // Scaled accessors (multiply by UI_SCALE to get UI-FBO pixel values)
    inline float slot_size()          { return BASE_SLOT_SIZE          * UI::UI_SCALE; }
    inline float slot_padding()       { return BASE_SLOT_PADDING       * UI::UI_SCALE; }
    inline float panel_padding()      { return BASE_PANEL_PADDING      * UI::UI_SCALE; }
    inline float icon_button_size()   { return BASE_ICON_BUTTON_SIZE   * UI::UI_SCALE; }
    inline float icon_button_margin() { return BASE_ICON_BUTTON_MARGIN * UI::UI_SCALE; }
}

// Inventory UI state
struct InventoryUI {
    bool is_open = false;                           // Is inventory panel open?
    int hovered_slot = -1;                          // Currently hovered slot (-1 = none)
    bool hovering_button = false;                   // Is mouse over inventory button?
    
    // Selected item for "use on hotspot" feature
    std::string selected_item_id;                   // Item selected to use (empty = none)
    
    // Textures
    Renderer::TextureID slot_bg_tex = 0;           // Slot background
    Renderer::TextureID panel_bg_tex = 0;          // Panel background (optional)
    Renderer::TextureID icon_button_tex = 0;       // Inventory icon button
    Renderer::TextureID icon_button_hover_tex = 0; // Inventory icon button (hover)
    
    bool initialized = false;
};

// Initialize inventory UI (load textures)
void init_inventory_ui();

// Shutdown inventory UI
void shutdown_inventory_ui();

// Update inventory UI (handle hover, input)
// mouse_pos: viewport pixel coordinates
// Returns true if UI consumed the input (click was on inventory)
bool update_inventory_ui(Vec2 mouse_pos);

// Render inventory UI (call after upscale, at viewport resolution)
// mouse_pos: viewport pixel coordinates for drag rendering
void render_inventory_ui();

// Open/close inventory panel
void open_inventory();
void close_inventory();
void toggle_inventory();
bool is_inventory_open();

// Item selection for "use on hotspot"
void select_item(const std::string& item_id);
void clear_selected_item();
const std::string& get_selected_item();
bool has_selected_item();

}
