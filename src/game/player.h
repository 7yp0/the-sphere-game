#pragma once

#include "types.h"
#include "core/animation_bank.h"
#include <cstdint>

namespace Game {

// Animation state for player
enum class AnimationState {
    Idle,
    Walking
};

// Direction for walking animation (used with Guybrush spritesheet)
enum class WalkDirection {
    Down,
    Right,
    Up,
    Left
};

// Hotspot interaction state machine
enum class HotspotInteractionState {
    None,           // Not interacting with any hotspot
    Approaching,    // Walking towards hotspot
    InRange         // Within interaction distance
};

struct Player {
    // State
    Vec3 position;                              // Pixel coordinates in scene space (xy) + z-depth (z)
    Vec3 target_position;                       // Pixel coordinates in scene space + z-depth
    Vec2 size = Vec2(64.0f, 100.0f);            // Sprite dimensions in pixels (Guybrush frame size)
    AnimationState animation_state = AnimationState::Idle;
    WalkDirection walk_direction = WalkDirection::Down;  // Current facing direction
    Core::AnimationBank* animations;            // Generic animation bank for any entity
    PivotPoint pivot = PivotPoint::BOTTOM_CENTER;
    int active_hotspot_index = -1;              // -1 = none, otherwise index into scene.geometry.hotspots
    HotspotInteractionState hotspot_state = HotspotInteractionState::None;
    
    // Settings/Config
    float speed = 300.0f;                          // Pixels per second
    float distance_threshold = 1.0f;               // Min distance to target to consider "moving"
    float boundary_margin = 10.0f;                 // Keep player away from scene edges
    float hotspot_proximity_tolerance = 20.0f;     // Extra distance beyond interaction_distance
    float direction_normalization_threshold = 0.1f; // Min distance to normalize direction vector
    float stuck_movement_threshold = 0.1f;         // Pixels per frame to consider player stuck
};

void player_init(Player& player, uint32_t viewport_width, uint32_t viewport_height, 
                 Core::AnimationBank* animations);

void player_handle_input(Player& player);

void player_update(Player& player, uint32_t viewport_width, uint32_t viewport_height, float delta_time);

// Get the animation name for the current player state and direction
const char* player_get_animation_name(const Player& player);

}
