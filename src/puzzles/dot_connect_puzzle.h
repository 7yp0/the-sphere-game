#pragma once

#include "types.h"
#include <functional>
#include <vector>
#include <set>
#include <utility>

namespace Puzzles {

struct DotNode {
    Vec2  position;
    float radius = 4.0f;
};

struct DotConnectConfig {
    std::vector<DotNode>            nodes;
    std::vector<std::pair<int,int>> required_connections;  // indices into nodes
    std::function<void()>           on_complete;
};

class DotConnectPuzzle {
public:
    void init(const DotConnectConfig& config);
    void update(Vec2 mouse_pos);
    void render() const;

    bool is_complete() const { return m_complete; }

private:
    DotConnectConfig             m_config;
    std::set<std::pair<int,int>> m_drawn;     // currently drawn connections (normalized: a < b)
    int                          m_selected  = -1;
    bool                         m_complete  = false;
    Vec2                         m_mouse_pos = Vec2(0, 0);

    int  node_at(Vec2 pos) const;
    bool check_complete() const;
};

} // namespace Puzzles
