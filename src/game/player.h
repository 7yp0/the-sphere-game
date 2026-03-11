#pragma once

#include "types.h"
#include "core/animation_bank.h"
#include "ecs/components/transform.h"
#include "ecs/components/sprite.h"
#include "ecs/entity.h"
#include <cstdint>

// Forward declaration
namespace ECS { class World; }

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
    // Movement target (pixel coordinates + z-depth)
    Vec3 target_position;
    
    // Sprite dimensions in base resolution (testa_spritesheet: 104x130, scaled to half)
    Vec2 size = Vec2(52.0f, 65.0f);
    
    // Animation state
    AnimationState animation_state = AnimationState::Idle;
    WalkDirection walk_direction = WalkDirection::Down;
    Core::AnimationBank animations;  // Owned animation bank for this player
    PivotPoint pivot = PivotPoint::BOTTOM_CENTER;
    
    // Hotspot interaction
    int active_hotspot_index = -1;
    HotspotInteractionState hotspot_state = HotspotInteractionState::None;
    
    // Edge-following collision state
    bool is_edge_following = false;                // Currently sliding along an edge
    Vec2 edge_start = Vec2(0.0f, 0.0f);           // Start point of current edge
    Vec2 edge_end = Vec2(0.0f, 0.0f);             // End point of current edge
    Vec2 edge_direction = Vec2(0.0f, 0.0f);       // Normalized direction along edge (toward target)
    
    // Time-based stuck detection
    float stuck_timer = 0.0f;                     // Accumulated time without meaningful movement
    Vec2 last_significant_position = Vec2(0.0f, 0.0f);  // Position to compare for stuck detection
    
    // Settings/Config (all values in base resolution 320x180)
    float speed = 75.0f;                           // Pixels per second (base res)
    float distance_threshold = 0.25f;              // Min distance to target to consider "moving"
    float boundary_margin = 2.5f;                  // Keep player away from scene edges
    float hotspot_proximity_tolerance = 5.0f;      // Extra distance beyond interaction_distance
    float direction_normalization_threshold = 0.025f; // Min distance to normalize direction vector
    float stuck_movement_threshold = 1.0f;         // Pixels to move to reset stuck timer
    float stuck_timeout = 0.5f;                    // Seconds without movement to cancel target
};

// Create player ECS entity with Transform and Sprite components
// Returns the created entity ID
ECS::EntityID player_create_entity(Player& player, ECS::World& world,
                                   uint32_t base_width, uint32_t base_height);

void player_init(Player& player, uint32_t viewport_width, uint32_t viewport_height, 
                 ECS::Transform2_5DComponent& transform);

void player_handle_input(Player& player, ECS::Transform2_5DComponent& transform);

void player_update(Player& player, ECS::Transform2_5DComponent& transform,
                   uint32_t viewport_width, uint32_t viewport_height, float delta_time);

// Get the animation name for the current player state and direction
const char* player_get_animation_name(const Player& player);

}
