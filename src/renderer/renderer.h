#pragma once

#include <cstdint>
#include "types.h"
#include "animation.h"

namespace Renderer {

using GLuint = uint32_t;
using TextureID = uint32_t;

void init_renderer(uint32_t width, uint32_t height);
void clear_screen();
void render_sprite(TextureID tex, Vec2 pos, Vec2 size, float z_depth = 0.0f);
void render_sprite_animated(const SpriteAnimation* anim, Vec2 pos, Vec2 size, float z_depth = 0.0f);
void shutdown();

}
