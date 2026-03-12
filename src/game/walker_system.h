#pragma once

// ============================================================================
// Walker System - Path-based movement for walking entities
// ============================================================================
//
// This system handles movement for entities with WalkerComponent.
// Unlike the old edge-following approach, this uses:
// 1. Target resolution (map clicks to valid walkable positions)
// 2. Visibility graph pathfinding (find path around obstacles)
// 3. Simple waypoint following (walk along computed path)
//
// The navigation logic (pathfinding, visibility graph) is in:
//   src/navigation/navigation.h/cpp
//
// Usage:
//   walker_set_target()  - Set new destination, computes path
//   walker_update()      - Call each frame to move along path
//   walker_at_target()   - Check if reached destination
//   walker_stop()        - Cancel current movement
// ============================================================================

#include "../ecs/components/walker.h"
#include "../ecs/components/transform.h"
#include "../collision/polygon_utils.h"
#include "../types.h"
#include <vector>

namespace Game {

// ============================================================================
// Public API
// ============================================================================

// Update a walker's position toward its target
// Returns true if still moving, false if arrived or no valid path
// Updates transform.position and walker state
bool walker_update(ECS::WalkerComponent& walker, 
                   ECS::Transform2_5DComponent& transform,
                   const std::vector<Collision::Polygon>& walkable_areas,
                   const std::vector<Collision::Polygon>& obstacles,
                   float delta_time);

// Check if walker has reached its final target
bool walker_at_target(const ECS::WalkerComponent& walker, Vec2 current_pos);

// Stop walker movement immediately
void walker_stop(ECS::WalkerComponent& walker, Vec2 current_pos);

// Set new target for walker
// Automatically computes path using visibility graph navigation
// If target is not walkable, finds closest reachable point
// current_pos: walker's current position (needed for path computation)
void walker_set_target(ECS::WalkerComponent& walker, Vec2 current_pos, Vec3 target,
                       const std::vector<Collision::Polygon>& walkable_areas,
                       const std::vector<Collision::Polygon>& obstacles);

// Get the movement direction (for animation purposes)
// Returns normalized direction toward current waypoint
Vec2 walker_get_direction(const ECS::WalkerComponent& walker, Vec2 current_pos);

// Recompute path from current position (useful after teleport or geometry change)
void walker_recompute_path(ECS::WalkerComponent& walker, Vec2 current_pos,
                           const std::vector<Collision::Polygon>& walkable_areas,
                           const std::vector<Collision::Polygon>& obstacles);

} // namespace Game
