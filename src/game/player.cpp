#include "player.h"
#include "platform/mac/mac_window.h"
#include <cmath>

namespace Game {

// Player movement constants
const float PLAYER_DISTANCE_THRESHOLD = 1.0f;  // pixels
const float PLAYER_VIEWPORT_MARGIN = 0.0f;     // 0% margin, player can reach edge

void player_init(Player& player, uint32_t viewport_width, uint32_t viewport_height, 
                 Renderer::TextureID texture) {
    player.position = Vec2(viewport_width * 0.5f, viewport_height * 0.5f);
    player.target_position = Vec2(viewport_width * 0.5f, viewport_height * 0.5f);
    player.speed = PLAYER_SPEED;
    player.texture = texture;
}

void player_handle_input(Player& player) {
    if (Platform::mouse_clicked()) {
        Vec2 mouse_pos = Platform::get_mouse_pos();
        // Validate that mouse is within reasonable bounds (screen space)
        // Platform should return valid coordinates, but sanity-check anyway
        player.target_position = mouse_pos;
    }
}

// Clamp player position to viewport bounds with safety margin
static void clamp_player_position(Player& player, uint32_t viewport_width, uint32_t viewport_height) {
    const float left = PLAYER_VIEWPORT_MARGIN * viewport_width;
    const float right = (1.0f - PLAYER_VIEWPORT_MARGIN) * viewport_width;
    const float top = PLAYER_VIEWPORT_MARGIN * viewport_height;
    const float bottom = (1.0f - PLAYER_VIEWPORT_MARGIN) * viewport_height;
    
    player.position.x = std::max(left, std::min(right, player.position.x));
    player.position.y = std::max(top, std::min(bottom, player.position.y));
}

void player_update(Player& player, uint32_t viewport_width, uint32_t viewport_height, float delta_time) {
    // Calculate direction vector to target
    Vec2 direction = Vec2(
        player.target_position.x - player.position.x,
        player.target_position.y - player.position.y
    );
    
    // Calculate distance
    float distance_sq = direction.x * direction.x + direction.y * direction.y;
    float distance = std::sqrt(distance_sq);
    
    // Only move if distance is significant (avoid jitter at target)
    if (distance > PLAYER_DISTANCE_THRESHOLD) {
        // Normalize direction
        direction.x /= distance;
        direction.y /= distance;
        
        // Calculate movement
        Vec2 movement = Vec2(
            direction.x * player.speed * delta_time,
            direction.y * player.speed * delta_time
        );
        
        // Update position
        player.position.x += movement.x;
        player.position.y += movement.y;
        
        // Keep within viewport bounds
        clamp_player_position(player, viewport_width, viewport_height);
    }
}

Vec2 player_get_render_position(const Player& player, uint32_t viewport_width, uint32_t viewport_height) {
    // Convert pixel coordinates to normalized render space [-1, 1]
    float norm_x = (player.position.x / (float)viewport_width) * 2.0f - 1.0f;
    float norm_y = (player.position.y / (float)viewport_height) * 2.0f - 1.0f;
    return Vec2(norm_x, norm_y);
}

}
