#pragma once

namespace SaveSystem {

// Call once after the asset manager is initialized.
void init();

// Call every frame to flush debounced saves.
void update(float delta_time);

// Save current game state to the auto-save slot immediately.
void save();

// Restore game state from the auto-save slot.
// Returns true on success.
bool load();

// True if an auto-save file exists.
bool has_save();

// Delete the auto-save file (use for New Game).
void clear_save();

// Schedule a debounced save (max once per second).
// Use this for frequent events like flag changes and item pickups.
void schedule_save();

}
