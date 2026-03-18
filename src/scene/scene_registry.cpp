#include "scene_registry.h"
#include "scene.h"
#include "../game/game.h"
#include "../game/player.h"
#include "../save/save_system.h"
#include "../debug/debug_log.h"
#include "../ecs/ecs.h"
#include <unordered_map>

// Forward-declare each scene's init function (defined in their own .cpp files)
namespace Scene {
    void init_scene_test();
    void init_scene_test2();
    // void init_scene_living_room();  // add as new scenes are created
}

namespace Scene {

static std::unordered_map<std::string, SceneInitFn> g_registry;

void register_scene(const std::string& name, SceneInitFn fn) {
    g_registry[name] = fn;
}

void register_all_scenes() {
    register_scene("test", init_scene_test);
    register_scene("test2", init_scene_test2);
    // register_scene("living_room", init_scene_living_room);
}

void load_scene(const std::string& scene_name, const std::string& spawn_point_name) {
    using namespace Game;

    // ---- Snapshot current scene state before teardown ----
    // Skip during SaveSystem::load() — scene_states were already restored from the
    // save file and must not be overwritten by the current (pre-load) live state.
    if (!g_state.scene.name.empty() && !SaveSystem::is_loading()) {
        auto& snap = g_state.scene_states[g_state.scene.name];
        snap.hotspot_enabled.clear();
        for (const auto& hs : g_state.scene.geometry.hotspots) {
            snap.hotspot_enabled[hs.name] = hs.enabled;
        }
        snap.entity_visible.clear();
        for (const auto& [name, id] : g_state.scene.named_entities) {
            auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(id);
            if (sprite) snap.entity_visible[name] = sprite->visible;
        }
    }

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

    // ---- Restore persisted scene state (if we've been here before) ----
    auto state_it = g_state.scene_states.find(scene_name);
    if (state_it != g_state.scene_states.end()) {
        const auto& snap = state_it->second;
        for (auto& hs : g_state.scene.geometry.hotspots) {
            auto hs_it = snap.hotspot_enabled.find(hs.name);
            if (hs_it != snap.hotspot_enabled.end()) hs.enabled = hs_it->second;
        }
        for (const auto& [name, visible] : snap.entity_visible) {
            set_entity_visible(name, visible);
        }
    }

    // ---- Determine spawn position and direction ----
    Vec2 spawn_pos(-1.0f, -1.0f);  // sentinel: -1,-1 → use scene center
    std::string spawn_dir = "down";
    if (!spawn_point_name.empty()) {
        Vec2 found;
        if (get_spawn_point(spawn_point_name, found, spawn_dir)) {
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

    // Apply spawn direction
    if      (spawn_dir == "up")    g_state.player.walk_direction = Game::WalkDirection::Up;
    else if (spawn_dir == "left")  g_state.player.walk_direction = Game::WalkDirection::Left;
    else if (spawn_dir == "right") g_state.player.walk_direction = Game::WalkDirection::Right;
    else                           g_state.player.walk_direction = Game::WalkDirection::Down;

    DEBUG_INFO("[SceneRegistry] Loaded scene '%s', spawn '%s'",
               scene_name.c_str(), spawn_point_name.c_str());

    // Auto-save on scene change — only during active gameplay.
    // Not during init() (MAIN_MENU) or SaveSystem::load() (g_is_loading guard inside save()).
    if (Game::g_state.mode == Game::GameMode::GAMEPLAY) {
        SaveSystem::save();
    }
}

} // namespace Scene
