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

// Input APIs
Vec2 get_mouse_pos();
bool mouse_clicked();

}
