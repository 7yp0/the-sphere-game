#pragma once

#include <cstdint>

namespace Platform {

struct WindowConfig {
    uint32_t width = 800;
    uint32_t height = 600;
    const char* title = "The Sphere Game";
};

// Init Window + OpenGL Context
bool init_window(const WindowConfig& config);

// Swap Buffers (Render Loop)
void swap_buffers();

// Clear Screen
void clear_screen();

// Poll Events
bool window_should_close();

// Cleanup
void shutdown();

} // namespace Platform
