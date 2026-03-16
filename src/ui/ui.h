#pragma once
#include "config.h"
#include "renderer/renderer.h"
#include <string>
#include <algorithm>

namespace UI {

// Scale factor from base resolution (320x180) to UI-FBO resolution (1280x720)
constexpr float UI_SCALE = static_cast<float>(Config::VIEWPORT_WIDTH) / static_cast<float>(Config::BASE_WIDTH);

// Base padding in logical (320x180) pixels
constexpr float BASE_BACKDROP_PADDING = 5.0f;

}
