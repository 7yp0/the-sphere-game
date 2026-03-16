#pragma once
#include "config.h"
#include "renderer/renderer.h"
#include <string>
#include <algorithm>

namespace UI {

// Scale factor from base resolution (320x180) to UI-FBO resolution (dynamic letterbox size).
// Always returns the UI FBO dimensions — safe to call outside rendering context (e.g. hit detection).
inline float UI_SCALE() { return (float)Renderer::get_ui_fbo_width() / (float)Config::BASE_WIDTH; }

// Base padding in logical (320x180) pixels
constexpr float BASE_BACKDROP_PADDING = 5.0f;

}
