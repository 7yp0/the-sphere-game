#include "scene_registry.h"
#include "scene.h"
#include "../game/game.h"
#include "../game/player.h"
#include "../debug/debug_log.h"
#include <unordered_map>

// Forward-declare each scene's init function (defined in their own .cpp files)
namespace Scene {
    void init_scene_test();
    // void init_scene_living_room();  // add as new scenes are created
}

namespace Scene {

static std::unordered_map<std::string, SceneInitFn> g_registry;

void register_scene(const std::string& name, SceneInitFn fn) {
    g_registry[name] = fn;
}

void register_all_scenes() {
    register_scene("test", init_scene_test);
    // register_scene("living_room", init_scene_living_room);
}

void load_scene(const std::string& scene_name, const std::string& spawn_point_name) {
    using namespace Game;

    // ---- Tear down current scene ----
    for (ECS::EntityID id : g_state.scene.prop_entities) {
        if (g_state.ecs_world.is_valid(id))
            g_state.ecs_world.destroy_entity(id);
    }
    for (ECS::EntityID id : g_state.scene.light_entities) {
        if (g_state.ecs_world.is_valid(id))
            g_state.ecs_world.destroy_entity(id);
    }
    for (ECS::EntityID id : g_state.scene.projector_light_entities) {
        if (g_state.ecs_world.is_valid(id))
            g_state.ecs_world.destroy_entity(id);
    }
    if (g_state.player_entity != ECS::INVALID_ENTITY) {
        g_state.ecs_world.destroy_entity(g_state.player_entity);
        g_state.player_entity = ECS::INVALID_ENTITY;
    }

    // Reset scene and player state
    g_state.scene  = {};   // Scene::Scene — using {} avoids name conflict inside namespace
    g_state.player = {};

    // ---- Run new scene init ----
    auto it = g_registry.find(scene_name);
    if (it == g_registry.end()) {
        DEBUG_ERROR("[SceneRegistry] Scene '%s' not registered", scene_name.c_str());
        return;
    }
    it->second();

    // ---- Determine spawn position ----
    Vec2 spawn_pos(-1.0f, -1.0f);  // sentinel: -1,-1 → use scene center
    if (!spawn_point_name.empty()) {
        Vec2 found;
        if (get_spawn_point(spawn_point_name, found)) {
            spawn_pos = found;
        } else {
            DEBUG_LOG("[SceneRegistry] Spawn point '%s' not found, using center",
                      spawn_point_name.c_str());
        }
    }

    // ---- (Re)create player entity at spawn position ----
    g_state.player_entity = Game::player_create_entity(
        g_state.player, g_state.ecs_world,
        g_state.base_width, g_state.base_height,
        spawn_pos);

    DEBUG_INFO("[SceneRegistry] Loaded scene '%s', spawn '%s'",
               scene_name.c_str(), spawn_point_name.c_str());
}

} // namespace Scene
