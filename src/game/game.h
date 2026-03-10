#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include "core/animation_bank.h"
#include "scene/scene.h"
#include "config.h"
#include "player.h"
#include <cstdint>

namespace Game {

struct GameState {
    Scene::Scene scene;
    Core::AnimationBank playerAnimations;
    Player player;
    
    uint32_t viewport_width = Config::VIEWPORT_WIDTH;
    uint32_t viewport_height = Config::VIEWPORT_HEIGHT;
    
    // Base resolution for game logic (all coordinates in this space)
    uint32_t base_width = Config::BASE_WIDTH;
    uint32_t base_height = Config::BASE_HEIGHT;
    
    // 2.5D depth scaling factor: (window_res / base_res)
    // Used to translate base resolution Y-positions to depth scale calculations
    float scale_factor = 1.0f;  // Will be calculated: viewport_height / BASE_HEIGHT
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

}

