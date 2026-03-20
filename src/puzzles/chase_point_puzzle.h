#pragma once

#include "types.h"
#include <functional>
#include <vector>

namespace Puzzles {

struct ChasePointConfig {
    std::vector<Vec2> waypoints;   // sequence of positions the dot visits (index 0 = start)
    float dot_radius   = 4.0f;
    float hover_radius = 14.0f;   // hover detection: dot_radius + safety padding
    std::function<void()> on_complete;
};

class ChasePointPuzzle {
public:
    void init(const ChasePointConfig& config);
    void update(Vec2 mouse_pos);
    void render() const;

    bool is_complete() const { return m_complete; }

private:
    ChasePointConfig m_config;
    int  m_current   = 0;
    bool m_complete  = false;
    Vec2 m_draw_pos  = Vec2(0, 0);  // current visual position (lerped)
};

} // namespace Puzzles
