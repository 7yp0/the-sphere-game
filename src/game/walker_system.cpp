#include "walker_system.h"
#include <cmath>

namespace Game {

// ============================================================================
// Internal helper functions
// ============================================================================

// Check if a point is within tolerance of a line segment (edge)
static bool is_point_near_edge(Vec2 point, Vec2 edge_start, Vec2 edge_end, float tolerance = 2.0f) {
    Vec2 edge = Vec2(edge_end.x - edge_start.x, edge_end.y - edge_start.y);
    Vec2 to_point = Vec2(point.x - edge_start.x, point.y - edge_start.y);
    
    float edge_len_sq = edge.x * edge.x + edge.y * edge.y;
    if (edge_len_sq < 0.0001f) return false;
    
    // Project point onto edge line
    float t = (to_point.x * edge.x + to_point.y * edge.y) / edge_len_sq;
    
    // Check if projection is within edge bounds (with some slack)
    if (t < -0.1f || t > 1.1f) return false;
    
    // Calculate distance from point to edge line
    float edge_len = std::sqrt(edge_len_sq);
    Vec2 edge_norm = Vec2(edge.x / edge_len, edge.y / edge_len);
    Vec2 perp = Vec2(-edge_norm.y, edge_norm.x);
    float dist = std::abs(to_point.x * perp.x + to_point.y * perp.y);
    
    return dist <= tolerance;
}

// Check if direct path to target is clear (no collision)
static bool is_direct_path_clear(Vec2 from, Vec2 to, const std::vector<Collision::Polygon>& walkable_areas) {
    if (walkable_areas.empty()) return true;
    
    // Check if starting position is inside walkable area
    if (!Collision::point_in_walkable_area(from, walkable_areas)) {
        return false;
    }
    
    // Check several points along the path
    const int steps = 5;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / (float)steps;
        Vec2 test_pos = Vec2(from.x + t * (to.x - from.x), from.y + t * (to.y - from.y));
        if (!Collision::point_in_walkable_area(test_pos, walkable_areas)) {
            return false;
        }
    }
    return true;
}

// Check if walker has reached the end of the edge they're following
// Returns the endpoint if reached, or invalid Vec2 if not
static Vec2 get_reached_edge_endpoint(Vec2 position, const ECS::WalkerComponent& walker, float epsilon = 2.0f) {
    Vec2 to_start = Vec2(walker.edge_start.x - position.x, walker.edge_start.y - position.y);
    Vec2 to_end = Vec2(walker.edge_end.x - position.x, walker.edge_end.y - position.y);
    
    float dist_start_sq = to_start.x * to_start.x + to_start.y * to_start.y;
    float dist_end_sq = to_end.x * to_end.x + to_end.y * to_end.y;
    
    float eps_sq = epsilon * epsilon;
    
    // Determine which endpoint we're heading toward based on edge_direction
    Vec2 edge_vec = Vec2(walker.edge_end.x - walker.edge_start.x, 
                         walker.edge_end.y - walker.edge_start.y);
    float dot = walker.edge_direction.x * edge_vec.x + walker.edge_direction.y * edge_vec.y;
    
    if (dot >= 0.0f) {
        if (dist_end_sq <= eps_sq) return walker.edge_end;
    } else {
        if (dist_start_sq <= eps_sq) return walker.edge_start;
    }
    
    return Vec2(-99999.0f, -99999.0f); // Invalid marker
}

// Slide along collision boundary with edge-following
static Vec2 slide_along_boundary(Vec2 old_pos, Vec2 desired_pos, 
                                  const std::vector<Collision::Polygon>& walkable_areas,
                                  ECS::WalkerComponent& walker) {
    // If desired position is valid, just use it and stop edge following
    if (Collision::point_in_walkable_area(desired_pos, walkable_areas)) {
        walker.is_edge_following = false;
        return desired_pos;
    }
    
    // If we're already edge-following, try to continue along the current edge
    if (walker.is_edge_following) {
        Vec2 movement = Vec2(desired_pos.x - old_pos.x, desired_pos.y - old_pos.y);
        float move_dist = std::sqrt(movement.x * movement.x + movement.y * movement.y);
        float proj = movement.x * walker.edge_direction.x + movement.y * walker.edge_direction.y;
        
        // Only move if projection is positive (heading in the right direction along edge)
        if (proj > 0.01f && move_dist > 0.001f) {
            Vec2 slide_movement = Vec2(walker.edge_direction.x * move_dist, 
                                       walker.edge_direction.y * move_dist);
            Vec2 new_pos = Vec2(old_pos.x + slide_movement.x, old_pos.y + slide_movement.y);
            
            bool in_walkable = Collision::point_in_walkable_area(new_pos, walkable_areas);
            bool near_edge = is_point_near_edge(new_pos, walker.edge_start, walker.edge_end, 1.5f);
            
            if (in_walkable || near_edge) {
                return new_pos;
            }
            
            // Try smaller steps
            for (float scale = 0.75f; scale > 0.1f; scale -= 0.25f) {
                Vec2 test_pos = Vec2(old_pos.x + slide_movement.x * scale, 
                                     old_pos.y + slide_movement.y * scale);
                if (Collision::point_in_walkable_area(test_pos, walkable_areas) ||
                    is_point_near_edge(test_pos, walker.edge_start, walker.edge_end, 1.5f)) {
                    return test_pos;
                }
            }
        }
        
        // proj is too low - behavior depends on edge type
        // For hole edges: keep moving toward vertex to get around obstacle
        // For outer boundary: stay put, stuck timer will handle timeout
        
        if (!walker.is_on_hole_edge) {
            return old_pos;
        }
        
        // Hole edge - keep moving toward end of edge
        const float vertex_snap_dist = 3.0f;
        Vec2 to_start = Vec2(walker.edge_start.x - old_pos.x, walker.edge_start.y - old_pos.y);
        Vec2 to_end = Vec2(walker.edge_end.x - old_pos.x, walker.edge_end.y - old_pos.y);
        float dist_start_sq = to_start.x * to_start.x + to_start.y * to_start.y;
        float dist_end_sq = to_end.x * to_end.x + to_end.y * to_end.y;
        
        if (dist_start_sq < vertex_snap_dist * vertex_snap_dist) return walker.edge_start;
        if (dist_end_sq < vertex_snap_dist * vertex_snap_dist) return walker.edge_end;
        
        // Continue moving along edge
        float move_dist_local = std::sqrt((desired_pos.x - old_pos.x) * (desired_pos.x - old_pos.x) + 
                                          (desired_pos.y - old_pos.y) * (desired_pos.y - old_pos.y));
        if (move_dist_local > 0.001f) {
            Vec2 continue_movement = Vec2(walker.edge_direction.x * move_dist_local,
                                          walker.edge_direction.y * move_dist_local);
            Vec2 continue_pos = Vec2(old_pos.x + continue_movement.x, old_pos.y + continue_movement.y);
            
            if (Collision::point_in_walkable_area(continue_pos, walkable_areas) ||
                is_point_near_edge(continue_pos, walker.edge_start, walker.edge_end, 1.5f)) {
                return continue_pos;
            }
            
            for (float scale = 0.5f; scale > 0.1f; scale -= 0.2f) {
                Vec2 test_pos = Vec2(old_pos.x + continue_movement.x * scale,
                                     old_pos.y + continue_movement.y * scale);
                if (Collision::point_in_walkable_area(test_pos, walkable_areas) ||
                    is_point_near_edge(test_pos, walker.edge_start, walker.edge_end, 1.5f)) {
                    return test_pos;
                }
            }
        }
        
        return old_pos;
    }
    
    // Not edge-following - find closest edge and start following
    Collision::EdgeInfo edge = Collision::find_closest_edge(desired_pos, walkable_areas);
    if (!edge.valid) {
        return old_pos;
    }
    
    walker.is_edge_following = true;
    walker.is_on_hole_edge = edge.is_hole;
    walker.edge_start = edge.start;
    walker.edge_end = edge.end;
    
    // Choose edge direction based on movement direction
    Vec2 movement = Vec2(desired_pos.x - old_pos.x, desired_pos.y - old_pos.y);
    float move_dist = std::sqrt(movement.x * movement.x + movement.y * movement.y);
    
    Vec2 edge_vec = Vec2(edge.end.x - edge.start.x, edge.end.y - edge.start.y);
    float edge_len = std::sqrt(edge_vec.x * edge_vec.x + edge_vec.y * edge_vec.y);
    if (edge_len < 0.001f) return old_pos;
    
    Vec2 edge_dir = Vec2(edge_vec.x / edge_len, edge_vec.y / edge_len);
    float proj = movement.x * edge_dir.x + movement.y * edge_dir.y;
    
    if (proj >= 0.0f) {
        walker.edge_direction = edge_dir;
    } else {
        walker.edge_direction = Vec2(-edge_dir.x, -edge_dir.y);
        proj = -proj;
    }
    
    // Try to slide along this new edge
    if (proj > 0.0f && move_dist > 0.001f) {
        Vec2 slide_movement = Vec2(walker.edge_direction.x * move_dist, 
                                   walker.edge_direction.y * move_dist);
        Vec2 new_pos = Vec2(old_pos.x + slide_movement.x, old_pos.y + slide_movement.y);
        
        if (Collision::point_in_walkable_area(new_pos, walkable_areas) ||
            is_point_near_edge(new_pos, walker.edge_start, walker.edge_end, 1.5f)) {
            return new_pos;
        }
        
        for (float scale = 0.75f; scale > 0.1f; scale -= 0.25f) {
            Vec2 test_pos = Vec2(old_pos.x + slide_movement.x * scale, 
                                 old_pos.y + slide_movement.y * scale);
            if (Collision::point_in_walkable_area(test_pos, walkable_areas) ||
                is_point_near_edge(test_pos, walker.edge_start, walker.edge_end, 1.5f)) {
                return test_pos;
            }
        }
    }
    
    return old_pos;
}

// Handle vertex transition when reaching end of edge
static void handle_vertex_transition(ECS::WalkerComponent& walker, Vec2& new_pos, Vec2 reached_vertex,
                                     float move_dist, const std::vector<Collision::Polygon>& walkable_areas) {
    Vec2 target_2d = Vec2(walker.target_position.x, walker.target_position.y);
    
    // Check if direct path to target is now clear
    if (is_direct_path_clear(reached_vertex, target_2d, walkable_areas)) {
        walker.is_edge_following = false;
        new_pos = reached_vertex;
        return;
    }
    
    // Find next edge at this vertex
    Collision::EdgeInfo current_edge;
    current_edge.start = walker.edge_start;
    current_edge.end = walker.edge_end;
    current_edge.valid = true;
    
    Collision::EdgeInfo next_edge = Collision::find_next_edge_at_vertex(
        current_edge, reached_vertex, target_2d, walkable_areas);
    
    if (next_edge.valid) {
        new_pos = reached_vertex;
        walker.edge_start = next_edge.start;
        walker.edge_end = next_edge.end;
        walker.is_on_hole_edge = next_edge.is_hole;
        
        // Calculate direction toward target along new edge
        Vec2 to_target = Vec2(target_2d.x - reached_vertex.x, target_2d.y - reached_vertex.y);
        Vec2 edge_vec = Vec2(next_edge.end.x - next_edge.start.x, 
                             next_edge.end.y - next_edge.start.y);
        float edge_len = std::sqrt(edge_vec.x * edge_vec.x + edge_vec.y * edge_vec.y);
        
        if (edge_len > 0.001f) {
            Vec2 edge_dir = Vec2(edge_vec.x / edge_len, edge_vec.y / edge_len);
            float dot = edge_dir.x * to_target.x + edge_dir.y * to_target.y;
            walker.edge_direction = (dot >= 0) ? edge_dir : Vec2(-edge_dir.x, -edge_dir.y);
        }
        
        // Continue moving along new edge
        float remaining = move_dist * 0.5f;
        if (remaining > 0.1f) {
            Vec2 edge_move = Vec2(walker.edge_direction.x * remaining,
                                  walker.edge_direction.y * remaining);
            Vec2 continued_pos = Vec2(new_pos.x + edge_move.x, new_pos.y + edge_move.y);
            
            if (Collision::point_in_walkable_area(continued_pos, walkable_areas) ||
                is_point_near_edge(continued_pos, walker.edge_start, walker.edge_end, 2.0f)) {
                new_pos = continued_pos;
            }
        }
    } else {
        walker.is_edge_following = false;
    }
}

// ============================================================================
// Public API
// ============================================================================

bool walker_update(ECS::WalkerComponent& walker, 
                   ECS::Transform2_5DComponent& transform,
                   const std::vector<Collision::Polygon>& walkable_areas,
                   float delta_time) {
    Vec2 to_target = Vec2(
        walker.target_position.x - transform.position.x,
        walker.target_position.y - transform.position.y
    );
    
    float distance_sq = to_target.x * to_target.x + to_target.y * to_target.y;
    float threshold_sq = walker.distance_threshold * walker.distance_threshold;
    walker.wants_to_move = distance_sq > threshold_sq;
    
    // Reached target
    if (!walker.wants_to_move) {
        walker.is_edge_following = false;
        walker.movement_direction = Vec2(0.0f, 0.0f);
        walker.stuck_timer = 0.0f;
        walker.last_significant_position = transform.position;
        return false;
    }
    
    // Store direction for animation
    float dist = std::sqrt(distance_sq);
    walker.movement_direction = Vec2(to_target.x / dist, to_target.y / dist);
    
    // Check if we can stop edge-following (direct path is now clear)
    if (walker.is_edge_following) {
        if (is_direct_path_clear(transform.position, 
                                  Vec2(walker.target_position.x, walker.target_position.y),
                                  walkable_areas)) {
            walker.is_edge_following = false;
        }
    }
    
    Vec2 old_pos = transform.position;
    
    // Calculate desired position this frame
    Vec2 movement = Vec2(
        walker.movement_direction.x * walker.speed * delta_time,
        walker.movement_direction.y * walker.speed * delta_time
    );
    
    float move_dist_sq = movement.x * movement.x + movement.y * movement.y;
    Vec2 desired_pos;
    if (move_dist_sq >= distance_sq) {
        desired_pos = Vec2(walker.target_position.x, walker.target_position.y);
    } else {
        desired_pos = Vec2(old_pos.x + movement.x, old_pos.y + movement.y);
    }
    
    // Move with collision handling
    Vec2 new_pos = slide_along_boundary(old_pos, desired_pos, walkable_areas, walker);
    
    // Handle vertex transitions
    if (walker.is_edge_following) {
        Vec2 reached_vertex = get_reached_edge_endpoint(new_pos, walker);
        if (reached_vertex.x > -99990.0f) {
            handle_vertex_transition(walker, new_pos, reached_vertex, 
                                     std::sqrt(move_dist_sq), walkable_areas);
        }
    }
    
    // Apply position
    transform.position = new_pos;
    
    // Stuck detection
    Vec2 position_delta = Vec2(
        transform.position.x - walker.last_significant_position.x,
        transform.position.y - walker.last_significant_position.y
    );
    float movement_from_last_sq = position_delta.x * position_delta.x + position_delta.y * position_delta.y;
    float stuck_threshold_sq = walker.stuck_movement_threshold * walker.stuck_movement_threshold;
    
    if (movement_from_last_sq >= stuck_threshold_sq) {
        walker.stuck_timer = 0.0f;
        walker.last_significant_position = transform.position;
    } else {
        walker.stuck_timer += delta_time;
        
        if (walker.stuck_timer >= walker.stuck_timeout) {
            // Stuck too long - cancel movement
            walker.target_position = Vec3(transform.position.x, transform.position.y, 0.0f);
            walker.is_edge_following = false;
            walker.stuck_timer = 0.0f;
            walker.wants_to_move = false;
            return false;
        }
    }
    
    return true;
}

bool walker_at_target(const ECS::WalkerComponent& walker, Vec2 current_pos) {
    Vec2 to_target = Vec2(
        walker.target_position.x - current_pos.x,
        walker.target_position.y - current_pos.y
    );
    float distance_sq = to_target.x * to_target.x + to_target.y * to_target.y;
    float threshold_sq = walker.distance_threshold * walker.distance_threshold;
    return distance_sq <= threshold_sq;
}

void walker_stop(ECS::WalkerComponent& walker, Vec2 current_pos) {
    walker.target_position = Vec3(current_pos.x, current_pos.y, 0.0f);
    walker.is_edge_following = false;
    walker.wants_to_move = false;
    walker.stuck_timer = 0.0f;
}

void walker_set_target(ECS::WalkerComponent& walker, Vec3 target,
                       const std::vector<Collision::Polygon>& walkable_areas) {
    Vec2 target_2d(target.x, target.y);
    
    // If target is not in walkable area (e.g. inside a hole),
    // find the closest reachable point on polygon boundary
    if (!walkable_areas.empty() && !Collision::point_in_walkable_area(target_2d, walkable_areas)) {
        Vec2 closest = Collision::closest_point_on_any_polygon(target_2d, walkable_areas);
        target.x = closest.x;
        target.y = closest.y;
    }
    
    walker.target_position = target;
    walker.stuck_timer = 0.0f;
}

Vec2 walker_get_direction(const ECS::WalkerComponent& walker, [[maybe_unused]] Vec2 current_pos) {
    if (!walker.wants_to_move) {
        return Vec2(0.0f, 0.0f);
    }
    return walker.movement_direction;
}

} // namespace Game
