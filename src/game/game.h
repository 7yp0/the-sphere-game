#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include <cstdint>

namespace Game {

constexpr float PLAYER_SPEED = 300.0f;

struct Player {
    Vec2 position;          // Pixel coordinates (0-1024, 0-768)
    Vec2 target_position;   // Pixel coordinates
    float speed = PLAYER_SPEED;   // pixels per second
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

