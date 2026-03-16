#pragma once
#include "../../types.h"

namespace ECS {

struct TalkableComponent {
    Vec2 bubble_offset = Vec2(0.0f, 0.0f); // Offset der Sprechblase relativ zur Entity
    Vec4 text_color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);    // RGBA-Farbe des Texts
};

} // namespace ECS
