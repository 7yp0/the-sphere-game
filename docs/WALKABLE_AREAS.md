# Walkable Areas & Collision System

Walkable areas define where the player can move in a scene. The system supports complex room shapes and obstacles using polygon-based collision detection.

## Overview

The collision system uses **two types of polygons**:

- **Walkable Areas** (green) = Polygons where the player CAN walk
- **Obstacles** (orange) = Polygons where the player CANNOT walk

A point is walkable if:

1. It's inside ANY walkable area polygon, AND
2. It's NOT inside ANY obstacle polygon

All geometry is stored in `assets/scenes/<scene>/geometry.json`.

> **⚠️ IMPORTANT: All polygons MUST be convex!**
>
> The navigation system assumes convex polygons for efficient collision detection.
> Concave polygons will cause incorrect path computation. Split complex shapes
> into multiple convex polygons. See [NAVIGATION.md](NAVIGATION.md) for details.

## Creating Walkable Areas

### Basic Room Shape

1. Press **D** to open debug overlay
2. Press **W** to enter walkable area creation mode
3. Click around the room perimeter (clockwise or counter-clockwise)
4. Press **F** or click first vertex to close

```
     ┌────────────────────────────┐
     │                            │
     │     WALKABLE AREA          │
     │     (green polygon)        │
     │                            │
     └────────────────────────────┘
```

### Multiple Walkable Areas

Create separate walkable areas for different accessible regions:

```
     ┌────────┐       ┌────────┐
     │        │       │        │
     │  AREA  │       │  AREA  │
     │   1    │       │   2    │  (upper balcony, etc.)
     │        │       │        │
     └────────┘       └────────┘
```

The player can walk within any walkable area. Multiple areas don't need to connect - they're independent.

## Creating Obstacles

To create an **obstacle** (table, pillar, pit) that the player must walk around:

1. Press **O** to enter obstacle creation mode
2. Click around the obstacle perimeter
3. Press **F** or click first vertex to close

```
     ┌────────────────────────────┐  (walkable area - green)
     │                            │
     │     ┌────────┐             │
     │     │OBSTACLE│  ← Table    │  (obstacle - orange)
     │     │(blocked)│            │
     │     └────────┘             │
     │                            │
     └────────────────────────────┘
```

### How It Works

The collision check is simple:

```cpp
bool is_walkable = point_in_any_walkable_area(point) 
                && !point_in_any_obstacle(point);
```

**Example:**

```
Point in walkable area?  → Yes
Point in obstacle?       → No
Result: WALKABLE

Point in walkable area?  → Yes
Point in obstacle?       → Yes (inside table)
Result: NOT WALKABLE
```

### Multiple Obstacles

You can create multiple obstacles within a scene:

```
     ┌────────────────────────────────┐  (walkable area - green)
     │                                │
     │     ┌────┐      ┌────┐        │
     │     │TBL1│      │TBL2│        │  (obstacles - orange)
     │     └────┘      └────┘        │
     │                                │
     │           ┌────────┐          │
     │           │ PILLAR │          │  (another obstacle)
     │           └────────┘          │
     │                                │
     └────────────────────────────────┘
```

```

## Movement Behavior

### Clicking Inside Walkable Area

Player walks directly to the clicked point.

### Clicking Outside Walkable Area

Player walks to the **nearest valid point** on the walkable area boundary.

```

                    X ← Click here (outside)
                    │
     ┌──────────────┼─────────────┐
     │              ↓             │
     │              ● ← Actual    │
     │           destination      │
     └────────────────────────────┘

```

### Clicking Inside an Obstacle

The click is **inside a blocked area** (obstacle). The player walks to the nearest point on the obstacle's edge:

```

     ┌────────────────────────────┐
     │                            │
     │     ┌────────┐             │
     │     │   X    │ ← Click     │  (obstacle - orange)
     │     │   │    │             │
     │     └───┼────┘             │
     │         ↓                  │
     │         ● ← Destination    │
     │       (edge of obstacle)   │
     └────────────────────────────┘

```

## Walker System

The movement logic uses a **visibility graph + A\* pathfinding** approach (see [NAVIGATION.md](NAVIGATION.md) for full details).

```cpp
// In walker_system.h

struct WalkerComponent {
    Vec3 target_position;
    float speed;
    std::vector<Vec2> path_waypoints;  // Computed path
    size_t current_waypoint_index;
    bool has_valid_path;
    bool wants_to_move;
    // ...
};

// Set target - computes path using visibility graph
void walker_set_target(WalkerComponent& walker, Vec2 current_pos, Vec3 target, 
                       const std::vector<Collision::Polygon>& walkable_areas,
                       const std::vector<Collision::Polygon>& obstacles);

// Stop immediately
void walker_stop(WalkerComponent& walker, Vec2 current_pos);

// Update position each frame (follows computed path)
void walker_update(WalkerComponent& walker, ECS::Transform2_5DComponent& transform,
                   const std::vector<Collision::Polygon>& walkable_areas,
                   const std::vector<Collision::Polygon>& obstacles, float dt);
```

### Path Computation

When you set a target:

1. **Resolve Target**: If click is outside walkable area or inside obstacle, find nearest valid point
2. **Check Line-of-Sight**: If direct path is clear, use single waypoint
3. **Build Visibility Graph**: Otherwise, build graph from obstacle/area vertices
4. **A\* Pathfinding**: Find shortest path through the graph
5. **Store Waypoints**: Path is stored in WalkerComponent for following

### Following the Path

Each frame, `walker_update`:

1. Moves toward current waypoint at constant speed
2. When waypoint reached, advances to next
3. When all waypoints visited, stops

This ensures smooth, predictable navigation around obstacles without stuck states or oscillation.

## Collision Functions

### point_in_polygon

Check if a point is inside a single polygon.

```cpp
bool point_in_polygon(Vec2 point, const Polygon& polygon);
```

### point_in_walkable_area

Check if a point is inside ANY walkable area polygon.

```cpp
bool point_in_walkable_area(Vec2 point, const std::vector<Polygon>& walkable_areas);
```

### point_is_walkable

Check if a point is walkable (in walkable area AND not in obstacle).

```cpp
bool point_is_walkable(Vec2 point, 
                       const std::vector<Polygon>& walkable_areas,
                       const std::vector<Polygon>& obstacles);
```

### closest_point_on_polygon

Find the nearest point on a polygon's boundary.

```cpp
Vec2 closest_point_on_polygon(Vec2 point, const Polygon& polygon);
```

### find_nearest_walkable_point

Find the nearest walkable point when outside all areas or inside a hole.

```cpp
Vec2 find_nearest_walkable_point(Vec2 point, const std::vector<Polygon>& walkable_areas);
```

## JSON Format

```json
{
  "walkable_areas": [
    {
      "name": "room",
      "points": [[50, 100], [250, 100], [250, 180], [50, 180]]
    },
    {
      "name": "table_hole",
      "points": [[120, 130], [180, 130], [180, 160], [120, 160]]
    }
  ]
}
```

**Note:** The order of polygons doesn't matter. The odd/even rule automatically determines walkability based on containment.

## Debugging

With debug overlay active (**D** key):

- Walkable areas shown in green
- Holes (inner polygons) also shown in green
- Vertices shown as small rectangles
- Click + drag to move vertices
- Right-click vertex to delete
- Press **R** to reload geometry from JSON

## Tips

### Room Boundaries

Make walkable areas slightly smaller than visual room boundaries. This prevents the player from walking "into" walls.

```
     Wall texture edge
     │
     ↓
     █████████████████████████████
     █                           █
     █  ┌─────────────────────┐  █ ← Walkable area edge
     █  │                     │  █   (a few pixels inward)
     █  │                     │  █
```

### Obstacle Padding

For obstacles, make the hole slightly larger than the visual object:

```
     Visual table (sprite)
           ┌───────┐
           │███████│
     ┌─────┼───────┼─────┐ ← Hole polygon
     │     │███████│     │   (extends past visual)
     └─────┴───────┴─────┘
```

### Complex Obstacles

For L-shaped or irregular obstacles, use multiple rectangular holes:

```
     ┌────────────────────────────┐
     │                            │
     │  ┌───┐                     │
     │  │ H1├────┐                │
     │  └───┤ H2 │                │
     │      └────┘                │
     │                            │
     └────────────────────────────┘
```

## File Locations

| File | Purpose |
|------|---------|
| `assets/scenes/<scene>/geometry.json` | Walkable area polygons |
| `src/collision/polygon_utils.h` | Polygon math functions |
| `src/collision/polygon_utils.cpp` | Implementation |
| `src/navigation/navigation.h` | Pathfinding API |
| `src/navigation/navigation.cpp` | Visibility graph + A* |
| `src/game/walker_system.h` | Walker movement component |
| `src/game/walker_system.cpp` | Path-following logic |
| `src/debug/geometry_editor.cpp` | Visual editor |

## Related Documentation

- [NAVIGATION.md](NAVIGATION.md) - Full pathfinding system documentation
