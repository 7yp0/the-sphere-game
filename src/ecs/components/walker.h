#pragma once

// Walker Component for ECS
// Handles point-and-click movement with edge-following collision

#include "../../types.h"

namespace ECS {

// ============================================================================
// WalkerComponent - Movement state for entities that walk/move
// ============================================================================
// Used by player, NPCs, or any entity that needs to move toward a target
// with collision handling and edge-following behavior.
// ============================================================================

struct WalkerComponent {
    // Movement target (pixel coordinates + z-depth)
    Vec3 target_position = Vec3(0.0f, 0.0f, 0.0f);
    
    // Movement settings
    float speed = 75.0f;                    // Pixels per second
    float distance_threshold = 0.25f;       // Distance to consider "arrived"
    
    // Edge-following collision state
    bool is_edge_following = false;         // Currently sliding along an edge
    bool is_on_hole_edge = false;           // True if edge belongs to inner polygon (obstacle)
    Vec2 edge_start = Vec2(0.0f, 0.0f);     // Start point of current edge
    Vec2 edge_end = Vec2(0.0f, 0.0f);       // End point of current edge
    Vec2 edge_direction = Vec2(0.0f, 0.0f); // Normalized direction along edge
    
    // Stuck detection
    float stuck_timer = 0.0f;               // Time without meaningful movement
    Vec2 last_significant_position = Vec2(0.0f, 0.0f);
    float stuck_movement_threshold = 1.0f;  // Pixels to move to reset timer
    float stuck_timeout = 0.5f;             // Seconds to cancel movement
    
    // State
    bool wants_to_move = false;             // True if not yet at target
    Vec2 movement_direction = Vec2(0.0f, 0.0f); // Direction toward target (for animation)
    
    WalkerComponent() = default;
    
    WalkerComponent(float move_speed, float arrive_threshold = 0.25f)
        : speed(move_speed), distance_threshold(arrive_threshold) {}
    
    // Set a new target position
    void set_target(Vec3 target) {
        target_position = target;
        // Reset stuck detection when new target is set
        stuck_timer = 0.0f;
    }
    
    // Stop movement immediately
    void stop() {
        is_edge_following = false;
        wants_to_move = false;
    }
};

} // namespace ECS
