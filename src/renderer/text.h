#pragma once

#include "types.h"
#include "renderer.h"

namespace Renderer {

constexpr float CHAR_SPACING = 0.80f;

void init_text_renderer();
// max_chars: number of characters to render (-1 = all)
void render_text(const char* text, Vec2 pos, float scale = 1.0f, Vec4 color = Vec4(1,1,1,1), int max_chars = -1);
float calculate_text_width(const char* text, float scale = 1.0f);

}
