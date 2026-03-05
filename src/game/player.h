#pragma once

#include "game.h"
#include <cstdint>

namespace Game {

// Initialize player at viewport center
void player_init(Player& player, uint32_t viewport_width, uint32_t viewport_height, 
                 Renderer::TextureID texture);

// Handle player input (mouse clicks to set target position)
void player_handle_input(Player& player);

// Update player position towards target with bounds checking
void player_update(Player& player, uint32_t viewport_width, uint32_t viewport_height, float delta_time);

// Convert pixel position to render space (normalized [-1, 1])
Vec2 player_get_render_position(const Player& player, uint32_t viewport_width, uint32_t viewport_height);

}
