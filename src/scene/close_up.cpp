#include "close_up.h"
#include "scene_registry.h"
#include "../game/game.h"
#include "../game/player.h"
#include "../ecs/ecs.h"
#include "../debug/debug_log.h"

namespace Scene {

static CloseUpConfig s_config;

void enter_close_up(const std::string& name, CloseUpConfig config) {
    using namespace Game;

    // Capture return context before tearing down the current scene
    g_state.has_close_up_context = true;
    g_state.close_up_return.scene_name      = g_state.scene.name;
    g_state.close_up_return.player_direction = g_state.player.walk_direction;

    if (g_state.player_entity != ECS::INVALID_ENTITY) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(g_state.player_entity);
        if (transform) {
            g_state.close_up_return.player_pos = transform->position;
        }
    }

    s_config = std::move(config);

    // Switch to CLOSE_UP before load_scene so the autosave guard inside
    // load_scene (which only saves in GAMEPLAY mode) does not fire.
    g_state.mode = GameMode::CLOSE_UP;

    load_scene(name, "");

    DEBUG_INFO("[CloseUp] Entered close-up '%s', will return to '%s'",
               name.c_str(), g_state.close_up_return.scene_name.c_str());
}

void exit_close_up() {
    using namespace Game;

    if (!g_state.has_close_up_context) {
        DEBUG_ERROR("[CloseUp] exit_close_up() called without an active close-up context");
        return;
    }

    const std::string  return_scene = g_state.close_up_return.scene_name;
    const Vec2         return_pos   = g_state.close_up_return.player_pos;
    const WalkDirection return_dir  = g_state.close_up_return.player_direction;

    g_state.has_close_up_context = false;

    // Switch to GAMEPLAY before load_scene so the autosave fires on return
    // (matches normal scene-change behaviour).
    g_state.mode = GameMode::GAMEPLAY;

    load_scene(return_scene, "");

    // load_scene places the player at the scene centre (empty spawn point name).
    // Override with the saved position.
    if (g_state.player_entity != ECS::INVALID_ENTITY) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(g_state.player_entity);
        if (transform) {
            transform->position = return_pos;
        }
    }
    g_state.player.walk_direction = return_dir;

    DEBUG_INFO("[CloseUp] Exited close-up, returned to scene '%s'", return_scene.c_str());
}

const CloseUpConfig& get_close_up_config() {
    return s_config;
}

} // namespace Scene
