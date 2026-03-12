#include "navigation.h"
#include "../debug/debug_log.h"
#include <cmath>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <limits>

namespace Navigation {

// ============================================================================
// Helper: Distance Functions
// ============================================================================

float distance(Vec2 a, Vec2 b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

float distance_squared(Vec2 a, Vec2 b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return dx * dx + dy * dy;
}

// ============================================================================
// Helper: Cross Product
// ============================================================================

// Returns the z-component of the cross product of vectors (o->a) x (o->b)
// Positive: b is counter-clockwise from a (relative to o)
// Negative: b is clockwise from a (relative to o)
// Zero: a, o, b are collinear
static float cross_product_2d(Vec2 o, Vec2 a, Vec2 b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

// Check if value is effectively zero within epsilon
static bool is_zero(float val, float eps = EPSILON) {
    return std::abs(val) < eps;
}

// ============================================================================
// Geometry: Point in Convex Polygon
// ============================================================================

bool point_in_convex_polygon(Vec2 point, const Collision::Polygon& polygon) {
    if (!polygon.is_valid() || polygon.points.size() < 3) {
        return false;
    }
    
    size_t n = polygon.points.size();
    
    // For convex polygons: point is inside if it's on the same side of all edges
    // We check sign of cross product for each edge
    bool sign_positive = false;
    bool sign_set = false;
    
    for (size_t i = 0; i < n; i++) {
        Vec2 p1 = polygon.points[i];
        Vec2 p2 = polygon.points[(i + 1) % n];
        
        float cross = cross_product_2d(p1, p2, point);
        
        // Allow points ON the edge (cross ~= 0)
        if (is_zero(cross, POINT_EPSILON)) {
            // Check if point is within edge bounds
            float t = 0.0f;
            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            if (std::abs(dx) > std::abs(dy)) {
                t = (point.x - p1.x) / dx;
            } else if (std::abs(dy) > EPSILON) {
                t = (point.y - p1.y) / dy;
            }
            if (t >= -EPSILON && t <= 1.0f + EPSILON) {
                continue; // On edge, continue checking
            }
        }
        
        if (!sign_set) {
            sign_positive = (cross > 0);
            sign_set = true;
        } else if ((cross > 0) != sign_positive) {
            return false; // Different sign = outside
        }
    }
    
    return true;
}

// ============================================================================
// Geometry: Segment Intersection
// ============================================================================

bool segment_intersects_segment(Vec2 a1, Vec2 a2, Vec2 b1, Vec2 b2) {
    // Use cross product orientation test
    float d1 = cross_product_2d(b1, b2, a1);
    float d2 = cross_product_2d(b1, b2, a2);
    float d3 = cross_product_2d(a1, a2, b1);
    float d4 = cross_product_2d(a1, a2, b2);
    
    // Check for proper intersection (segments cross each other)
    // Use smaller epsilon for more robust detection
    const float SEG_EPS = 0.01f;
    if (((d1 > SEG_EPS && d2 < -SEG_EPS) || (d1 < -SEG_EPS && d2 > SEG_EPS)) &&
        ((d3 > SEG_EPS && d4 < -SEG_EPS) || (d3 < -SEG_EPS && d4 > SEG_EPS))) {
        return true;
    }
    
    // Also check if segments nearly touch (one endpoint very close to other segment)
    // This prevents paths that graze obstacle edges
    const float GRAZE_DIST = 1.0f;
    
    // Check if any endpoint of segment A is very close to segment B
    Vec2 projA1 = project_point_to_segment(a1, b1, b2);
    Vec2 projA2 = project_point_to_segment(a2, b1, b2);
    Vec2 projB1 = project_point_to_segment(b1, a1, a2);
    Vec2 projB2 = project_point_to_segment(b2, a1, a2);
    
    // If any projection is very close AND the segment actually crosses
    float distA1 = distance(a1, projA1);
    float distA2 = distance(a2, projA2);
    float distB1 = distance(b1, projB1);
    float distB2 = distance(b2, projB2);
    
    // Check if d values indicate segments are on opposite sides (crossing)
    bool a_crosses_b = (d1 * d2 < 0.0f); // a1 and a2 on different sides of line b
    bool b_crosses_a = (d3 * d4 < 0.0f); // b1 and b2 on different sides of line a
    
    // If both crossing conditions met and one endpoint is very close, it's an intersection
    if (a_crosses_b && b_crosses_a) {
        if (distA1 < GRAZE_DIST || distA2 < GRAZE_DIST || distB1 < GRAZE_DIST || distB2 < GRAZE_DIST) {
            return true;
        }
    }
    
    return false;
}

bool segment_intersects_polygon(Vec2 seg_start, Vec2 seg_end, 
                                 const Collision::Polygon& polygon) {
    if (!polygon.is_valid()) {
        return false;
    }
    
    size_t n = polygon.points.size();
    for (size_t i = 0; i < n; i++) {
        Vec2 edge_start = polygon.points[i];
        Vec2 edge_end = polygon.points[(i + 1) % n];
        
        if (segment_intersects_segment(seg_start, seg_end, edge_start, edge_end)) {
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// Geometry: Point Walkability
// ============================================================================

bool is_point_walkable(Vec2 point,
                       const std::vector<Collision::Polygon>& walkable_areas,
                       const std::vector<Collision::Polygon>& obstacles) {
    // Must be inside at least one walkable area
    bool in_walkable = false;
    for (const auto& area : walkable_areas) {
        if (Collision::point_in_polygon(point, area)) {
            in_walkable = true;
            break;
        }
    }
    
    if (!in_walkable) {
        return false;
    }
    
    // Must NOT be inside any obstacle
    for (const auto& obs : obstacles) {
        if (Collision::point_in_polygon(point, obs)) {
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// Geometry: Segment Walkability
// ============================================================================

bool is_segment_walkable(Vec2 start, Vec2 end,
                         const std::vector<Collision::Polygon>& walkable_areas,
                         const std::vector<Collision::Polygon>& obstacles) {
    // Empty walkable area = everything walkable (for testing)
    if (walkable_areas.empty()) {
        return true;
    }
    
    // Both endpoints must be walkable
    if (!is_point_walkable(start, walkable_areas, obstacles)) {
        return false;
    }
    if (!is_point_walkable(end, walkable_areas, obstacles)) {
        return false;
    }
    
    // Segment must not cross any obstacle
    for (const auto& obs : obstacles) {
        if (segment_intersects_polygon(start, end, obs)) {
            return false;
        }
    }
    
    // Check multiple points along segment to ensure we stay in walkable area
    // This catches cases where segment exits and re-enters walkable area
    // Use more samples for longer segments
    float seg_len = distance(start, end);
    int num_samples = std::max(5, static_cast<int>(seg_len / 10.0f)); // 1 sample per 10 pixels
    num_samples = std::min(num_samples, 20); // Cap at 20 samples
    
    for (int i = 1; i < num_samples; i++) {
        float t = static_cast<float>(i) / static_cast<float>(num_samples);
        Vec2 sample(start.x + t * (end.x - start.x),
                    start.y + t * (end.y - start.y));
        
        if (!is_point_walkable(sample, walkable_areas, obstacles)) {
            return false;
        }
    }
    
    return true;
}

bool has_line_of_sight(Vec2 start, Vec2 end,
                       const std::vector<Collision::Polygon>& walkable_areas,
                       const std::vector<Collision::Polygon>& obstacles) {
    return is_segment_walkable(start, end, walkable_areas, obstacles);
}

// ============================================================================
// Geometry: Point Projection
// ============================================================================

Vec2 project_point_to_segment(Vec2 point, Vec2 seg_start, Vec2 seg_end) {
    Vec2 seg_vec(seg_end.x - seg_start.x, seg_end.y - seg_start.y);
    Vec2 point_vec(point.x - seg_start.x, point.y - seg_start.y);
    
    float seg_len_sq = seg_vec.x * seg_vec.x + seg_vec.y * seg_vec.y;
    
    if (seg_len_sq < EPSILON) {
        return seg_start; // Degenerate segment
    }
    
    // Project point onto line, clamp t to [0, 1] for segment
    float t = (point_vec.x * seg_vec.x + point_vec.y * seg_vec.y) / seg_len_sq;
    t = std::max(0.0f, std::min(1.0f, t));
    
    return Vec2(seg_start.x + t * seg_vec.x, seg_start.y + t * seg_vec.y);
}

Vec2 closest_point_on_polygon_boundary(Vec2 point, const Collision::Polygon& polygon) {
    if (!polygon.is_valid()) {
        return point;
    }
    
    Vec2 closest = polygon.points[0];
    float min_dist_sq = std::numeric_limits<float>::max();
    
    size_t n = polygon.points.size();
    for (size_t i = 0; i < n; i++) {
        Vec2 edge_start = polygon.points[i];
        Vec2 edge_end = polygon.points[(i + 1) % n];
        
        Vec2 projected = project_point_to_segment(point, edge_start, edge_end);
        float dist_sq = distance_squared(point, projected);
        
        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            closest = projected;
        }
    }
    
    return closest;
}

// ============================================================================
// Target Resolution
// ============================================================================

Vec2 resolve_target(Vec2 click_point,
                    const std::vector<Collision::Polygon>& walkable_areas,
                    const std::vector<Collision::Polygon>& obstacles) {
    // If click is already walkable, use it directly
    if (is_point_walkable(click_point, walkable_areas, obstacles)) {
        return click_point;
    }
    
    // Find closest walkable point
    Vec2 best_point = click_point;
    float best_dist_sq = std::numeric_limits<float>::max();
    
    // Strategy 1: Check closest point on walkable area boundaries
    for (const auto& area : walkable_areas) {
        Vec2 closest = closest_point_on_polygon_boundary(click_point, area);
        
        // Move slightly inside the polygon
        Vec2 center(0.0f, 0.0f);
        for (const auto& p : area.points) {
            center.x += p.x;
            center.y += p.y;
        }
        center.x /= area.points.size();
        center.y /= area.points.size();
        
        Vec2 to_center(center.x - closest.x, center.y - closest.y);
        float len = distance(closest, center);
        if (len > EPSILON) {
            to_center.x /= len;
            to_center.y /= len;
        }
        
        // Try multiple offset sizes
        const float offsets[] = {OBSTACLE_OFFSET, OBSTACLE_OFFSET * 0.5f, 2.0f, 1.0f, 0.0f};
        for (float offset : offsets) {
            Vec2 test_point(closest.x + to_center.x * offset,
                           closest.y + to_center.y * offset);
            
            if (is_point_walkable(test_point, walkable_areas, obstacles)) {
                float dist_sq = distance_squared(click_point, test_point);
                if (dist_sq < best_dist_sq) {
                    best_dist_sq = dist_sq;
                    best_point = test_point;
                }
                break; // Found a valid point with this closest boundary point
            }
        }
    }
    
    // Strategy 2: Check closest point on obstacle boundaries (for clicks inside obstacles)
    for (const auto& obs : obstacles) {
        if (Collision::point_in_polygon(click_point, obs)) {
            Vec2 closest = closest_point_on_polygon_boundary(click_point, obs);
            
            // Move slightly away from obstacle
            Vec2 center(0.0f, 0.0f);
            for (const auto& p : obs.points) {
                center.x += p.x;
                center.y += p.y;
            }
            center.x /= obs.points.size();
            center.y /= obs.points.size();
            
            Vec2 away(closest.x - center.x, closest.y - center.y);
            float len = distance(closest, center);
            if (len > EPSILON) {
                away.x /= len;
                away.y /= len;
            }
            
            // Try multiple offset sizes
            const float offsets[] = {OBSTACLE_OFFSET, OBSTACLE_OFFSET * 0.5f, 2.0f, 1.0f};
            for (float offset : offsets) {
                Vec2 test_point(closest.x + away.x * offset,
                               closest.y + away.y * offset);
                
                if (is_point_walkable(test_point, walkable_areas, obstacles)) {
                    float dist_sq = distance_squared(click_point, test_point);
                    if (dist_sq < best_dist_sq) {
                        best_dist_sq = dist_sq;
                        best_point = test_point;
                    }
                    break;
                }
            }
        }
    }
    
    // Strategy 3: If still no luck, sample along ALL walkable area edges
    if (best_dist_sq == std::numeric_limits<float>::max()) {
        for (const auto& area : walkable_areas) {
            size_t n = area.points.size();
            for (size_t i = 0; i < n; i++) {
                const Vec2& p1 = area.points[i];
                const Vec2& p2 = area.points[(i + 1) % n];
                
                // Sample points along this edge
                for (int t = 0; t <= 10; t++) {
                    float frac = t / 10.0f;
                    Vec2 edge_point(p1.x + frac * (p2.x - p1.x),
                                   p1.y + frac * (p2.y - p1.y));
                    
                    // Calculate inward normal
                    Vec2 edge_dir(p2.x - p1.x, p2.y - p1.y);
                    float edge_len = std::sqrt(edge_dir.x * edge_dir.x + edge_dir.y * edge_dir.y);
                    if (edge_len < EPSILON) continue;
                    
                    // Perpendicular (try both directions)
                    Vec2 perp1(-edge_dir.y / edge_len, edge_dir.x / edge_len);
                    Vec2 perp2(edge_dir.y / edge_len, -edge_dir.x / edge_len);
                    
                    // Calculate area centroid to determine which perpendicular is "inward"
                    Vec2 centroid(0.0f, 0.0f);
                    for (const auto& p : area.points) {
                        centroid.x += p.x;
                        centroid.y += p.y;
                    }
                    centroid.x /= n;
                    centroid.y /= n;
                    
                    Vec2 to_center(centroid.x - edge_point.x, centroid.y - edge_point.y);
                    float dot1 = perp1.x * to_center.x + perp1.y * to_center.y;
                    Vec2& inward = (dot1 > 0) ? perp1 : perp2;
                    
                    // Try offsets
                    const float offsets[] = {4.0f, 2.0f, 1.0f};
                    for (float offset : offsets) {
                        Vec2 test_point(edge_point.x + inward.x * offset,
                                       edge_point.y + inward.y * offset);
                        
                        if (is_point_walkable(test_point, walkable_areas, obstacles)) {
                            float dist_sq = distance_squared(click_point, test_point);
                            if (dist_sq < best_dist_sq) {
                                best_dist_sq = dist_sq;
                                best_point = test_point;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return best_point;
}

// ============================================================================
// A* Pathfinding Implementation
// ============================================================================

// Node for A* priority queue
struct AStarNode {
    size_t index;       // Index in nodes vector
    float f_score;      // g + h
    
    bool operator>(const AStarNode& other) const {
        return f_score > other.f_score;
    }
};

// Reconstruct path from came_from map
static Path reconstruct_path(const std::vector<Vec2>& nodes,
                             const std::unordered_map<size_t, size_t>& came_from,
                             size_t start_idx, size_t goal_idx) {
    Path path;
    path.valid = true;
    
    // Build path backwards
    std::vector<Vec2> reversed;
    size_t current = goal_idx;
    
    while (current != start_idx) {
        reversed.push_back(nodes[current]);
        auto it = came_from.find(current);
        if (it == came_from.end()) {
            // Broken path
            path.valid = false;
            return path;
        }
        current = it->second;
    }
    
    // We don't include start in waypoints (player is already there)
    // Reverse to get correct order
    path.waypoints.reserve(reversed.size());
    for (auto it = reversed.rbegin(); it != reversed.rend(); ++it) {
        path.waypoints.push_back(*it);
    }
    
    return path;
}

// ============================================================================
// Visibility Graph & Path Finding
// ============================================================================

Path find_path(Vec2 start, Vec2 target,
               const std::vector<Collision::Polygon>& walkable_areas,
               const std::vector<Collision::Polygon>& obstacles) {
    Path result;
    result.clear();
    
    // Edge case: start or target not walkable
    if (!is_point_walkable(start, walkable_areas, obstacles)) {
        return result;
    }
    
    // Resolve target to walkable position
    Vec2 resolved_target = resolve_target(target, walkable_areas, obstacles);
    
    // If target resolution failed
    if (!is_point_walkable(resolved_target, walkable_areas, obstacles)) {
        return result;
    }
    
    // Check for direct line of sight
    if (has_line_of_sight(start, resolved_target, walkable_areas, obstacles)) {
        result.waypoints.push_back(resolved_target);
        result.valid = true;
        return result;
    }
    
    // Build visibility graph nodes
    // Nodes: start (0), target (1), then obstacle vertices
    std::vector<Vec2> nodes;
    nodes.push_back(start);
    nodes.push_back(resolved_target);
    
    // Add obstacle vertices with small offset (to navigate around corners)
    for (const auto& obs : obstacles) {
        if (!obs.is_valid()) continue;
        
        // Calculate obstacle centroid
        Vec2 centroid(0.0f, 0.0f);
        for (const auto& p : obs.points) {
            centroid.x += p.x;
            centroid.y += p.y;
        }
        centroid.x /= obs.points.size();
        centroid.y /= obs.points.size();
        
        size_t n = obs.points.size();
        for (size_t vi = 0; vi < n; vi++) {
            const Vec2& vertex = obs.points[vi];
            
            // Calculate outward direction using bisector of adjacent edges
            // This gives better results for non-convex corners
            Vec2 prev = obs.points[(vi + n - 1) % n];
            Vec2 next = obs.points[(vi + 1) % n];
            
            // Edge directions (into vertex)
            Vec2 edge1(vertex.x - prev.x, vertex.y - prev.y);
            Vec2 edge2(next.x - vertex.x, next.y - vertex.y);
            
            // Normalize
            float len1 = std::sqrt(edge1.x * edge1.x + edge1.y * edge1.y);
            float len2 = std::sqrt(edge2.x * edge2.x + edge2.y * edge2.y);
            if (len1 > EPSILON) { edge1.x /= len1; edge1.y /= len1; }
            if (len2 > EPSILON) { edge2.x /= len2; edge2.y /= len2; }
            
            // Perpendiculars (pointing outward - left side of edge direction)  
            Vec2 perp1(-edge1.y, edge1.x);
            Vec2 perp2(-edge2.y, edge2.x);
            
            // Bisector direction
            Vec2 bisect(perp1.x + perp2.x, perp1.y + perp2.y);
            float bisect_len = std::sqrt(bisect.x * bisect.x + bisect.y * bisect.y);
            if (bisect_len > EPSILON) {
                bisect.x /= bisect_len;
                bisect.y /= bisect_len;
            } else {
                // Fallback: direction away from centroid
                bisect.x = vertex.x - centroid.x;
                bisect.y = vertex.y - centroid.y;
                bisect_len = std::sqrt(bisect.x * bisect.x + bisect.y * bisect.y);
                if (bisect_len > EPSILON) { bisect.x /= bisect_len; bisect.y /= bisect_len; }
            }
            
            // Try different offset sizes, take the largest that works
            const float offsets[] = {OBSTACLE_OFFSET, OBSTACLE_OFFSET * 0.5f, 3.0f};
            for (float offset : offsets) {
                Vec2 offset_vertex(vertex.x + bisect.x * offset,
                                   vertex.y + bisect.y * offset);
                
                if (is_point_walkable(offset_vertex, walkable_areas, obstacles)) {
                    nodes.push_back(offset_vertex);
                    break;
                }
            }
        }
    }
    
    // Also add walkable area vertices (helps with complex walkable shapes)
    for (const auto& area : walkable_areas) {
        if (!area.is_valid()) continue;
        
        // Calculate area centroid
        Vec2 centroid(0.0f, 0.0f);
        for (const auto& p : area.points) {
            centroid.x += p.x;
            centroid.y += p.y;
        }
        centroid.x /= area.points.size();
        centroid.y /= area.points.size();
        
        size_t n = area.points.size();
        for (size_t vi = 0; vi < n; vi++) {
            const Vec2& vertex = area.points[vi];
            
            // Direction toward centroid (inward)
            Vec2 inward(centroid.x - vertex.x, centroid.y - vertex.y);
            float len = std::sqrt(inward.x * inward.x + inward.y * inward.y);
            if (len > EPSILON) {
                inward.x /= len;
                inward.y /= len;
            }
            
            // Try different offset sizes
            const float offsets[] = {OBSTACLE_OFFSET, OBSTACLE_OFFSET * 0.5f, 3.0f};
            for (float offset : offsets) {
                Vec2 offset_vertex(vertex.x + inward.x * offset,
                                   vertex.y + inward.y * offset);
                
                if (is_point_walkable(offset_vertex, walkable_areas, obstacles)) {
                    nodes.push_back(offset_vertex);
                    break;
                }
            }
        }
    }
    
    // Limit nodes to prevent performance issues
    if (nodes.size() > MAX_PATH_NODES) {
        nodes.resize(MAX_PATH_NODES);
    }
    
    size_t num_nodes = nodes.size();
    size_t start_idx = 0;
    size_t goal_idx = 1;
    
    // Build adjacency: which nodes can see each other
    // Using vector of vectors for adjacency list
    std::vector<std::vector<std::pair<size_t, float>>> adjacency(num_nodes);
    
    int total_edges = 0;
    for (size_t i = 0; i < num_nodes; i++) {
        for (size_t j = i + 1; j < num_nodes; j++) {
            if (has_line_of_sight(nodes[i], nodes[j], walkable_areas, obstacles)) {
                float dist = distance(nodes[i], nodes[j]);
                adjacency[i].push_back({j, dist});
                adjacency[j].push_back({i, dist});
                total_edges++;
            }
        }
    }
    
    (void)total_edges; // Suppress unused warning
    
    // A* algorithm
    std::vector<float> g_score(num_nodes, std::numeric_limits<float>::max());
    std::vector<float> f_score(num_nodes, std::numeric_limits<float>::max());
    std::unordered_map<size_t, size_t> came_from;
    
    g_score[start_idx] = 0.0f;
    f_score[start_idx] = distance(start, resolved_target); // h = distance to goal
    
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open_set;
    open_set.push({start_idx, f_score[start_idx]});
    
    std::vector<bool> closed(num_nodes, false);
    
    while (!open_set.empty()) {
        AStarNode current = open_set.top();
        open_set.pop();
        
        if (current.index == goal_idx) {
            // Found path!
            return reconstruct_path(nodes, came_from, start_idx, goal_idx);
        }
        
        if (closed[current.index]) {
            continue;
        }
        closed[current.index] = true;
        
        // Explore neighbors
        for (const auto& [neighbor_idx, edge_dist] : adjacency[current.index]) {
            if (closed[neighbor_idx]) {
                continue;
            }
            
            float tentative_g = g_score[current.index] + edge_dist;
            
            if (tentative_g < g_score[neighbor_idx]) {
                came_from[neighbor_idx] = current.index;
                g_score[neighbor_idx] = tentative_g;
                f_score[neighbor_idx] = tentative_g + distance(nodes[neighbor_idx], resolved_target);
                open_set.push({neighbor_idx, f_score[neighbor_idx]});
            }
        }
    }
    
    // No path found - try to get as close as possible
    // Find the closest reachable node to target
    float best_dist = std::numeric_limits<float>::max();
    size_t best_node = start_idx;
    
    for (size_t i = 0; i < num_nodes; i++) {
        if (g_score[i] < std::numeric_limits<float>::max()) {
            float dist_to_target = distance(nodes[i], resolved_target);
            if (dist_to_target < best_dist && i != start_idx) {
                best_dist = dist_to_target;
                best_node = i;
            }
        }
    }
    
    if (best_node != start_idx) {
        return reconstruct_path(nodes, came_from, start_idx, best_node);
    }
    
    return result; // Empty path
}

} // namespace Navigation
