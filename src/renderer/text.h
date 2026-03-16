#pragma once

#include "types.h"
#include "renderer.h"

namespace Renderer {

constexpr float CHAR_SPACING = 0.80f;

// Glyph cell is 24x32px. The font is vertically centered in the cell,
// with pixel content spanning rows 0–27 (28px) and ~2px gaps top and bottom.
// Use GLYPH_VISUAL_HEIGHT for background sizing and GLYPH_TOP_PADDING to
// shift the draw position so the visual glyph lands at the center of the background.
constexpr float GLYPH_VISUAL_HEIGHT = 28.0f;  // rows 0–27 contain pixels
constexpr float GLYPH_TOP_PADDING   = 0.0f;   // font is now centered — no top offset needed

void init_text_renderer();
// max_chars: number of characters to render (-1 = all)
void render_text(const char* text, Vec2 pos, float scale = 1.0f, Vec4 color = Vec4(1,1,1,1), int max_chars = -1);
float calculate_text_width(const char* text, float scale = 1.0f);

}
