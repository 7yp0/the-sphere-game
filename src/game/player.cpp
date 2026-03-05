#include "player.h"
#include "game.h"
#include "platform/mac/mac_window.h"
#include "types.h"
#include "config.h"
#include <cmath>
#include <algorithm>

namespace Game {

const float PLAYER_DISTANCE_THRESHOLD = 1.0f;
const float PLAYER_BOUNDARY_MARGIN = 10.0f;  // Keep player away from scene edges

void player_init(Player& player, uint32_t viewport_width, uint32_t viewport_height, 
                 Core::AnimationBank* animations) {
    // Spawn player at center of scene
    player.position = Vec2(viewport_width * 0.5f, viewport_height * 0.5f);
    player.target_position = Vec2(viewport_width * 0.5f, viewport_height * 0.5f);
    player.speed = PLAYER_SPEED;
    player.animation_state = AnimationState::Idle;
    player.animations = animations;
    // size is already initialized in Player struct
}

void player_handle_input(Player& player) {
    if (Platform::mouse_clicked()) {
        Vec2 mouse_pos = Platform::get_mouse_pos();
        // Store target position - bounds checking happens in player_update()
        player.target_position = mouse_pos;
    }
}

static void clamp_player_position(Player& player, uint32_t viewport_width, uint32_t viewport_height) {
    const float left = PLAYER_BOUNDARY_MARGIN;
    const float right = viewport_width - PLAYER_BOUNDARY_MARGIN;
    const float top = PLAYER_BOUNDARY_MARGIN;
    const float bottom = viewport_height - PLAYER_BOUNDARY_MARGIN;
    
    player.position.x = std::max(left, std::min(right, player.position.x));
    player.position.y = std::max(top, std::min(bottom, player.position.y));
}

void player_update(Player& player, uint32_t viewport_width, uint32_t viewport_height, float delta_time) {
    Vec2 direction = Vec2(
        player.target_position.x - player.position.x,
        player.target_position.y - player.position.y
    );
    
    float distance_sq = direction.x * direction.x + direction.y * direction.y;
    float distance = std::sqrt(distance_sq);
    bool is_moving = distance > PLAYER_DISTANCE_THRESHOLD;
    
    AnimationState new_state = is_moving ? AnimationState::Walking : AnimationState::Idle;
    if (new_state != player.animation_state) {
        player.animation_state = new_state;
        if (player.animation_state == AnimationState::Idle && player.animations) {
            Renderer::SpriteAnimation* idle_anim = player.animations->get("idle");
            if (idle_anim) {
                Renderer::animation_reset(idle_anim);
            }
        } else if (player.animation_state == AnimationState::Walking && player.animations) {
            Renderer::SpriteAnimation* walk_anim = player.animations->get("walk");
            if (walk_anim) {
                Renderer::animation_reset(walk_anim);
            }
        }
    }
    
    const char* anim_name = (player.animation_state == AnimationState::Idle) ? "idle" : "walk";
    Renderer::SpriteAnimation* current_anim = 
        (player.animations) ? player.animations->get(anim_name) : nullptr;
    if (current_anim) {
        Renderer::animate(current_anim, delta_time);
    }
    
    if (is_moving) {
        direction.x /= distance;
        direction.y /= distance;
        
        Vec2 movement = Vec2(
            direction.x * player.speed * delta_time,
            direction.y * player.speed * delta_time
        );
        
        player.position.x += movement.x;
        player.position.y += movement.y;
        clamp_player_position(player, viewport_width, viewport_height);
    }
}

}
