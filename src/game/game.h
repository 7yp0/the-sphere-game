#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include <cstdint>

namespace Game {

struct Player {
    Vec2 position;
    Vec2 target_position;
    float speed = 200.0f;  // pixels per second
    Renderer::TextureID texture;
};

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

