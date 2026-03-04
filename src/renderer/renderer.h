#pragma once

#include <cstdint>
#include "types.h"

namespace Renderer {

using GLuint = uint32_t;
using TextureID = uint32_t;

void init_renderer(uint32_t width, uint32_t height);
void clear_screen();
void render_sprite(TextureID tex, Vec2 pos, Vec2 size);
void shutdown();

}
