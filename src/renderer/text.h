#pragma once

#include "types.h"
#include "renderer.h"

namespace Renderer {

constexpr float CHAR_SPACING = 0.80f;

void init_text_renderer();
void render_text(const char* text, Vec2 pos, float scale = 1.0f);

}
