#pragma once

#include <cstdint>

namespace Config {

// Viewport and rendering resolution
// Must match expected scene dimensions for correct coordinate system behavior
constexpr uint32_t VIEWPORT_WIDTH = 512;
constexpr uint32_t VIEWPORT_HEIGHT = 360;

// Window title
constexpr const char* WINDOW_TITLE = "The Sphere Game";

}
