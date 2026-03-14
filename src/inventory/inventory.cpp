#include "inventory.h"
#include "renderer/texture_loader.h"
#include <algorithm>

namespace Inventory {

static InventoryState g_inventory;

void init() {
    if (g_inventory.initialized) return;
    
    g_inventory.item_defs.clear();
    for (auto& slot : g_inventory.slots) {
        slot.clear();
    }
    g_inventory.combination_callback = nullptr;
    g_inventory.initialized = true;
}

void shutdown() {
    g_inventory.item_defs.clear();
    for (auto& slot : g_inventory.slots) {
        slot.clear();
    }
    g_inventory.combination_callback = nullptr;
    g_inventory.initialized = false;
}

void register_item(const std::string& id, const std::string& tooltip_key, const std::string& icon_path) {
    // Check if item already registered
    for (auto& def : g_inventory.item_defs) {
        if (def.id == id) {
            // Update existing definition
            def.tooltip_key = tooltip_key;
            def.icon_path = icon_path;
            if (!icon_path.empty()) {
                def.icon_tex = Renderer::load_texture(icon_path.c_str());
            }
            return;
        }
    }
    
    // Add new item definition
    ItemDef def;
    def.id = id;
    def.tooltip_key = tooltip_key;
    def.icon_path = icon_path;
    if (!icon_path.empty()) {
        def.icon_tex = Renderer::load_texture(icon_path.c_str());
    }
    g_inventory.item_defs.push_back(def);
}

void set_item_on_use(const std::string& id, std::function<void()> callback) {
    for (auto& def : g_inventory.item_defs) {
        if (def.id == id) {
            def.on_use_callback = callback;
            def.selectable = false;  // Non-selectable items trigger callback instead
            return;
        }
    }
}

const ItemDef* get_item_def(const std::string& id) {
    for (const auto& def : g_inventory.item_defs) {
        if (def.id == id) {
            return &def;
        }
    }
    return nullptr;
}

int add_item(const std::string& item_id) {
    // Verify item is registered
    if (get_item_def(item_id) == nullptr) {
        return -1;  // Unknown item
    }
    
    // Find first empty slot
    int slot_index = find_empty_slot();
    if (slot_index < 0) {
        return -1;  // Inventory full
    }
    
    g_inventory.slots[slot_index].item_id = item_id;
    return slot_index;
}

bool remove_item(const std::string& item_id) {
    int slot_index = find_item_slot(item_id);
    if (slot_index < 0) {
        return false;  // Item not found
    }
    
    g_inventory.slots[slot_index].clear();
    return true;
}

void remove_item_at(int slot_index) {
    if (slot_index >= 0 && slot_index < (int)MAX_SLOTS) {
        g_inventory.slots[slot_index].clear();
    }
}

bool has_item(const std::string& item_id) {
    return find_item_slot(item_id) >= 0;
}

int find_item_slot(const std::string& item_id) {
    for (int i = 0; i < (int)MAX_SLOTS; i++) {
        if (g_inventory.slots[i].item_id == item_id) {
            return i;
        }
    }
    return -1;
}

int find_empty_slot() {
    for (int i = 0; i < (int)MAX_SLOTS; i++) {
        if (g_inventory.slots[i].is_empty()) {
            return i;
        }
    }
    return -1;
}

int get_item_count() {
    int count = 0;
    for (const auto& slot : g_inventory.slots) {
        if (!slot.is_empty()) {
            count++;
        }
    }
    return count;
}

bool is_full() {
    return find_empty_slot() < 0;
}

void clear() {
    for (auto& slot : g_inventory.slots) {
        slot.clear();
    }
}

void set_combination_callback(CombinationCallback callback) {
    g_inventory.combination_callback = callback;
}

void set_invalid_combination_callback(InvalidCombinationCallback callback) {
    g_inventory.invalid_combination_callback = callback;
}

std::string try_combine(const std::string& item1_id, const std::string& item2_id) {
    std::string result;
    
    if (g_inventory.combination_callback) {
        // Try both directions (item1 + item2 and item2 + item1)
        result = g_inventory.combination_callback(item1_id, item2_id);
        if (result.empty()) {
            result = g_inventory.combination_callback(item2_id, item1_id);
        }
    }
    
    // If combination failed, call invalid combination callback
    if (result.empty() && g_inventory.invalid_combination_callback) {
        g_inventory.invalid_combination_callback(item1_id, item2_id);
    }
    
    return result;
}

const Slot& get_slot(int index) {
    static Slot empty_slot;  // For out-of-bounds access
    if (index >= 0 && index < (int)MAX_SLOTS) {
        return g_inventory.slots[index];
    }
    return empty_slot;
}

const std::array<Slot, MAX_SLOTS>& get_slots() {
    return g_inventory.slots;
}

}
