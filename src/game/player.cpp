#include "player.h"
#include "game.h"
#include "platform/mac/mac_window.h"
#include "types.h"
#include "config.h"
#include "renderer/texture_loader.h"
#include "renderer/spritesheet_utils.h"
#include <cmath>
#include <algorithm>

namespace Game {

const float PLAYER_DISTANCE_THRESHOLD = 1.0f;
const float PLAYER_BOUNDARY_MARGIN = 10.0f;  // Keep player away from scene edges

void player_init(Player& player, uint32_t viewport_width, uint32_t viewport_height, 
                 Core::AnimationBank* animations) {
    // Load player spritesheet
    Renderer::TextureID player_sprite_map = Renderer::load_texture("player/player_spritesheet.png");
    
    // Calculate UV coordinates for all 4 frames in a 4-column, 1-row grid
    auto uv_frames = Renderer::create_uv_grid(4, 1);
    
    // Idle animation - frame 0 only
    Renderer::SpriteAnimation idle_anim;
    idle_anim.texture = player_sprite_map;
    idle_anim.frames = { uv_frames[0] };
    idle_anim.frame_duration = 1.0f;
    idle_anim.elapsed_time = 0.0f;
    idle_anim.current_frame = 0;
    animations->add("idle", idle_anim);
    
    // Walk animation - frames 1, 2, 3
    Renderer::SpriteAnimation walk_anim;
    walk_anim.texture = player_sprite_map;
    walk_anim.frames = { uv_frames[1], uv_frames[2], uv_frames[3] };
    walk_anim.frame_duration = 0.2f;
    walk_anim.elapsed_time = 0.0f;
    walk_anim.current_frame = 0;
    animations->add("walk", walk_anim);
    
    // Initialize player entity
    player.position = Vec2(viewport_width * 0.5f, viewport_height * 0.5f);
    player.target_position = Vec2(viewport_width * 0.5f, viewport_height * 0.5f);
    player.speed = PLAYER_SPEED;
    player.animation_state = AnimationState::Idle;
    player.animations = animations;
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
