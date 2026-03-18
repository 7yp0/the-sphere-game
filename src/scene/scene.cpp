#include "scene.h"
#include "game/game.h"
#include "debug/debug_log.h"
#include "ecs/ecs.h"

using Game::g_state;

namespace Scene {

bool register_hotspot_callback(const std::string& hotspot_name, std::function<void()> callback) {
    for (auto& hotspot : g_state.scene.geometry.hotspots) {
        if (hotspot.name == hotspot_name) {
            hotspot.callback = callback;
            return true;
        }
    }
    DEBUG_ERROR("[Scene] Hotspot '%s' not found - callback not registered", hotspot_name.c_str());
    return false;
}

bool register_hotspot_item_callback(const std::string& hotspot_name, const std::string& item_id, std::function<void()> callback) {
    for (auto& hotspot : g_state.scene.geometry.hotspots) {
        if (hotspot.name == hotspot_name) {
            hotspot.item_callbacks[item_id] = callback;
            return true;
        }
    }
    DEBUG_ERROR("[Scene] Hotspot '%s' not found - item callback not registered", hotspot_name.c_str());
    return false;
}

bool set_hotspot_enabled(const std::string& hotspot_name, bool enabled) {
    for (auto& hotspot : g_state.scene.geometry.hotspots) {
        if (hotspot.name == hotspot_name) {
            hotspot.enabled = enabled;
            return true;
        }
    }
    DEBUG_ERROR("[Scene] Hotspot '%s' not found", hotspot_name.c_str());
    return false;
}

Hotspot* get_hotspot(const std::string& hotspot_name) {
    for (auto& hotspot : g_state.scene.geometry.hotspots) {
        if (hotspot.name == hotspot_name) {
            return &hotspot;
        }
    }
    return nullptr;
}

void register_entity(const std::string& name, ECS::EntityID entity) {
    g_state.scene.named_entities[name] = entity;
}

ECS::EntityID get_entity(const std::string& name) {
    auto it = g_state.scene.named_entities.find(name);
    if (it != g_state.scene.named_entities.end()) {
        return it->second;
    }
    DEBUG_ERROR("[Scene] Entity '%s' not found", name.c_str());
    return ECS::INVALID_ENTITY;
}

bool set_entity_visible(const std::string& name, bool visible) {
    ECS::EntityID entity = get_entity(name);
    if (entity == ECS::INVALID_ENTITY) {
        return false;
    }

    auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(entity);
    if (sprite) {
        sprite->visible = visible;
        return true;
    }
    return false;
}

bool get_spawn_point(const std::string& name, Vec2& out_pos, std::string& out_direction) {
    for (const auto& sp : g_state.scene.geometry.spawn_points) {
        if (sp.name == name) {
            out_pos = sp.position;
            out_direction = sp.direction.empty() ? "down" : sp.direction;
            return true;
        }
    }
    DEBUG_ERROR("[Scene] Spawn point '%s' not found", name.c_str());
    return false;
}

}
