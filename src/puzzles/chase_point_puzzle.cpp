#include "chase_point_puzzle.h"
#include "renderer/renderer.h"
#include "core/timing.h"
#include "platform.h"
#include "types.h"
#include <cmath>

namespace Puzzles {

static constexpr float Z_TRAIL = -0.84f;
static constexpr float Z_DOT   = -0.85f;

static const Vec4 DOT_COLOR   (0.90f, 0.90f, 1.00f, 1.0f);
static const Vec4 TRAIL_COLOR (0.45f, 0.75f, 1.00f, 0.35f);
static const Vec4 VISITED_COLOR(0.45f, 0.75f, 1.00f, 0.50f);

static float vdist(Vec2 a, Vec2 b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

static constexpr float LERP_SPEED = 0.8f;  // higher = faster

void ChasePointPuzzle::init(const ChasePointConfig& config) {
    m_config   = config;
    m_current  = 0;
    m_complete = false;
    if (!config.waypoints.empty())
        m_draw_pos = config.waypoints[0];
}

void ChasePointPuzzle::update(Vec2 mouse_pos) {
    if (m_config.waypoints.empty()) return;

    int last = (int)m_config.waypoints.size() - 1;
    const Vec2& target = m_config.waypoints[std::min(m_current, last)];

    // Lerp visual position toward current target
    float dt = Core::get_delta_time();
    float t  = 1.0f - std::exp(-LERP_SPEED * dt);
    m_draw_pos.x += (target.x - m_draw_pos.x) * t;
    m_draw_pos.y += (target.y - m_draw_pos.y) * t;

    if (m_complete) return;

    // Completion: dot has visually arrived at the last waypoint
    if (m_current == last && vdist(m_draw_pos, target) < 2.0f) {
        m_complete = true;
        if (m_config.on_complete) m_config.on_complete();
        return;
    }

    // Hover advances to next waypoint (last waypoint needs no hover — just arrive)
    if (m_current < last && vdist(mouse_pos, target) <= m_config.hover_radius) {
        m_current++;
    }
}

void ChasePointPuzzle::render() const {
    // ---- Active dot (uses lerped visual position) ----
    if (!m_complete) {
        float r = m_config.dot_radius;
        Renderer::render_rounded_rect(
            Vec3(m_draw_pos.x, m_draw_pos.y, Z_DOT),
            Vec2(r * 2.0f, r * 2.0f), DOT_COLOR, r, PivotPoint::CENTER);
    }
}

} // namespace Puzzles
