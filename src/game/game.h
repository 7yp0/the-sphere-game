#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include "core/animation_bank.h"
#include "scene/scene.h"
#include <cstdint>

namespace Game {

constexpr float PLAYER_SPEED = 300.0f;

// Animation state for player/entities
enum class AnimationState {
    Idle,
    Walking
};

struct Player {
    Vec2 position;                              // Pixel coordinates (0-1024, 0-768)
    Vec2 target_position;                       // Pixel coordinates
    float speed = PLAYER_SPEED;                 // pixels per second
    AnimationState animation_state = AnimationState::Idle;
    Core::AnimationBank* animations;            // Generic animation bank for any entity
    PivotPoint pivot = PivotPoint::BOTTOM_CENTER;
};

struct GameState {
    Scene::Scene scene;
    Core::AnimationBank playerAnimations;
    Player player;
    
    uint32_t viewport_width = 1024;
    uint32_t viewport_height = 768;
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

