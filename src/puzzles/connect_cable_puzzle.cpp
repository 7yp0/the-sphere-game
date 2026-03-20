#include "connect_cable_puzzle.h"
#include "renderer/renderer.h"
#include "platform.h"
#include "types.h"
#include <cmath>

namespace Puzzles {

static constexpr float Z_CABLE  = -0.85f;
static constexpr float Z_SOCKET = -0.84f;
static constexpr float Z_HANDLE = -0.86f;

static const Vec4 COPPER      (0.72f, 0.45f, 0.20f, 1.0f);
static const Vec4 SOCKET_DARK (0.30f, 0.30f, 0.30f, 1.0f);
static const Vec4 SOCKET_RIM  (0.55f, 0.50f, 0.45f, 1.0f);
static const Vec4 HANDLE_COLOR(0.85f, 0.55f, 0.25f, 1.0f);

static float vdist(Vec2 a, Vec2 b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

static Vec2 bezier(Vec2 a, Vec2 ctrl, Vec2 b, float t) {
    float mt = 1.0f - t;
    return Vec2(mt * mt * a.x + 2 * mt * t * ctrl.x + t * t * b.x,
                mt * mt * a.y + 2 * mt * t * ctrl.y + t * t * b.y);
}

// ============================================================================

void ConnectCablePuzzle::init(const ConnectCableConfig& config) {
    m_config    = config;
    m_complete  = false;
    m_dragging  = -1;
}

void ConnectCablePuzzle::update(Vec2 mouse_pos) {
    if (m_complete) return;

    bool mouse_down = Platform::mouse_down();

    // Start drag on click near a free end
    if (m_dragging < 0 && Platform::mouse_clicked()) {
        if (!m_config.cable_a.connected && vdist(mouse_pos, m_config.cable_a.free_pos) < 10.0f)
            m_dragging = 0;
        else if (!m_config.cable_b.connected && vdist(mouse_pos, m_config.cable_b.free_pos) < 10.0f)
            m_dragging = 1;
    }

    // Move dragged end
    if (m_dragging == 0) m_config.cable_a.free_pos = mouse_pos;
    if (m_dragging == 1) m_config.cable_b.free_pos = mouse_pos;

    // On release: snap to socket if close enough
    if (!mouse_down && m_dragging >= 0) {
        ConnectCable& c = (m_dragging == 0) ? m_config.cable_a : m_config.cable_b;
        if (vdist(c.free_pos, c.socket_pos) < c.socket_radius) {
            c.connected = true;
            c.free_pos  = c.socket_pos;
        }
        m_dragging = -1;
    }

    // Complete when both connected
    if (m_config.cable_a.connected && m_config.cable_b.connected && !m_complete) {
        m_complete = true;
        if (m_config.on_both_connected) m_config.on_both_connected();
    }
}

void ConnectCablePuzzle::render_socket(Vec2 pos, float r) const {
    Renderer::render_rounded_rect(
        Vec3(pos.x, pos.y, Z_SOCKET),
        Vec2(r * 2.2f, r * 2.2f), SOCKET_DARK, r * 1.1f, PivotPoint::CENTER);
    Renderer::render_rounded_rect(
        Vec3(pos.x, pos.y, Z_SOCKET - 0.001f),
        Vec2(r * 1.2f, r * 1.2f), SOCKET_RIM,  r * 0.6f, PivotPoint::CENTER);
}

void ConnectCablePuzzle::render_cable(const ConnectCable& cable) const {
    Vec2 a    = cable.anchor_pos;
    Vec2 b    = cable.connected ? cable.socket_pos : cable.free_pos;

    // Sag control point: midpoint, pushed down by distance * sag_factor
    float span = vdist(a, b);
    Vec2  ctrl((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f + span * m_config.sag_factor);

    const int N = 20;
    Vec2 prev = a;
    for (int i = 1; i <= N; i++) {
        float t    = (float)i / (float)N;
        Vec2  next = bezier(a, ctrl, b, t);
        Renderer::render_cable_segment(prev, next, m_config.cable_half_width, COPPER, Z_CABLE);
        prev = next;
    }

    // Free-end handle
    if (!cable.connected) {
        float hw = m_config.cable_half_width + 2.0f;
        Renderer::render_rounded_rect(
            Vec3(b.x, b.y, Z_HANDLE),
            Vec2(hw * 2.0f, hw * 2.0f), HANDLE_COLOR, hw, PivotPoint::CENTER);
    }
}

void ConnectCablePuzzle::render() const {
    // Sockets
    render_socket(m_config.cable_a.socket_pos, m_config.cable_a.socket_radius);
    render_socket(m_config.cable_b.socket_pos, m_config.cable_b.socket_radius);

    // Cables
    render_cable(m_config.cable_a);
    render_cable(m_config.cable_b);
}

} // namespace Puzzles
