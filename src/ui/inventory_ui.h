#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include <string>

namespace UI {

// Inventory UI configuration
namespace InventoryConfig {
    constexpr int GRID_COLS = 4;                    // Columns in inventory grid
    constexpr int GRID_ROWS = 4;                    // Rows in inventory grid (4x4 = 16 slots)
    constexpr float SLOT_SIZE = 80.0f;              // Size of each slot in viewport pixels
    constexpr float SLOT_PADDING = 6.0f;            // Padding between slots
    constexpr float PANEL_PADDING = 20.0f;          // Padding around the grid
    constexpr float ICON_BUTTON_SIZE = 48.0f;       // Size of inventory icon button
    constexpr float ICON_BUTTON_MARGIN = 16.0f;     // Margin from screen edge
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
