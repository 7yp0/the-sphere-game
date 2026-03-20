#include "dot_connect_puzzle.h"
#include "renderer/renderer.h"
#include "platform.h"
#include "types.h"
#include <cmath>
#include <algorithm>

namespace Puzzles {

static constexpr float Z_LINE     = -0.85f;
static constexpr float Z_NODE     = -0.86f;
static constexpr float Z_NODE_SEL = -0.87f;

static const Vec4 LINE_COLOR    (0.45f, 0.75f, 1.00f, 1.0f);  // light blue
static const Vec4 LINE_PREVIEW  (0.45f, 0.75f, 1.00f, 0.5f);  // faded preview
static const Vec4 NODE_DEFAULT  (0.70f, 0.70f, 0.75f, 1.0f);  // decoy nodes
static const Vec4 NODE_CORRECT  (0.30f, 0.85f, 0.40f, 1.0f);  // correct nodes: green
static const Vec4 NODE_SELECTED (0.80f, 0.90f, 1.00f, 1.0f);
static const Vec4 NODE_OUTLINE  (0.45f, 0.75f, 1.00f, 1.0f);

static float vdist(Vec2 a, Vec2 b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

// ============================================================================

void DotConnectPuzzle::init(const DotConnectConfig& config) {
    m_config   = config;
    m_drawn.clear();
    m_selected = -1;
    m_complete = false;
}

int DotConnectPuzzle::node_at(Vec2 pos) const {
    for (int i = 0; i < (int)m_config.nodes.size(); i++) {
        if (vdist(pos, m_config.nodes[i].position) <= m_config.nodes[i].radius + 3.0f)
            return i;
    }
    return -1;
}

bool DotConnectPuzzle::check_complete() const {
    std::set<std::pair<int,int>> required;
    for (const auto& c : m_config.required_connections)
        required.insert({std::min(c.first, c.second), std::max(c.first, c.second)});
    return m_drawn == required;
}

void DotConnectPuzzle::update(Vec2 mouse_pos) {
    m_mouse_pos = mouse_pos;
    if (m_complete) return;
    if (!Platform::mouse_clicked()) return;

    int hit = node_at(mouse_pos);

    if (hit < 0) {
        m_selected = -1;
        return;
    }

    if (m_selected < 0) {
        // Nothing selected → select this node
        m_selected = hit;
    } else if (m_selected == hit) {
        // Click same node → deselect
        m_selected = -1;
    } else {
        // Different node selected → toggle connection
        auto key = std::make_pair(std::min(m_selected, hit), std::max(m_selected, hit));
        if (m_drawn.count(key))
            m_drawn.erase(key);
        else
            m_drawn.insert(key);
        m_selected = hit;  // keep drawing from this node

        if (check_complete()) {
            m_complete = true;
            if (m_config.on_complete) m_config.on_complete();
        }
    }
}

void DotConnectPuzzle::render() const {
    // Build set of correct node indices (those that appear in any required connection)
    std::set<int> correct_nodes;
    for (const auto& c : m_config.required_connections) {
        correct_nodes.insert(c.first);
        correct_nodes.insert(c.second);
    }

    // ---- Drawn lines ----
    for (const auto& conn : m_drawn) {
        const Vec2& a = m_config.nodes[conn.first].position;
        const Vec2& b = m_config.nodes[conn.second].position;
        Renderer::render_cable_segment(a, b, 0.8f, LINE_COLOR, Z_LINE);
    }

    // ---- Preview line from selected node to mouse ----
    if (m_selected >= 0) {
        const Vec2& a = m_config.nodes[m_selected].position;
        Renderer::render_cable_segment(a, m_mouse_pos, 0.8f, LINE_PREVIEW, Z_LINE);
    }

    // ---- Nodes ----
    for (int i = 0; i < (int)m_config.nodes.size(); i++) {
        const DotNode& n   = m_config.nodes[i];
        bool           sel = (i == m_selected);
        bool           cor = correct_nodes.count(i) > 0;
        float          r   = n.radius;

        // Outline ring when selected
        if (sel) {
            Renderer::render_rounded_rect(
                Vec3(n.position.x, n.position.y, Z_NODE_SEL + 0.001f),
                Vec2((r + 2.5f) * 2.0f, (r + 2.5f) * 2.0f),
                NODE_OUTLINE, (r + 2.5f), PivotPoint::CENTER);
        }

        Vec4 color = sel ? NODE_SELECTED : (cor ? NODE_CORRECT : NODE_DEFAULT);
        Renderer::render_rounded_rect(
            Vec3(n.position.x, n.position.y, Z_NODE_SEL),
            Vec2(r * 2.0f, r * 2.0f),
            color, r, PivotPoint::CENTER);
    }
}

} // namespace Puzzles
