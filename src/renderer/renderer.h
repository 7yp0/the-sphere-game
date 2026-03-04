#pragma once

#include <cstdint>

namespace Renderer {

using GLuint = uint32_t;

void init_renderer(uint32_t width, uint32_t height);
void clear_screen();
void render_quad();
void shutdown();

}
