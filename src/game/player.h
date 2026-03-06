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

constexpr float PLAYER_SPEED = 300.0f;

struct Player {
    Vec2 position;                              // Pixel coordinates in scene space (0 to scene.width/height)
    Vec2 target_position;                       // Pixel coordinates in scene space
    Vec2 size = Vec2(30.0f, 30.0f);             // Sprite dimensions in pixels
    float speed = PLAYER_SPEED;                 // pixels per second
    AnimationState animation_state = AnimationState::Idle;
    Core::AnimationBank* animations;            // Generic animation bank for any entity
    PivotPoint pivot = PivotPoint::BOTTOM_CENTER;
};

void player_init(Player& player, uint32_t viewport_width, uint32_t viewport_height, 
                 Core::AnimationBank* animations);

void player_handle_input(Player& player);

void player_update(Player& player, uint32_t viewport_width, uint32_t viewport_height, float delta_time);

}
