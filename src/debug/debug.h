#pragma once

#include "types.h"

namespace Debug {

extern bool overlay_enabled;

void toggle_overlay();
void handle_debug_keys();
void render_overlay(Vec2 mouse_pixel);

}
