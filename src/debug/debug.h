#pragma once

#include "types.h"

namespace Debug {

extern bool overlay_enabled;

void toggle_overlay();
void handle_debug_keys();
void render_overlay(Vec2 mouse_pixel);

// Geometry editor mouse handling (returns true if click was consumed by editor)
bool handle_mouse_click(Vec2 mouse_pixel);
bool handle_mouse_right_click(Vec2 mouse_pixel);
void handle_mouse_release();

// Load geometry for current scene (call after scene load)
void load_scene_geometry();

}
