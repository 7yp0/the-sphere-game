#pragma once

// Walker System - Handles movement for entities with WalkerComponent
// Provides edge-following collision resolution for walking entities

#include "../ecs/components/walker.h"
#include "../ecs/components/transform.h"
#include "../collision/polygon_utils.h"
#include "../types.h"
#include <vector>

namespace Game {

// ============================================================================
// Walker System - Movement with edge-following collision
// ============================================================================

// Update a walker's position toward its target
// Returns true if still moving, false if arrived or stuck
// Updates transform.position and walker state
bool walker_update(ECS::WalkerComponent& walker, 
                   ECS::Transform2_5DComponent& transform,
                   const std::vector<Collision::Polygon>& walkable_areas,
                   float delta_time);

// Check if walker has reached its target
bool walker_at_target(const ECS::WalkerComponent& walker, Vec2 current_pos);

// Stop walker movement immediately
void walker_stop(ECS::WalkerComponent& walker, Vec2 current_pos);

// Set new target for walker
// If target is not in walkable area, automatically adjusts to nearest reachable point
void walker_set_target(ECS::WalkerComponent& walker, Vec3 target,
                       const std::vector<Collision::Polygon>& walkable_areas);

// Get the movement direction (for animation purposes)
// Returns normalized direction from current position toward target
Vec2 walker_get_direction(const ECS::WalkerComponent& walker, Vec2 current_pos);

} // namespace Game
