#pragma once

#include "types.h"
#include <functional>

namespace Puzzles {

// One cable: fixed anchor + draggable free end that snaps to a socket.
struct ConnectCable {
    Vec2  anchor_pos;
    Vec2  free_pos;          // current position of the draggable end
    Vec2  socket_pos;        // target socket
    float socket_radius = 6.0f;
    bool  connected     = false;
};

struct ConnectCableConfig {
    ConnectCable cable_a;
    ConnectCable cable_b;
    float cable_half_width = 1.5f;
    float sag_factor       = 0.25f;
    std::function<void()> on_both_connected;
};

class ConnectCablePuzzle {
public:
    void init(const ConnectCableConfig& config);
    void update(Vec2 mouse_pos);
    void render() const;

    bool is_complete() const { return m_complete; }

private:
    ConnectCableConfig m_config;
    bool m_complete = false;
    int  m_dragging = -1;   // -1=none, 0=cable_a, 1=cable_b

    void render_cable(const ConnectCable& cable) const;
    void render_socket(Vec2 pos, float radius) const;
};

} // namespace Puzzles
