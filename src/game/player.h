#pragma once

#include "game.h"
#include <cstdint>

namespace Game {

void player_init(Player& player, uint32_t viewport_width, uint32_t viewport_height, 
                 Core::AnimationBank* animations);

void player_handle_input(Player& player);

void player_update(Player& player, uint32_t viewport_width, uint32_t viewport_height, float delta_time);

Vec2 player_get_render_position(const Player& player, uint32_t viewport_width, uint32_t viewport_height);

}
