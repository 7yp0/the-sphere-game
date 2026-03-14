#include "inventory_ui.h"
#include "cursor.h"
#include "inventory/inventory.h"
#include "renderer/texture_loader.h"
#include "renderer/text.h"
#include "config.h"
#include "platform.h"

namespace UI {

static InventoryUI g_inv_ui;

// Helper: Calculate panel dimensions
static Vec2 get_panel_size() {
    float grid_width = InventoryConfig::GRID_COLS * InventoryConfig::SLOT_SIZE 
                     + (InventoryConfig::GRID_COLS - 1) * InventoryConfig::SLOT_PADDING;
    float grid_height = InventoryConfig::GRID_ROWS * InventoryConfig::SLOT_SIZE 
                      + (InventoryConfig::GRID_ROWS - 1) * InventoryConfig::SLOT_PADDING;
    return Vec2(
        grid_width + InventoryConfig::PANEL_PADDING * 2,
        grid_height + InventoryConfig::PANEL_PADDING * 2
    );
}

// Helper: Calculate panel position (centered on screen)
static Vec2 get_panel_position() {
    Vec2 panel_size = get_panel_size();
    float viewport_w = (float)Platform::get_window_width();
    float viewport_h = (float)Platform::get_window_height();
    return Vec2(
        (viewport_w - panel_size.x) * 0.5f,
        (viewport_h - panel_size.y) * 0.5f
    );
}

// Helper: Calculate slot position within panel
static Vec2 get_slot_position(int slot_index) {
    Vec2 panel_pos = get_panel_position();
    int col = slot_index % InventoryConfig::GRID_COLS;
    int row = slot_index / InventoryConfig::GRID_COLS;
    
    float x = panel_pos.x + InventoryConfig::PANEL_PADDING 
            + col * (InventoryConfig::SLOT_SIZE + InventoryConfig::SLOT_PADDING);
    float y = panel_pos.y + InventoryConfig::PANEL_PADDING 
            + row * (InventoryConfig::SLOT_SIZE + InventoryConfig::SLOT_PADDING);
    
    return Vec2(x, y);
}

// Helper: Check if point is in rectangle
static bool point_in_rect(Vec2 point, Vec2 rect_pos, Vec2 rect_size) {
    return point.x >= rect_pos.x && point.x < rect_pos.x + rect_size.x
        && point.y >= rect_pos.y && point.y < rect_pos.y + rect_size.y;
}

// Helper: Get inventory button position (bottom-left corner)
static Vec2 get_icon_button_position() {
    float viewport_h = (float)Platform::get_window_height();
    return Vec2(
        InventoryConfig::ICON_BUTTON_MARGIN,
        viewport_h - InventoryConfig::ICON_BUTTON_SIZE - InventoryConfig::ICON_BUTTON_MARGIN
    );
}

void init_inventory_ui() {
    if (g_inv_ui.initialized) return;
    
    // Initialize the inventory system
    Inventory::init();
    
    // Load UI textures
    g_inv_ui.icon_button_tex = Renderer::load_texture("ui/inventory/inventory_button.png");
    g_inv_ui.icon_button_hover_tex = Renderer::load_texture("ui/inventory/inventory_button_hover.png");
    g_inv_ui.slot_bg_tex = Renderer::load_texture("ui/inventory/slot_bg.png");
    g_inv_ui.panel_bg_tex = Renderer::load_texture("ui/inventory/panel_bg.png");
    
    g_inv_ui.is_open = false;
    g_inv_ui.hovered_slot = -1;
    g_inv_ui.selected_item_id.clear();
    g_inv_ui.initialized = true;
}

void shutdown_inventory_ui() {
    Inventory::shutdown();
    g_inv_ui.is_open = false;
    g_inv_ui.hovered_slot = -1;
    g_inv_ui.selected_item_id.clear();
    g_inv_ui.initialized = false;
}

bool update_inventory_ui(Vec2 mouse_pos) {
    if (!g_inv_ui.initialized) return false;
    
    bool consumed = false;
    
    // Check inventory icon button hover/click
    Vec2 btn_pos = get_icon_button_position();
    Vec2 btn_size = Vec2(InventoryConfig::ICON_BUTTON_SIZE, InventoryConfig::ICON_BUTTON_SIZE);
    bool hovering_button = point_in_rect(mouse_pos, btn_pos, btn_size);
    
    // Only consume mouse_clicked if we're actually hovering UI elements
    bool mouse_clicked = false;
    if (hovering_button || g_inv_ui.is_open) {
        mouse_clicked = Platform::mouse_clicked();
    }
    
    // Track hover state for rendering
    g_inv_ui.hovering_button = hovering_button;
    
    if (hovering_button && mouse_clicked) {
        toggle_inventory();
        consumed = true;
    }
    
    // If inventory is closed, nothing more to do
    if (!g_inv_ui.is_open) {
        return consumed || hovering_button;
    }
    
    // Check if mouse is over the panel
    Vec2 panel_pos = get_panel_position();
    Vec2 panel_size = get_panel_size();
    bool hovering_panel = point_in_rect(mouse_pos, panel_pos, panel_size);
    
    // Close inventory when mouse leaves panel AND we have an item selected
    if (!hovering_panel && !hovering_button && has_selected_item()) {
        close_inventory();
        return true;
    }
    
    // Click outside panel closes inventory (but not on button)
    if (mouse_clicked && !hovering_panel && !hovering_button) {
        close_inventory();
        return true;
    }
    
    // Reset hovered slot
    g_inv_ui.hovered_slot = -1;
    
    // Check slot hover
    if (hovering_panel) {
        consumed = true;  // Panel consumes all input when open
        
        for (int i = 0; i < (int)Inventory::MAX_SLOTS; i++) {
            Vec2 slot_pos = get_slot_position(i);
            Vec2 slot_size = Vec2(InventoryConfig::SLOT_SIZE, InventoryConfig::SLOT_SIZE);
            
            if (point_in_rect(mouse_pos, slot_pos, slot_size)) {
                g_inv_ui.hovered_slot = i;
                
                // Set tooltip for hovered item (but not for selected item)
                const auto& slot = Inventory::get_slot(i);
                if (!slot.is_empty() && slot.item_id != g_inv_ui.selected_item_id) {
                    const auto* item_def = Inventory::get_item_def(slot.item_id);
                    if (item_def && !item_def->tooltip_key.empty()) {
                        set_tooltip(item_def->tooltip_key);
                    }
                    // Set hover state for outline on item cursor
                    if (has_selected_item()) {
                        set_cursor_state(CursorState::Hover);
                    }
                }
                break;
            }
        }
    }
    
    // Handle click on slot
    if (mouse_clicked && g_inv_ui.hovered_slot >= 0) {
        const auto& clicked_slot = Inventory::get_slot(g_inv_ui.hovered_slot);
        
        // Don't allow clicking on already selected item
        if (!clicked_slot.is_empty() && clicked_slot.item_id != g_inv_ui.selected_item_id) {
            const auto* item_def = Inventory::get_item_def(clicked_slot.item_id);
            
            // Check if we already have an item selected (for combination)
            if (has_selected_item()) {
                // Try to combine selected item with clicked item (even non-selectable items)
                std::string result = Inventory::try_combine(g_inv_ui.selected_item_id, clicked_slot.item_id);
                if (!result.empty()) {
                    // Combination successful - remove both items, add result
                    Inventory::remove_item(g_inv_ui.selected_item_id);
                    Inventory::remove_item(clicked_slot.item_id);
                    Inventory::add_item(result);
                }
                // Combination failed - clear selected item (invalid_combination_callback already called)
                clear_selected_item();
            }
            // No item selected - check if clicked item is non-selectable
            else if (item_def && !item_def->selectable) {
                // Trigger on-use callback
                if (item_def->on_use_callback) {
                    item_def->on_use_callback();
                }
            } else {
                // Select clicked item (cursor will show item texture)
                select_item(clicked_slot.item_id);
            }
        }
        consumed = true;
    }
    
    return consumed || hovering_panel || hovering_button;
}

void render_inventory_ui() {
    if (!g_inv_ui.initialized) return;
    
    // Render inventory icon button
    Vec2 btn_pos = get_icon_button_position();
    Vec2 btn_size = Vec2(InventoryConfig::ICON_BUTTON_SIZE, InventoryConfig::ICON_BUTTON_SIZE);
    
    // Select texture based on hover state
    Renderer::TextureID btn_tex = g_inv_ui.hovering_button 
        ? g_inv_ui.icon_button_hover_tex 
        : g_inv_ui.icon_button_tex;
    
    if (btn_tex != 0) {
        // Use texture
        Renderer::render_sprite(
            btn_tex,
            Vec3(btn_pos.x, btn_pos.y, ZDepth::GAME_HUD),
            btn_size,
            PivotPoint::TOP_LEFT
        );
    } else {
        // Fallback to solid color
        Vec4 btn_color = has_selected_item() 
            ? Vec4(0.4f, 0.5f, 0.3f, 0.9f)   // Greenish when item selected
            : Vec4(0.3f, 0.3f, 0.4f, 0.9f);  // Dark background
        
        Renderer::render_rect(
            Vec3(btn_pos.x, btn_pos.y, ZDepth::GAME_HUD),
            btn_size,
            btn_color,
            PivotPoint::TOP_LEFT
        );
        
        Renderer::render_text(
            "INV",
            Vec2(btn_pos.x + InventoryConfig::ICON_BUTTON_SIZE * 0.5f - 24.0f, 
                 btn_pos.y + InventoryConfig::ICON_BUTTON_SIZE * 0.5f - 12.0f),
            0.75f
        );
    }
    
    // If inventory is closed, we're done
    if (!g_inv_ui.is_open) return;
    
    // Render panel background
    Vec2 panel_pos = get_panel_position();
    Vec2 panel_size = get_panel_size();
    
    if (g_inv_ui.panel_bg_tex != 0) {
        Renderer::render_sprite(
            g_inv_ui.panel_bg_tex,
            Vec3(panel_pos.x, panel_pos.y, ZDepth::PANELS),
            panel_size,
            PivotPoint::TOP_LEFT
        );
    } else {
        Vec4 panel_color = Vec4(0.15f, 0.15f, 0.2f, 0.95f);
        Renderer::render_rect(
            Vec3(panel_pos.x, panel_pos.y, ZDepth::PANELS),
            panel_size,
            panel_color,
            PivotPoint::TOP_LEFT
        );
    }
    
    // Render slots
    for (int i = 0; i < (int)Inventory::MAX_SLOTS; i++) {
        Vec2 slot_pos = get_slot_position(i);
        Vec2 slot_size = Vec2(InventoryConfig::SLOT_SIZE, InventoryConfig::SLOT_SIZE);
        
        // Slot background
        if (g_inv_ui.slot_bg_tex != 0) {
            Renderer::render_sprite(
                g_inv_ui.slot_bg_tex,
                Vec3(slot_pos.x, slot_pos.y, ZDepth::PANELS - 0.001f),
                slot_size,
                PivotPoint::TOP_LEFT
            );
        } else {
            Vec4 slot_color = Vec4(0.25f, 0.25f, 0.3f, 1.0f);
            if (i == g_inv_ui.hovered_slot) {
                slot_color = Vec4(0.35f, 0.35f, 0.45f, 1.0f);
            }
            Renderer::render_rect(
                Vec3(slot_pos.x, slot_pos.y, ZDepth::PANELS - 0.001f),
                slot_size,
                slot_color,
                PivotPoint::TOP_LEFT
            );
        }
        
        // Render item icon (but not if it's the currently selected item - that's on cursor)
        const auto& slot = Inventory::get_slot(i);
        if (!slot.is_empty() && slot.item_id != g_inv_ui.selected_item_id) {
            const auto* item_def = Inventory::get_item_def(slot.item_id);
            if (item_def && item_def->icon_tex != 0) {
                float icon_size = InventoryConfig::SLOT_SIZE - 8.0f;
                Vec2 icon_pos = Vec2(
                    slot_pos.x + (InventoryConfig::SLOT_SIZE - icon_size) * 0.5f,
                    slot_pos.y + (InventoryConfig::SLOT_SIZE - icon_size) * 0.5f
                );
                Renderer::render_sprite(
                    item_def->icon_tex,
                    Vec3(icon_pos.x, icon_pos.y, ZDepth::PANELS - 0.002f),
                    Vec2(icon_size, icon_size),
                    PivotPoint::TOP_LEFT
                );
            } else {
                // No texture - render placeholder with item ID
                Renderer::render_text(
                    slot.item_id.substr(0, 3).c_str(),
                    Vec2(slot_pos.x + 8.0f, slot_pos.y + 20.0f),
                    0.5f
                );
            }
        }
    }
}

void open_inventory() {
    g_inv_ui.is_open = true;
}

void close_inventory() {
    g_inv_ui.is_open = false;
    g_inv_ui.hovered_slot = -1;
}

void toggle_inventory() {
    if (g_inv_ui.is_open) {
        close_inventory();
    } else {
        open_inventory();
    }
}

bool is_inventory_open() {
    return g_inv_ui.is_open;
}

void select_item(const std::string& item_id) {
    g_inv_ui.selected_item_id = item_id;
}

void clear_selected_item() {
    g_inv_ui.selected_item_id.clear();
}

const std::string& get_selected_item() {
    return g_inv_ui.selected_item_id;
}

bool has_selected_item() {
    return !g_inv_ui.selected_item_id.empty();
}

}
