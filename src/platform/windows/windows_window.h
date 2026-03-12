#pragma once

#include <cstdint>
#include "types.h"
#include "config.h"

namespace Platform {

struct WindowConfig {
    uint32_t width = Config::VIEWPORT_WIDTH;
    uint32_t height = Config::VIEWPORT_HEIGHT;
    const char* title = Config::WINDOW_TITLE;
};

bool init_window(const WindowConfig& config);
void swap_buffers();
bool window_should_close();
void shutdown();

Vec2 get_mouse_pos();
uint32_t get_window_width();
uint32_t get_window_height();
bool mouse_clicked();
bool mouse_right_clicked();
bool mouse_down();
float scroll_delta();  // Returns scroll delta Y (positive = up, negative = down), consume-on-read
bool key_pressed(int key_code);
bool shift_down();     // Returns true while Shift key is held

void set_mouse_pos(Vec2 pos);
void set_mouse_clicked(bool clicked);
void set_mouse_right_clicked(bool clicked);
void set_mouse_down(bool down);
void set_scroll_delta(float delta);
void set_key_pressed(int key_code, bool pressed);

}
