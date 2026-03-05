#pragma once

#include <cstdint>
#include "types.h"

namespace Platform {

struct WindowConfig {
    uint32_t width = 800;
    uint32_t height = 600;
    const char* title = "The Sphere Game";
};

bool init_window(const WindowConfig& config);
void swap_buffers();
bool window_should_close();
void shutdown();

Vec2 get_mouse_pos();
uint32_t get_window_height();
bool mouse_clicked();
bool key_pressed(int key_code);

void set_mouse_pos(Vec2 pos);
void set_mouse_clicked(bool clicked);
void set_key_pressed(int key_code, bool pressed);

}
