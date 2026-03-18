#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include "animation_bank.h"
#include "scene/scene.h"
#include "config.h"
#include "player.h"
#include "ecs/ecs.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Game {

enum class GameMode {
    MAIN_MENU,  // Overlay menu shown, scene renders underneath
    GAMEPLAY,   // Normal gameplay, player can move
    DEBUG,      // Debug overlay active (ESC reserved for debug tools)
};

// Persisted state for a single scene (saved on exit, restored on re-entry)
struct SceneState {
    std::unordered_map<std::string, bool> hotspot_enabled;
    std::unordered_map<std::string, bool> entity_visible;
};

struct GameState {
    Scene::Scene scene;
    Player player;

    // ECS World - manages all entities and components
    ECS::World ecs_world;

    // Player entity ID for quick access
    ECS::EntityID player_entity = ECS::INVALID_ENTITY;

    uint32_t viewport_width = Config::VIEWPORT_WIDTH;
    uint32_t viewport_height = Config::VIEWPORT_HEIGHT;

    // Base resolution for game logic (all coordinates in this space)
    uint32_t base_width = Config::BASE_WIDTH;
    uint32_t base_height = Config::BASE_HEIGHT;

    // 2.5D depth scaling factor: (window_res / base_res)
    // Used to translate base resolution Y-positions to depth scale calculations
    float scale_factor = 1.0f;  // Will be calculated: viewport_height / BASE_HEIGHT

    // Debug: current shadow caster count (updated each frame)
    uint32_t shadow_caster_count = 0;

    // Current game mode
    GameMode mode = GameMode::MAIN_MENU;

    // Global game state (persistent within a session)
    std::unordered_map<std::string, bool>        flags;
    std::unordered_map<std::string, int>         values;
    std::unordered_map<std::string, std::string> strings;

    // Per-scene state snapshots (saved on scene exit, restored on re-entry)
    std::unordered_map<std::string, SceneState> scene_states;
};

extern GameState g_state;

// Initialize game (load assets, setup entities)
void init();

// Set viewport dimensions (needed for mouse coordinate conversion)
void set_viewport(uint32_t width, uint32_t height);

// Update game state and visuals (player movement, input, logic, animations)
// delta_time: time since last frame in seconds
void update(float delta_time);

// Render game (call Renderer functions)
void render();

// Shutdown game (cleanup)
void shutdown();

// Start a completely fresh game (resets all state, loads Act 1).
void start_new_game();

// Restore game from auto-save and enter gameplay.
// Returns false if no save file exists.
bool continue_game();

// --- Global state accessors ---
void        set_flag(const std::string& key, bool value = true);
bool        get_flag(const std::string& key, bool default_val = false);

void        set_value(const std::string& key, int value);
int         get_value(const std::string& key, int default_val = 0);

void        set_string(const std::string& key, const std::string& value);
std::string get_string(const std::string& key, const std::string& default_val = "");

}

