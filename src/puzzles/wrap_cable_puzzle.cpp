#include "wrap_cable_puzzle.h"
#include "renderer/renderer.h"
#include "platform.h"
#include "types.h"
#include <cmath>
#include <algorithm>

namespace Puzzles {

static constexpr float Z_CABLE_FRONT = -0.90f;  // more negative = closer to camera = in front
static constexpr float Z_PILLAR      = -0.80f;
static constexpr float Z_CABLE_BACK  = -0.70f;  // less negative = farther from camera = behind

static const Vec4 COPPER(0.72f, 0.45f, 0.20f, 1.0f);

static float vec2_len(Vec2 v) { return std::sqrt(v.x*v.x + v.y*v.y); }

static Vec2 bezier(Vec2 a, Vec2 ctrl, Vec2 b, float t) {
    float mt = 1.0f - t;
    return Vec2(mt*mt*a.x + 2*mt*t*ctrl.x + t*t*b.x,
                mt*mt*a.y + 2*mt*t*ctrl.y + t*t*b.y);
}

// ============================================================================

void WrapCablePuzzle::init(const WrapCableConfig& config) {
    m_config          = config;
    m_free_end        = config.anchor_pos;
    m_wraps.clear();
    m_behind          = 0;
    m_front           = 0;
    m_clockwise       = false;
    m_dragging        = false;
    m_complete        = false;
    m_in_zone         = false;
    m_zone_snap_valid = false;
    m_direction_locked = false;
}

void WrapCablePuzzle::render_sagging_segment(Vec2 a, Vec2 b, float z) const {
    Vec2  diff(b.x - a.x, b.y - a.y);
    float len = vec2_len(diff);
    if (len < 0.5f) return;

    float sag  = len * m_config.sag_factor;
    Vec2  ctrl((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f + sag);

    const int N = 16;
    Vec2 prev = a;
    for (int i = 1; i <= N; i++) {
        float t    = (float)i / (float)N;
        Vec2  next = bezier(a, ctrl, b, t);
        Renderer::render_cable_segment(prev, next, m_config.cable_half_width, COPPER, z);
        prev = next;
    }
}

// ============================================================================

void WrapCablePuzzle::update(Vec2 mouse_pos) {
    if (m_complete) return;

    bool mouse_down = Platform::mouse_down();
    if (!m_dragging && Platform::mouse_clicked()) {
        Vec2 d(mouse_pos.x - m_free_end.x, mouse_pos.y - m_free_end.y);
        if (vec2_len(d) < 10.0f) m_dragging = true;
    }
    if (!mouse_down) m_dragging = false;
    if (!m_dragging) return;

    const float px       = m_config.pillar.position.x;
    const float p_top    = m_config.pillar.position.y - m_config.pillar.height * 0.5f;
    const float p_bottom = m_config.pillar.position.y + m_config.pillar.height * 0.5f;
    const float seg_h    = m_config.cablesegment_height;
    const float mx       = mouse_pos.x;
    const float my       = mouse_pos.y;
    const float l_x      = px - m_config.pillar.radius;  // p_xmiddle - p_w/2
    const float r_x      = px + m_config.pillar.radius;  // p_xmiddle + p_w/2

    // Prevent snap if mouse is on the edge or on the rounded top of the pillar
    // The rounded area is between (p_top - radius) and (p_top + radius)
    const float edge_epsilon = 2.0f;
    const float round_top_min = p_top - m_config.pillar.radius / 2.0f;
    const float round_top_max = p_top + m_config.pillar.radius / 2.0f;
    if (mx > l_x && mx < r_x && ((my >= round_top_min && my <= round_top_max) || std::abs(my - p_top) < edge_epsilon)) {
        m_in_zone = false;
        m_zone_snap_valid = false;
        // Free end always follows mouse, but no snap/zone logic
        m_free_end = mouse_pos;
        return;
    }

    int  winds     = std::min(m_behind, m_front);
    bool half_wind = (m_behind != m_front);

    // Snap positions: for CW r_snap is lower (p_bottom), l_snap one step higher.
    // For CCW l_snap is lower, r_snap one step higher.
    Vec2 l_snap_pos, r_snap_pos;
    if (m_clockwise) {
        // CW: r_snap at bottom, l_snap one step higher (cable crosses \ on pillar face)
        r_snap_pos = Vec2(r_x, p_bottom - seg_h * (float)winds);
        l_snap_pos = Vec2(l_x, p_bottom - seg_h * (float)(winds + 1));
    } else {
        // CCW: r_snap at bottom, l_snap one step higher
        r_snap_pos = Vec2(r_x, p_bottom - seg_h * (float)winds);
        l_snap_pos = Vec2(l_x, p_bottom - seg_h * (float)(winds + 1));
    }

    // Snap helpers: sequential if-override (first sets candidate, second overrides)
    auto snap_for_right = [&]() -> WrapPoint {
        WrapPoint wp; wp.side = 0; wp.z = 0.0f;
        if (mx > l_x) { wp.position = l_snap_pos; wp.side = -1; }
        if (mx > r_x) { wp.position = r_snap_pos; wp.side = +1; }
        return wp;
    };
    auto snap_for_left = [&]() -> WrapPoint {
        WrapPoint wp; wp.side = 0; wp.z = 0.0f;
        if (mx < r_x) { wp.position = r_snap_pos; wp.side = +1; }
        if (mx < l_x) { wp.position = l_snap_pos; wp.side = -1; }
        return wp;
    };

    // Determine new zone and live snap position
    WrapPoint new_snap; new_snap.side = 0; new_snap.z = 0.0f;
    bool new_in_zone = false;

    if (m_clockwise) {
        if (my < p_top && !half_wind) {
            // CW first half: above top → cable goes behind pillar
            new_in_zone = true;
            new_snap    = snap_for_right();
            new_snap.z  = Z_CABLE_BACK;
        } else if (my > p_top && half_wind) {
            // CW second half: below top → cable comes in front
            new_in_zone = true;
            new_snap    = snap_for_left();
            new_snap.z  = Z_CABLE_FRONT;
        }
    } else {
        if (my > p_top && !half_wind) {
            // CCW first half: below top → cable goes in front
            new_in_zone = true;
            new_snap    = snap_for_right();
            new_snap.z  = Z_CABLE_FRONT;
        } else if (my < p_top && half_wind) {
            // CCW second half: above top → cable goes behind
            new_in_zone = true;
            new_snap    = snap_for_left();
            new_snap.z  = Z_CABLE_BACK;
        }
    }

    bool new_snap_valid = new_in_zone && (new_snap.side != 0);

    // Lock snap on zone EXIT (was in zone last frame, no longer in zone)
    if (m_in_zone && !new_in_zone && m_zone_snap_valid) {
        m_wraps.push_back(m_zone_snap);
        // Lock direction only after first snap is set
        if (!m_direction_locked) {
            m_direction_locked = true;
        }
        if (m_zone_snap.z == Z_CABLE_BACK) {
            m_behind++;
        } else {
            m_front++;
        }
        int w = std::min(m_behind, m_front);
        if (w >= m_config.required_winds && !m_complete) {
            m_complete = true;
            if (m_config.on_complete) m_config.on_complete();
        }
    }

    // Direction detection: lock only when first valid snap is set
    // CW:  mouse enters top-left  (above pillar top AND left of pillar)
    // CCW: mouse enters bottom-right (below pillar top AND right of pillar)
    if (!m_direction_locked) {
        if (my < p_top && mx < l_x) {
            m_clockwise = true;
        } else if (my > p_top && mx > r_x) {
            m_clockwise = false;
        }
    }

    // Update zone state
    m_in_zone         = new_in_zone;
    m_zone_snap       = new_snap;
    m_zone_snap_valid = new_snap_valid;

    // Free end always follows mouse — snap position only locks on zone exit
    m_free_end = mouse_pos;
}

// ============================================================================

void WrapCablePuzzle::render() const {
    const Pillar& pillar   = m_config.pillar;
    const float   p_top    = pillar.position.y - pillar.height * 0.5f;
    const float   p_bottom = pillar.position.y + pillar.height * 0.5f;
    const float   pw       = pillar.radius * 2.0f;
    const float   ph       = pillar.height;
    const float   l_x      = pillar.position.x - pillar.radius;
    const float   r_x      = pillar.position.x + pillar.radius;
    const float   seg_h    = m_config.cablesegment_height;

    // ---- Pillar ----
    Renderer::render_rect(
        Vec3(pillar.position.x, pillar.position.y, Z_PILLAR),
        Vec2(pw, ph), Vec4(0.52f, 0.48f, 0.44f, 1.0f), PivotPoint::CENTER);
    Renderer::render_rect(
        Vec3(pillar.position.x - pillar.radius + 1.0f, pillar.position.y, Z_PILLAR - 0.001f),
        Vec2(2.0f, ph - 2.0f), Vec4(1,1,1,0.22f), PivotPoint::CENTER);
    Renderer::render_rect(
        Vec3(pillar.position.x + pillar.radius - 2.0f, pillar.position.y, Z_PILLAR - 0.001f),
        Vec2(2.0f, ph - 2.0f), Vec4(0,0,0,0.22f), PivotPoint::CENTER);
    Renderer::render_rounded_rect(
        Vec3(pillar.position.x, p_top, Z_PILLAR - 0.002f),
        Vec2(pw * 1.25f, pw / 2), Vec4(0.58f, 0.54f, 0.50f, 1.0f), pw * 0.5f, PivotPoint::CENTER);

    // ---- Wound cable marks (one diagonal segment per complete wind) ----
    for (int i = 0; i + 1 < (int)m_wraps.size(); i += 2) {
        if (m_clockwise && m_wraps[i + 1].side == -1) {
            // CW: second snap was l_snap (left of pillar) → draw "\"
            Vec2 a(r_x, m_wraps[i].position.y);
            Vec2 b(l_x, m_wraps[i + 1].position.y);
            Renderer::render_cable_segment(a, b, m_config.cable_half_width, COPPER, Z_CABLE_FRONT);
        }
    }

    // ---- Cable segments (anchor → locked snaps → free end) ----
    bool  half_wind = (m_behind != m_front);
    float active_z  = half_wind ? Z_CABLE_BACK : Z_CABLE_FRONT;
    // While in zone: use zone snap's z for the last (active) segment
    float last_z    = m_zone_snap_valid ? m_zone_snap.z : active_z;

    std::vector<Vec2> points;
    points.push_back(m_config.anchor_pos);
    for (const auto& w : m_wraps) points.push_back(w.position);
    points.push_back(m_free_end);

    int total = (int)points.size();
    for (int i = 0; i + 1 < total; i++) {
        bool  is_last = (i + 2 == total);
        float z       = is_last ? last_z : m_wraps[i].z;
        render_sagging_segment(points[i], points[i + 1], z);
    }

    // ---- Free-end handle ----
    if (!m_complete) {
        float hw = m_config.cable_half_width + 2.0f;
        Renderer::render_rounded_rect(
            Vec3(m_free_end.x, m_free_end.y, Z_CABLE_FRONT - 0.001f),
            Vec2(hw * 2.0f, hw * 2.0f),
            Vec4(0.85f, 0.55f, 0.25f, 1.0f),
            hw, PivotPoint::CENTER);
    }
}

} // namespace Puzzles
