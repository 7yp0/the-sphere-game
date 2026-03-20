#pragma once

#include "types.h"
#include <functional>
#include <vector>

namespace Puzzles {

struct Pillar {
    Vec2  position;  // Zentrum der Säule (Pixelkoordinaten)
    float radius;    // Halbe Breite der Säule in Pixeln
    float height;    // Volle Höhe der Säule in Pixeln
};

struct WrapPoint {
    Vec2  position;
    int   side;  // +1 = right, -1 = left
    float z;     // z-depth for the cable segment arriving at this point
};

struct WrapCableConfig {
    Vec2   anchor_pos;
    Pillar pillar;
    int    required_winds;
    float  cable_half_width      = 1.5f;
    float  sag_factor            = 0.2f;
    float  cablesegment_height   = 6.0f;  // Höhe pro Wicklungs-Segment
    std::function<void()> on_complete;
};

class WrapCablePuzzle {
public:
    void init(const WrapCableConfig& config);
    void update(Vec2 mouse_pos);
    void render() const;

    bool is_complete() const { return m_complete; }
    int  wind_count()  const { return std::min(m_behind, m_front); }

private:
    WrapCableConfig        m_config;
    Vec2                   m_free_end;
    std::vector<WrapPoint> m_wraps;

    int   m_behind           = 0;
    int   m_front            = 0;
    bool  m_clockwise        = false;
    bool  m_direction_locked = false;
    bool  m_dragging         = false;
    bool  m_complete         = false;

    // Zone-based snap state: snap updates live while in zone, locks on zone exit
    bool      m_in_zone          = false;
    WrapPoint m_zone_snap;
    bool      m_zone_snap_valid  = false;

    void render_sagging_segment(Vec2 a, Vec2 b, float z) const;
};

} // namespace Puzzles
