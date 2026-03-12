#pragma once

// ============================================================================
// Navigation System
// Visibility-Graph-based pathfinding for 2D point-and-click movement
// ============================================================================
//
// The navigation system uses a simple but robust approach:
// 1. Resolve target: Map click point to nearest valid walkable position
// 2. Line-of-sight check: If direct path is clear, walk directly
// 3. Visibility Graph: Build graph from start, target, and obstacle vertices
// 4. A* pathfinding: Find shortest path through the graph
// 5. Path following: Walk along waypoints at constant speed
//
// Key design decisions:
// - All polygons MUST be convex (simplifies intersection tests)
// - Player is treated as a point (or very small radius)
// - No sliding/edge-following as primary navigation strategy
// - Deterministic, stable behavior without oscillation or stuck states
//
// ============================================================================

#include "../types.h"
#include "../collision/polygon_utils.h"
#include <vector>
#include <optional>

namespace Navigation {

// ============================================================================
// Configuration Constants
// ============================================================================

constexpr float EPSILON = 0.001f;           // Floating point tolerance
constexpr float POINT_EPSILON = 1.0f;       // Distance to consider points equal
constexpr float WAYPOINT_REACHED = 3.0f;    // Distance to consider waypoint reached
constexpr float OBSTACLE_OFFSET = 8.0f;     // Offset from obstacle vertices (must be > player radius)
constexpr int MAX_PATH_NODES = 256;         // Maximum nodes in pathfinding

// ============================================================================
// Path Data Structure
// ============================================================================

struct Path {
    std::vector<Vec2> waypoints;    // Ordered waypoints from start to target
    size_t current_index = 0;       // Index of next waypoint to reach
    bool valid = false;             // True if path was successfully computed
    
    // Reset path state
    void clear() {
        waypoints.clear();
        current_index = 0;
        valid = false;
    }
    
    // Check if we have more waypoints to visit
    bool has_next() const {
        return valid && current_index < waypoints.size();
    }
    
    // Get current target waypoint (or invalid if none)
    Vec2 current_waypoint() const {
        if (has_next()) {
            return waypoints[current_index];
        }
        return Vec2(-99999.0f, -99999.0f);
    }
    
    // Advance to next waypoint
    void advance() {
        if (has_next()) {
            current_index++;
        }
    }
    
    // Get final destination
    Vec2 final_destination() const {
        if (valid && !waypoints.empty()) {
            return waypoints.back();
        }
        return Vec2(-99999.0f, -99999.0f);
    }
};

// ============================================================================
// Geometry Helper Functions
// ============================================================================

// Check if a point lies inside a convex polygon
// Uses cross-product sign consistency check (efficient for convex polygons)
// Returns true if point is inside or on the boundary
bool point_in_convex_polygon(Vec2 point, const Collision::Polygon& polygon);

// Check if two line segments intersect (proper intersection, not just touch)
// Returns true if segments cross each other
// Uses robust epsilon-based comparison to handle near-touches
bool segment_intersects_segment(Vec2 a1, Vec2 a2, Vec2 b1, Vec2 b2);

// Check if a line segment intersects a polygon boundary
// Returns true if the segment crosses any edge of the polygon
// Note: endpoints touching edges counts as intersection
bool segment_intersects_polygon(Vec2 seg_start, Vec2 seg_end, 
                                 const Collision::Polygon& polygon);

// Check if a point is in a walkable position
// Must be inside at least one walkable area AND not inside any obstacle
bool is_point_walkable(Vec2 point,
                       const std::vector<Collision::Polygon>& walkable_areas,
                       const std::vector<Collision::Polygon>& obstacles);

// Check if an entire segment is walkable (can be traversed)
// The segment must:
// - Have both endpoints walkable
// - Not cross any walkable area boundary (stay inside)
// - Not cross any obstacle boundary (go around)
// Uses midpoint and multi-sample checking for robustness
bool is_segment_walkable(Vec2 start, Vec2 end,
                         const std::vector<Collision::Polygon>& walkable_areas,
                         const std::vector<Collision::Polygon>& obstacles);

// Project a point onto a line segment, returning the closest point on the segment
Vec2 project_point_to_segment(Vec2 point, Vec2 seg_start, Vec2 seg_end);

// Find the closest point on a polygon boundary to a given point
Vec2 closest_point_on_polygon_boundary(Vec2 point, const Collision::Polygon& polygon);

// ============================================================================
// Target Resolution
// ============================================================================

// Resolve a click point to a valid walkable target
// If click is already walkable: returns click point
// If click is outside walkable area: returns closest point on walkable boundary
// If click is inside obstacle: returns closest walkable point outside obstacle
// This ensures we always have a reachable destination
Vec2 resolve_target(Vec2 click_point,
                    const std::vector<Collision::Polygon>& walkable_areas,
                    const std::vector<Collision::Polygon>& obstacles);

// ============================================================================
// Visibility Graph & Pathfinding
// ============================================================================

// Build a visibility graph and find shortest path from start to target
// Uses obstacle vertices as intermediate waypoints
// Returns a path with waypoints to follow (may be empty if no path exists)
//
// Algorithm:
// 1. Collect nodes: start, target, and all obstacle vertices (with small offset)
// 2. Build edges: connect nodes that have clear line-of-sight
// 3. Run A* from start to target using euclidean distance heuristic
// 4. Return path as ordered list of waypoints
Path find_path(Vec2 start, Vec2 target,
               const std::vector<Collision::Polygon>& walkable_areas,
               const std::vector<Collision::Polygon>& obstacles);

// ============================================================================
// Convenience Functions
// ============================================================================

// Check if there's a direct line-of-sight from start to end
// Convenience wrapper around is_segment_walkable
bool has_line_of_sight(Vec2 start, Vec2 end,
                       const std::vector<Collision::Polygon>& walkable_areas,
                       const std::vector<Collision::Polygon>& obstacles);

// Calculate euclidean distance between two points
float distance(Vec2 a, Vec2 b);

// Calculate squared euclidean distance (avoids sqrt, useful for comparisons)
float distance_squared(Vec2 a, Vec2 b);

} // namespace Navigation
