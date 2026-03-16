#pragma once
#include <string>
#include "../types.h"

namespace Tooltip {
    void render(Vec2 mouse_pos, const char* text);
    void set(const std::string& text);
    void clear();
    const std::string& get();
}
