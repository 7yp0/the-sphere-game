#pragma once
#include "ui.h"
#include "../types.h"
#include "ecs/ecs.h"
#include <string>

namespace UI {

    // Scaled accessor (UI-FBO)
    inline float bubble_padding() { return UI::BASE_BACKDROP_PADDING * UI::UI_SCALE(); }

void show_speechbubble(ECS::EntityID entity, const std::string& text, const Vec4& color, float duration = 2.0f);
void render_speechbubbles();
void clear_speechbubbles();

} // namespace UI
