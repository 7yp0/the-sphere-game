#pragma once

#include "types.h"

namespace UI {

// Initialize main menu (call once after renderer is ready)
void init_main_menu();

// Update main menu — handle mouse hover and click.
// Returns true if the menu consumed input (always true while menu is open).
bool update_main_menu(Vec2 mouse_pos);

// Call when gameplay has started at least once so ESC can resume.
void set_can_resume(bool can_resume);

// Call immediately after switching to MAIN_MENU via ESC to suppress the
// close-on-same-frame race condition.
void mark_just_opened();

// Render main menu overlay into the current UI-FBO.
void render_main_menu();

}
