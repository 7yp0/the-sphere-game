#pragma once

#include <array>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include "renderer/renderer.h"

namespace Inventory {

// Maximum inventory slots (configurable)
constexpr uint32_t MAX_SLOTS = 16;

// Item definition - represents an item type
struct ItemDef {
    std::string id;              // Unique identifier (e.g., "screwdriver", "key_small")
    std::string tooltip_key;     // Localization key for tooltip (e.g., "item.screwdriver")
    std::string icon_path;       // Path to icon texture
    Renderer::TextureID icon_tex = 0;  // Loaded texture ID
    bool selectable = true;      // If false, item triggers on_use_callback instead of being selected
    std::function<void()> on_use_callback;  // Callback for non-selectable items (e.g., read book)
};

// Inventory slot - either empty or contains an item
struct Slot {
    std::string item_id;         // Empty string = empty slot
    
    bool is_empty() const { return item_id.empty(); }
    void clear() { item_id.clear(); }
};

// Item combination callback: (item1_id, item2_id) -> result_item_id (empty = no combination)
using CombinationCallback = std::function<std::string(const std::string&, const std::string&)>;

// Invalid combination callback: (item1_id, item2_id) -> called when combination fails
using InvalidCombinationCallback = std::function<void(const std::string&, const std::string&)>;

// Inventory system state
struct InventoryState {
    std::vector<ItemDef> item_defs;           // All registered item definitions
    std::array<Slot, MAX_SLOTS> slots;        // Inventory slots
    CombinationCallback combination_callback; // Callback for item combinations
    InvalidCombinationCallback invalid_combination_callback; // Callback for failed combinations
    
    bool initialized = false;
};

// Initialize inventory system
void init();

// Shutdown inventory system
void shutdown();

// Register an item definition (call this to define all game items)
// tooltip_key: localization key for the item tooltip (e.g., "item.screwdriver")
void register_item(const std::string& id, const std::string& tooltip_key, const std::string& icon_path);

// Set an on-use callback for an item (makes it non-selectable)
// Use for items that trigger actions directly (e.g., reading a book, checking a photo)
void set_item_on_use(const std::string& id, std::function<void()> callback);

// Get item definition by ID (returns nullptr if not found)
const ItemDef* get_item_def(const std::string& id);

// Add item to inventory (returns slot index, or -1 if inventory full)
int add_item(const std::string& item_id);

// Remove item from inventory (returns true if item was found and removed)
bool remove_item(const std::string& item_id);

// Remove item from specific slot
void remove_item_at(int slot_index);

// Check if inventory contains an item
bool has_item(const std::string& item_id);

// Get the slot index of an item (-1 if not found)
int find_item_slot(const std::string& item_id);

// Get first empty slot index (-1 if inventory full)
int find_empty_slot();

// Get number of items in inventory
int get_item_count();

// Check if inventory is full
bool is_full();

// Clear all items from inventory
void clear();

// Set combination callback for item-on-item interactions
void set_combination_callback(CombinationCallback callback);

// Set callback for invalid combinations (for UI feedback)
void set_invalid_combination_callback(InvalidCombinationCallback callback);

// Try to combine two items (returns result item ID, empty if no combination)
std::string try_combine(const std::string& item1_id, const std::string& item2_id);

// Get inventory slot (for UI rendering)
const Slot& get_slot(int index);

// Get all slots (for UI rendering)  
const std::array<Slot, MAX_SLOTS>& get_slots();

}
