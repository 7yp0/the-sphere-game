#pragma once

#include "types.h"

namespace Debug {

extern bool overlay_enabled;

void toggle_overlay();
void render_overlay(Vec2 mouse_pixel, uint32_t viewport_width, uint32_t viewport_height);

}
