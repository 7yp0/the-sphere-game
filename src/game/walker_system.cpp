// ============================================================================
// Walker System - Path-based movement implementation
// ============================================================================
//
// This is a complete rewrite of the walker system using visibility graph
// pathfinding instead of the previous edge-following approach.
//
// Key differences from old system:
// - No edge-following or sliding logic
// - Path is computed once when target is set (using A*)
// - Movement simply follows waypoints sequentially
// - Much simpler, more predictable behavior
//
// The complex pathfinding logic is encapsulated in navigation.h/cpp
// ============================================================================

#include "walker_system.h"
#include "../navigation/navigation.h"
#include "../debug/debug_log.h"
#include <cmath>

namespace Game {

// ============================================================================
// Internal Helper Functions
// ============================================================================

// Calculate distance between two points
static float calc_distance(Vec2 a, Vec2 b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

// Calculate squared distance (avoids sqrt)
static float calc_distance_squared(Vec2 a, Vec2 b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return dx * dx + dy * dy;
}

// Normalize a vector, returns zero vector if input is too small
static Vec2 normalize(Vec2 v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len < 0.001f) {
        return Vec2(0.0f, 0.0f);
    }
    return Vec2(v.x / len, v.y / len);
}

// ============================================================================
// Public API Implementation
// ============================================================================

bool walker_update(ECS::WalkerComponent& walker, 
                   ECS::Transform2_5DComponent& transform,
                   const std::vector<Collision::Polygon>& walkable_areas,
                   const std::vector<Collision::Polygon>& obstacles,
                   float delta_time) {
    
    // No movement if no valid path or not wanting to move
    if (!walker.wants_to_move || !walker.has_valid_path) {
        walker.movement_direction = Vec2(0.0f, 0.0f);
        return false;
    }
    
    // Check if we have a waypoint to move toward
    if (!walker.has_next_waypoint()) {
        // No more waypoints - we've arrived
        walker.wants_to_move = false;
        walker.movement_direction = Vec2(0.0f, 0.0f);
        return false;
    }
    
    Vec2 current_pos = transform.position;
    Vec2 target_waypoint = walker.current_waypoint();
    
    // Calculate direction and distance to current waypoint
    Vec2 to_waypoint(target_waypoint.x - current_pos.x, 
                     target_waypoint.y - current_pos.y);
    float distance_to_waypoint = std::sqrt(to_waypoint.x * to_waypoint.x + 
                                           to_waypoint.y * to_waypoint.y);
    
    // Check if we've reached the current waypoint
    if (distance_to_waypoint <= walker.distance_threshold) {
        walker.advance_waypoint();
        
        // Check if that was the last waypoint
        if (!walker.has_next_waypoint()) {
            // Arrived at final destination
            walker.wants_to_move = false;
            walker.movement_direction = Vec2(0.0f, 0.0f);
            return false;
        }
        
        // Update target to next waypoint
        target_waypoint = walker.current_waypoint();
        to_waypoint = Vec2(target_waypoint.x - current_pos.x,
                           target_waypoint.y - current_pos.y);
        distance_to_waypoint = std::sqrt(to_waypoint.x * to_waypoint.x +
                                         to_waypoint.y * to_waypoint.y);
    }
    
    // Normalize direction
    Vec2 direction = normalize(to_waypoint);
    walker.movement_direction = direction;
    
    // Calculate movement for this frame
    float move_distance = walker.speed * delta_time;
    
    // Don't overshoot the waypoint
    if (move_distance > distance_to_waypoint) {
        move_distance = distance_to_waypoint;
    }
    
    // Calculate new position
    Vec2 new_pos(current_pos.x + direction.x * move_distance,
                 current_pos.y + direction.y * move_distance);
    
    // Validate new position is walkable
    // This is a safety check - the path should already be valid
    if (!walkable_areas.empty()) {
        if (!Navigation::is_point_walkable(new_pos, walkable_areas, obstacles)) {
            // Something went wrong - path led to unwalkable position
            // Try to recover by stopping at current position
            walker.wants_to_move = false;
            walker.clear_path();
            return false;
        }
    }
    
    // Apply movement
    transform.position = new_pos;
    
    return true; // Still moving
}

bool walker_at_target(const ECS::WalkerComponent& walker, Vec2 current_pos) {
    if (!walker.has_valid_path || walker.path_waypoints.empty()) {
        return true; // No path = consider at target
    }
    
    Vec2 final_dest = walker.final_destination();
    float dist_sq = calc_distance_squared(current_pos, final_dest);
    float threshold_sq = walker.distance_threshold * walker.distance_threshold;
    
    return dist_sq <= threshold_sq;
}

void walker_stop(ECS::WalkerComponent& walker, [[maybe_unused]] Vec2 current_pos) {
    walker.stop();
}

void walker_set_target(ECS::WalkerComponent& walker, Vec2 current_pos, Vec3 target,
                       const std::vector<Collision::Polygon>& walkable_areas,
                       const std::vector<Collision::Polygon>& obstacles) {
    // Clear any existing path
    walker.clear_path();
    
    // Store the raw target (for reference)
    walker.target_position = target;
    
    Vec2 target_2d(target.x, target.y);
    
    // Check if we're already at target
    float dist_to_target = calc_distance(current_pos, target_2d);
    if (dist_to_target <= walker.distance_threshold) {
        walker.wants_to_move = false;
        return;
    }
    
    // If no walkable areas defined, allow free movement (direct path)
    if (walkable_areas.empty()) {
        walker.path_waypoints.push_back(target_2d);
        walker.has_valid_path = true;
        walker.wants_to_move = true;
        walker.current_waypoint_index = 0;
        return;
    }
    
    // Use navigation system to find path
    Navigation::Path nav_path = Navigation::find_path(current_pos, target_2d, 
                                                       walkable_areas, obstacles);
    
    if (nav_path.valid && !nav_path.waypoints.empty()) {
        walker.path_waypoints = std::move(nav_path.waypoints);
        walker.current_waypoint_index = 0;
        walker.has_valid_path = true;
        walker.wants_to_move = true;
    } else {
        // No path found - stay in place
        walker.has_valid_path = false;
        walker.wants_to_move = false;
    }
}

Vec2 walker_get_direction(const ECS::WalkerComponent& walker, [[maybe_unused]] Vec2 current_pos) {
    if (!walker.wants_to_move) {
        return Vec2(0.0f, 0.0f);
    }
    return walker.movement_direction;
}

void walker_recompute_path(ECS::WalkerComponent& walker, Vec2 current_pos,
                           const std::vector<Collision::Polygon>& walkable_areas,
                           const std::vector<Collision::Polygon>& obstacles) {
    // Clear existing path
    walker.clear_path();
    
    Vec2 target_2d(walker.target_position.x, walker.target_position.y);
    
    // Use navigation system to find path
    Navigation::Path nav_path = Navigation::find_path(current_pos, target_2d, 
                                                       walkable_areas, obstacles);
    
    if (nav_path.valid && !nav_path.waypoints.empty()) {
        walker.path_waypoints = std::move(nav_path.waypoints);
        walker.current_waypoint_index = 0;
        walker.has_valid_path = true;
        walker.wants_to_move = true;
    } else {
        // No path found
        walker.has_valid_path = false;
        walker.wants_to_move = false;
    }
}

} // namespace Game
