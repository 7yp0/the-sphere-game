#include "../inventory/inventory.h"

namespace Scene {

void init_act_1() {
    // Register item definitions that exist across the whole act
    // (scene-local items like "stone" are registered in their respective scene inits)
    Inventory::register_item("note", "A torn note with strange symbols", "ui/items/item_note.png");

    // Give player their starting items
    Inventory::add_item("note");
}

} // namespace Scene
