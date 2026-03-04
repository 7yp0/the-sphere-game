#pragma once

namespace Game {

// Initialize game (load assets, setup entities)
void init();

// Update game state (player movement, input, logic, collisions)
// delta_time: time since last frame in seconds
void update(float delta_time);

// Update visual effects (animations, particles, etc)
// delta_time: time since last frame in seconds
void visual_update(float delta_time);

// Render game (call Renderer functions)
void render();

// Shutdown game (cleanup)
void shutdown();

}

