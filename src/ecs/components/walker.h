#pragma once

// ============================================================================
// Walker Component for ECS
// Path-based movement with visibility graph navigation
// ============================================================================
//
// This component handles point-and-click movement using a simple approach:
// 1. When target is set, compute path using visibility graph + A*
// 2. Follow waypoints sequentially at constant speed
// 3. No complex edge-following or sliding logic
//
// The navigation logic is in src/navigation/navigation.h/cpp
// This component only stores state; walker_system.h handles updates.
// ============================================================================

#include "../../types.h"
#include <vector>

namespace ECS {

// ============================================================================
// WalkerComponent - Movement state for entities that walk/move
// ============================================================================

struct WalkerComponent {
    // Movement target (pixel coordinates + z-depth)
    // This is the final destination the player clicked on
    Vec3 target_position = Vec3(0.0f, 0.0f, 0.0f);
    
    // Movement settings
    float speed = 75.0f;                    // Pixels per second
    float distance_threshold = 3.0f;        // Distance to consider waypoint "reached"
    
    // Path state (managed by walker_system)
    std::vector<Vec2> path_waypoints;       // Computed path from navigation system
    size_t current_waypoint_index = 0;      // Index of next waypoint to reach
    bool has_valid_path = false;            // True if path was successfully computed
    
    // State
    bool wants_to_move = false;             // True if not yet at final target
    Vec2 movement_direction = Vec2(0.0f, 0.0f); // Direction toward current waypoint (for animation)
    
    WalkerComponent() = default;
    
    WalkerComponent(float move_speed, float arrive_threshold = 3.0f)
        : speed(move_speed), distance_threshold(arrive_threshold) {}
    
    // Clear current path (called when setting new target)
    void clear_path() {
        path_waypoints.clear();
        current_waypoint_index = 0;
        has_valid_path = false;
    }
    
    // Check if there are more waypoints to visit
    bool has_next_waypoint() const {
        return has_valid_path && current_waypoint_index < path_waypoints.size();
    }
    
    // Get current waypoint target (or invalid position if none)
    Vec2 current_waypoint() const {
        if (has_next_waypoint()) {
            return path_waypoints[current_waypoint_index];
        }
        return Vec2(-99999.0f, -99999.0f);
    }
    
    // Advance to next waypoint
    void advance_waypoint() {
        if (has_next_waypoint()) {
            current_waypoint_index++;
        }
    }
    
    // Get final destination
    Vec2 final_destination() const {
        if (!path_waypoints.empty()) {
            return path_waypoints.back();
        }
        return Vec2(target_position.x, target_position.y);
    }
    
    // Stop movement immediately
    void stop() {
        clear_path();
        wants_to_move = false;
        movement_direction = Vec2(0.0f, 0.0f);
    }
};

} // namespace ECS
