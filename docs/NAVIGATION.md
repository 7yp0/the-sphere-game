# Navigation & Movement System

This document describes the navigation and movement system used for point-and-click character movement in The Sphere Game.

## Overview

The navigation system enables characters to move through the game world by clicking on destinations. When the player clicks, the system:

1. **Resolves** the click point to a valid walkable destination
2. **Checks** for direct line-of-sight to the target
3. **Computes** a path around obstacles if needed (using visibility graph + A*)
4. **Follows** the computed path at a constant speed

## Why The Old System Was Replaced

The previous implementation used an **edge-following/sliding** approach:

### Problems with the Old Sliding System

1. **Complexity**: The sliding logic was ~550 lines of intricate code with multiple edge cases
2. **Stuck Detection**: Required a "stuck timer" and "emergency escape" mechanisms - clear signs of instability
3. **Local Decision Making**: Made decisions locally at each collision, often leading to suboptimal paths
4. **Oscillation**: Could get stuck oscillating between states at polygon corners
5. **Edge Transitions**: Complex vertex-to-vertex transitions caused unpredictable behavior
6. **Hard to Debug**: The state machine with edge-following, vertex transitions, and stuck recovery was difficult to reason about

### Benefits of the New System

1. **Simplicity**: Core logic is ~200 lines, much easier to understand and maintain
2. **Global Planning**: Computes the entire path upfront, no local decision making during movement
3. **Predictable**: Always follows the same path for the same start/end points
4. **No Stuck States**: If a path exists, it will be found; if not, movement doesn't start
5. **Easy to Debug**: Path waypoints can be visualized, behavior is deterministic

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         User Click                                   │
└────────────────────────────┬────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    Target Resolution                                 │
│  • Click in walkable area → use directly                            │
│  • Click outside walkable → project to nearest walkable point       │
│  • Click in obstacle → project to nearest point outside obstacle    │
└────────────────────────────┬────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────┐
│                  Line-of-Sight Check                                 │
│  • Can we walk directly from current position to target?            │
│  • Check: segment stays in walkable area, doesn't cross obstacles   │
└────────────────────────────┬────────────────────────────────────────┘
                             │
              ┌──────────────┴──────────────┐
              │ Direct path?                │
              ▼                             ▼
        ┌─────────┐                  ┌─────────────┐
        │   Yes   │                  │     No      │
        └────┬────┘                  └──────┬──────┘
             │                              │
             │                              ▼
             │              ┌─────────────────────────────────────────┐
             │              │         Visibility Graph                │
             │              │  • Nodes: start, target, obstacle       │
             │              │    vertices + walkable area corners     │
             │              │  • Edges: pairs with clear line-of-     │
             │              │    sight between them                   │
             │              └────────────────┬────────────────────────┘
             │                               │
             │                               ▼
             │              ┌─────────────────────────────────────────┐
             │              │            A* Pathfinding               │
             │              │  • Find shortest path through graph     │
             │              │  • Heuristic: euclidean distance        │
             │              └────────────────┬────────────────────────┘
             │                               │
             └───────────────┬───────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────┐
│                       Path Waypoints                                 │
│  • Ordered list of Vec2 points from start to target                 │
│  • May be single point (direct) or multiple (around obstacles)      │
└────────────────────────────┬────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      Path Following                                  │
│  • Move toward current waypoint at constant speed                   │
│  • When waypoint reached, advance to next                           │
│  • When all waypoints visited, stop                                 │
└─────────────────────────────────────────────────────────────────────┘
```

## Source Files

| File | Purpose |
|------|---------|
| `src/navigation/navigation.h` | Navigation API and data structures |
| `src/navigation/navigation.cpp` | Geometry helpers, visibility graph, A* implementation |
| `src/game/walker_system.h` | Walker system API for ECS integration |
| `src/game/walker_system.cpp` | Path-following movement logic |
| `src/ecs/components/walker.h` | WalkerComponent data structure |
| `src/collision/polygon_utils.h` | Base polygon collision utilities |

## Key Data Structures

### Path (`Navigation::Path`)

```cpp
struct Path {
    std::vector<Vec2> waypoints;    // Ordered waypoints from start to target
    size_t current_index = 0;       // Index of next waypoint to reach
    bool valid = false;             // True if path was successfully computed
};
```

The path stores only the waypoints needed to reach the destination. The start position is NOT included in waypoints (the walker is already there).

### WalkerComponent (`ECS::WalkerComponent`)

```cpp
struct WalkerComponent {
    Vec3 target_position;           // Final destination (including z for depth)
    float speed = 75.0f;            // Pixels per second
    float distance_threshold = 3.0f; // Distance to consider waypoint reached
    
    std::vector<Vec2> path_waypoints;   // Computed path
    size_t current_waypoint_index = 0;  // Current progress
    bool has_valid_path = false;        // Path state
    
    bool wants_to_move = false;         // Movement intent
    Vec2 movement_direction;            // For animation (normalized)
};
```

## API Reference

### walker_set_target

```cpp
void walker_set_target(
    ECS::WalkerComponent& walker,
    Vec2 current_pos,
    Vec3 target,
    const std::vector<Collision::Polygon>& walkable_areas,
    const std::vector<Collision::Polygon>& obstacles
);
```

Sets a new movement target. Internally:

1. Resolves target to valid walkable position
2. Computes path using visibility graph + A*
3. Stores path in walker component
4. Sets `wants_to_move = true` if path found

### walker_update

```cpp
bool walker_update(
    ECS::WalkerComponent& walker,
    ECS::Transform2_5DComponent& transform,
    const std::vector<Collision::Polygon>& walkable_areas,
    const std::vector<Collision::Polygon>& obstacles,
    float delta_time
);
```

Called every frame to move the walker. Returns `true` if still moving, `false` if arrived.

### walker_stop

```cpp
void walker_stop(ECS::WalkerComponent& walker, Vec2 current_pos);
```

Immediately stops movement and clears the current path.

## Algorithm Details

### 1. Target Resolution (`resolve_target`)

The target resolution ensures we always have a valid destination:

```
Input: click_point (may be anywhere on screen)
Output: valid walkable point nearest to click

if is_point_walkable(click_point):
    return click_point

// Click is invalid - find nearest valid point
best_point = click_point
best_distance = infinity

// Strategy 1: Check walkable area boundaries
for each walkable_area:
    closest = closest_point_on_boundary(click_point, walkable_area)
    offset_inside = move_point_toward_center(closest, OFFSET)
    if is_walkable(offset_inside) and distance(click_point, offset_inside) < best_distance:
        best_point = offset_inside
        best_distance = distance(click_point, offset_inside)

// Strategy 2: Check obstacle boundaries (for clicks inside obstacles)
for each obstacle:
    if point_in_polygon(click_point, obstacle):
        closest = closest_point_on_boundary(click_point, obstacle)
        offset_outside = move_point_away_from_center(closest, OFFSET)
        if is_walkable(offset_outside) and distance(click_point, offset_outside) < best_distance:
            best_point = offset_outside
            best_distance = distance(click_point, offset_outside)

return best_point
```

### 2. Line-of-Sight Check (`is_segment_walkable`)

Determines if we can walk directly between two points:

```
Input: start, end
Output: true if direct path is valid

if not is_point_walkable(start): return false
if not is_point_walkable(end): return false

// Check segment doesn't cross any obstacle
for each obstacle:
    if segment_intersects_polygon(start, end, obstacle):
        return false

// Check segment stays inside walkable area (sample multiple points)
for t in [0.2, 0.4, 0.5, 0.6, 0.8]:
    sample = lerp(start, end, t)
    if not is_point_walkable(sample):
        return false

return true
```

### 3. Visibility Graph Construction

The visibility graph connects all points that can "see" each other:

**Nodes:**

- Start position
- Target position (resolved)
- Obstacle vertices (offset slightly outward from obstacle center)
- Walkable area vertices (offset slightly inward)

**Edges:**

- Two nodes are connected if `is_segment_walkable(node_a, node_b)` is true

**Vertex Offset:**
Obstacle vertices are offset slightly (2 pixels) away from the obstacle center. This prevents paths from touching exact obstacle corners, which can cause numerical instability.

```
      Obstacle
    ┌─────────┐
    │    ●────┼──● offset vertex
    │  center │
    └─────────┘
```

### 4. A* Pathfinding

Standard A* algorithm with euclidean distance heuristic:

```
function find_path(start, target):
    open_set = priority_queue (by f_score)
    g_score[all] = infinity
    g_score[start] = 0
    f_score[start] = distance(start, target)
    
    open_set.add(start)
    
    while not open_set.empty():
        current = open_set.pop_lowest_f()
        
        if current == target:
            return reconstruct_path()
        
        for neighbor in adjacency[current]:
            tentative_g = g_score[current] + distance(current, neighbor)
            if tentative_g < g_score[neighbor]:
                came_from[neighbor] = current
                g_score[neighbor] = tentative_g
                f_score[neighbor] = tentative_g + distance(neighbor, target)
                open_set.add(neighbor)
    
    // No path found - return best reachable point
    return path_to_closest_reachable_node()
```

## Geometry Helper Functions

### `segment_intersects_segment`

Uses cross-product orientation test with epsilon tolerance:

```cpp
bool segment_intersects_segment(Vec2 a1, Vec2 a2, Vec2 b1, Vec2 b2);
```

Returns true if segments cross each other (proper intersection). Endpoint touches are NOT counted as intersections to avoid false positives at polygon corners.

### `point_in_convex_polygon`

For convex polygons, a point is inside if it's on the same side of all edges:

```cpp
bool point_in_convex_polygon(Vec2 point, const Polygon& polygon);
```

Uses cross-product sign consistency. Points exactly on edges are considered inside.

### `project_point_to_segment`

Projects a point onto a line segment:

```cpp
Vec2 project_point_to_segment(Vec2 point, Vec2 seg_start, Vec2 seg_end);
```

Returns the closest point on the segment to the given point.

## Edge Cases

### 1. Click Outside Walkable Area

The click is projected to the nearest point on the walkable area boundary, then offset slightly inside.

```
    Walkable Area
    ┌───────────────┐
    │               │
    │      ●───────►X click
    │   resolved    │
    │               │
    └───────────────┘
```

### 2. Click Inside Obstacle

The click is projected to the nearest point on the obstacle boundary, then offset slightly outside.

```
    Walkable Area
    ┌───────────────────┐
    │                   │
    │   ┌─────────┐     │
    │   │Obstacle │     │
    │   │    X────┼──●  │
    │   │  click  │resolved
    │   └─────────┘     │
    │                   │
    └───────────────────┘
```

### 3. Near Polygon Corners

The epsilon tolerance (POINT_EPSILON = 1.0) prevents numerical instability when:

- Player starts very close to a corner
- Path passes near a corner
- Segments nearly touch

### 4. Multiple Obstacles

The visibility graph handles multiple obstacles naturally - paths will route around all of them.

```
    ┌───────────────────────────┐
    │  Start                    │
    │    ●                      │
    │      \                    │
    │   ┌───●───┐   ┌───────┐   │
    │   │Obs 1  │   │Obs 2  │   │
    │   │       │   │       │   │
    │   └───────┘   └───●───┘   │
    │                    \      │
    │                     ● End │
    └───────────────────────────┘
```

### 5. Target Reachable Only via Detour

The A* algorithm finds the shortest valid path even if it requires a significant detour:

```
    ┌───────────────────────────┐
    │                           │
    │  ●Start                   │
    │  │                        │
    │  ├────┐  ┌────────────┐   │
    │  │wall│  │   wall     │   │
    │  │    │  │            │   │
    │  │    │  │            │   │
    │  │    │  └────────────┘   │
    │  │    │                   │
    │  │    └───────────────●End│
    │  │                    │   │
    │  └────────────────────┘   │
    │                           │
    └───────────────────────────┘
```

### 6. No Path Exists

If no complete path exists, the system:

1. Finds the closest reachable node to the target
2. Returns a path to that node
3. If nothing is reachable, returns empty path
4. Walker stays in place (no stuck loops or oscillation)

### 7. Already at Target

If start and target are within `distance_threshold` (3 pixels), no path is computed and `wants_to_move` remains false.

## Configuration Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `EPSILON` | 0.001 | Floating point comparison tolerance |
| `POINT_EPSILON` | 1.0 | Distance to consider points equal |
| `WAYPOINT_REACHED` | 3.0 | Distance to consider waypoint reached |
| `OBSTACLE_OFFSET` | 2.0 | Offset from obstacle vertices |
| `MAX_PATH_NODES` | 256 | Maximum nodes in visibility graph |

## Assumptions & Limitations

### Assumptions

1. **Convex Polygons**: All walkable areas and obstacles MUST be convex. Concave polygons will cause incorrect collision detection.

2. **Point Player**: The player is treated as a point or very small radius. For larger characters, polygon inflation would be needed.

3. **Static Geometry**: Walkable areas and obstacles are static during pathfinding. For dynamic obstacles, path recomputation is needed.

4. **2D Navigation**: Movement is in 2D (X, Y). The Z coordinate is for depth sorting only.

### Limitations

1. **No Polygon Inflation**: Doesn't account for character size. Narrow gaps smaller than character width will appear passable.

2. **Recomputation Needed for Dynamic Changes**: If obstacles move, you must call `walker_recompute_path()`.

3. **Limited Node Count**: Maximum 256 nodes to prevent performance issues with many obstacles.

## Future Extensions

### Adding Player Radius

To support a character radius:

1. Inflate obstacles by radius + margin
2. Shrink walkable areas by radius
3. Use inflated/shrunk polygons for pathfinding
4. Keep original polygons for rendering

### Dynamic Obstacles

To support moving obstacles:

1. Track obstacle positions
2. On obstacle move, call `walker_recompute_path()` for affected walkers
3. Consider partial path invalidation for performance

### Path Visualization (Debug)

To visualize paths:

```cpp
// In debug rendering
if (walker.has_valid_path) {
    Vec2 prev = transform.position;
    for (size_t i = walker.current_waypoint_index; i < walker.path_waypoints.size(); i++) {
        Vec2 next = walker.path_waypoints[i];
        render_debug_line(prev, next, Color(0, 1, 0, 0.5));
        prev = next;
    }
}
```

### Path Smoothing

Current paths follow exact waypoints with sharp corners. For smoother movement:

1. Implement Catmull-Rom spline interpolation
2. Use Bézier curves between waypoints
3. Apply velocity smoothing at corners

## Related Documentation

- [WALKABLE_AREAS.md](WALKABLE_AREAS.md) - Creating walkable areas and obstacles
- [GEOMETRY_EDITOR.md](GEOMETRY_EDITOR.md) - In-game geometry editing tools
- [architecture.md](architecture.md) - Overall game architecture
