#pragma once

namespace Scene {

// Register all acts (called once at startup)
void register_all_acts();

// Load an act: resets all game state, runs act init, loads starting scene
void load_act(int number);

} // namespace Scene
