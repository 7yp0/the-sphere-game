#pragma once

#include <cstdint>

namespace Config {

// Base resolution: logical game world resolution
// Game logic and positioning uses this coordinate space
constexpr uint32_t BASE_WIDTH = 336;
constexpr uint32_t BASE_HEIGHT = 189;

// Viewport and rendering resolution
// Must match expected scene dimensions for correct coordinate system behavior
constexpr uint32_t VIEWPORT_WIDTH = 672;
constexpr uint32_t VIEWPORT_HEIGHT = 378;

// Depth scaling (2.5D effect)
// horizon_y: pixel Y-coordinate where character scale = 1.0
// Default: middle of screen where normal scale applies
constexpr float DEFAULT_HORIZON_Y = 189.0f;

// Depth scale gradient: how much scale changes per pixel of distance from horizon
// Formula: scale = 1.0 + (horizon_y - sprite_y) * DEPTH_GRADIENT
// 0.002 means 100 pixels above horizon → scale = 0.8 (20% smaller)
// 0.001 means 100 pixels above horizon → scale = 0.9 (10% smaller)
constexpr float DEPTH_GRADIENT = 0.001f;

// Depth scale limits
constexpr float SCALE_MIN = 0.01f;
constexpr float SCALE_MAX = 2.0f;

// Window title
constexpr const char* WINDOW_TITLE = "The Sphere Game";

}
