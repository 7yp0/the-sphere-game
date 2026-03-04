#pragma once

#include <cstdint>

namespace Platform {

struct WindowConfig {
    uint32_t width = 800;
    uint32_t height = 600;
    const char* title = "The Sphere Game";
};

bool init_window(const WindowConfig& config);
void swap_buffers();
void clear_screen();
void init_renderer();
void render_quad();  
bool window_should_close();
void shutdown();

}
