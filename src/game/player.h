#pragma once

#include "types.h"
#include "core/animation_bank.h"
#include "ecs/components/transform.h"
#include "ecs/components/sprite.h"
#include "ecs/components/walker.h"
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
    
    // Player-specific settings
    float boundary_margin = 2.5f;                  // Keep player away from scene edges
    float hotspot_proximity_tolerance = 5.0f;      // Extra distance beyond interaction_distance
    float direction_normalization_threshold = 0.025f; // Min distance to normalize direction vector
};

// Create player ECS entity with Transform, Sprite, and Walker components
// Returns the created entity ID
ECS::EntityID player_create_entity(Player& player, ECS::World& world,
                                   uint32_t base_width, uint32_t base_height);

void player_init(Player& player, uint32_t viewport_width, uint32_t viewport_height, 
                 ECS::Transform2_5DComponent& transform, ECS::WalkerComponent& walker);

void player_handle_input(Player& player, ECS::Transform2_5DComponent& transform,
                         ECS::WalkerComponent& walker);

void player_update(Player& player, ECS::Transform2_5DComponent& transform,
                   ECS::WalkerComponent& walker,
                   uint32_t viewport_width, uint32_t viewport_height, float delta_time);

// Get the animation name for the current player state and direction
const char* player_get_animation_name(const Player& player);

}
