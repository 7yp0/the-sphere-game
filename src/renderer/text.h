#pragma once

#include "types.h"
#include "renderer.h"

namespace Renderer {

constexpr float CHAR_SPACING = 0.80f;

// Glyph cell is 24x32px. Content spans rows 1–28 (28px), 1px gap at top, 3px at bottom.
// GLYPH_VISUAL_CENTER is the average of first and last content row: (1+28)/2 = 14.5
// Use it to perfectly center text inside a background rect:
//   text_pos.y = bg_pos.y + bg_height * 0.5f - GLYPH_VISUAL_CENTER * scale
constexpr float GLYPH_VISUAL_HEIGHT = 28.0f;   // content height (rows 1–28), for bg sizing
constexpr float GLYPH_VISUAL_CENTER = 16.5f;   // visual center row within the 32px cell

void init_text_renderer();
// z: use ZDepth::DIALOGUE_TEXT, TOOLTIP_TEXT, or MAIN_MENU_TEXT depending on context.
// max_chars: number of characters to render (-1 = all)
void render_text(const char* text, Vec2 pos, float scale = 1.0f, Vec4 color = Vec4(1,1,1,1), float z = ZDepth::DIALOGUE_TEXT, int max_chars = -1);
float calculate_text_width(const char* text, float scale = 1.0f);

}
