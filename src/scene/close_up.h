#pragma once

#include "types.h"
#include <functional>
#include <string>

namespace Scene {

struct CloseUpConfig {
    bool show_inventory = false;
    std::function<void(Vec2)> on_update;   // called each frame with base-res mouse pos
    std::function<void()>     on_render;   // called each frame inside the scene FBO
    std::function<void()>     on_success;  // called on puzzle completion → auto-exit
    std::function<void()>     on_back;     // called on manual back → optional
};

// Enter a close-up scene.
// Saves current scene/player state, loads the named scene, sets GameMode::CLOSE_UP.
// Player is hidden and input-blocked; autosave is suppressed while in close-up.
void enter_close_up(const std::string& name, CloseUpConfig config);

// Exit the current close-up.
// Reloads the original scene, restores player position and direction, sets GameMode::GAMEPLAY.
// Autosave fires on return (same as a normal scene change).
void exit_close_up();

// Access the config for the currently active close-up (valid only in CLOSE_UP mode).
const CloseUpConfig& get_close_up_config();

} // namespace Scene
