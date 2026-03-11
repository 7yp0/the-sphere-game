#include "polygon_utils.h"
#include <cmath>
#include <algorithm>

namespace Collision {

// Helper: calculate distance from point to line segment
static float distance_point_to_segment(Vec2 p, Vec2 a, Vec2 b) {
    Vec2 ab = Vec2(b.x - a.x, b.y - a.y);
    Vec2 ap = Vec2(p.x - a.x, p.y - a.y);
    
    float ab_len_sq = ab.x * ab.x + ab.y * ab.y;
    if (ab_len_sq < 0.0001f) {
        return std::sqrt(ap.x * ap.x + ap.y * ap.y);
    }
    
    float t = (ap.x * ab.x + ap.y * ab.y) / ab_len_sq;
    t = std::max(0.0f, std::min(1.0f, t));
    
    Vec2 closest = Vec2(a.x + t * ab.x, a.y + t * ab.y);
    Vec2 diff = Vec2(p.x - closest.x, p.y - closest.y);
    return std::sqrt(diff.x * diff.x + diff.y * diff.y);
}

// Helper: find closest point on a line segment to a point
static Vec2 closest_point_on_segment(Vec2 p, Vec2 a, Vec2 b) {
    Vec2 ab = Vec2(b.x - a.x, b.y - a.y);
    Vec2 ap = Vec2(p.x - a.x, p.y - a.y);
    
    float ab_len_sq = ab.x * ab.x + ab.y * ab.y;
    if (ab_len_sq < 0.0001f) {
        return a;
    }
    
    float t = (ap.x * ab.x + ap.y * ab.y) / ab_len_sq;
    t = std::max(0.0f, std::min(1.0f, t));
    
    return Vec2(a.x + t * ab.x, a.y + t * ab.y);
}

// Helper: cross product of vectors (p1 -> p2) x (p1 -> p3)
static float cross_product(Vec2 p1, Vec2 p2, Vec2 p3) {
    return (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
}

// Helper: check if two line segments intersect
static bool segments_intersect(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4) {
    float d1 = cross_product(p3, p4, p1);
    float d2 = cross_product(p3, p4, p2);
    float d3 = cross_product(p1, p2, p3);
    float d4 = cross_product(p1, p2, p4);
    
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
        ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) {
        return true;
    }
    
    return false;
}

bool point_in_polygon(Vec2 point, const Polygon& polygon) {
    if (!polygon.is_valid()) return false;
    
    // Ray casting algorithm: cast a ray from point to infinity and count crossings
    int crossings = 0;
    size_t n = polygon.points.size();
    
    for (size_t i = 0; i < n; i++) {
        Vec2 p1 = polygon.points[i];
        Vec2 p2 = polygon.points[(i + 1) % n];
        
        // Check if ray crosses this edge
        if ((p1.y <= point.y && point.y < p2.y) || 
            (p2.y <= point.y && point.y < p1.y)) {
            
            // Calculate x-coordinate of intersection
            float x_intersect = p1.x + (point.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
            
            if (point.x < x_intersect) {
                crossings++;
            }
        }
    }
    
    return (crossings % 2) == 1;
}

Vec2 closest_point_on_polygon(Vec2 point, const Polygon& polygon) {
    if (!polygon.is_valid()) return point;
    
    Vec2 closest = polygon.points[0];
    float min_dist = distance_point_to_segment(point, polygon.points[0], polygon.points[1]);
    
    size_t n = polygon.points.size();
    for (size_t i = 0; i < n; i++) {
        Vec2 p1 = polygon.points[i];
        Vec2 p2 = polygon.points[(i + 1) % n];
        
        float dist = distance_point_to_segment(point, p1, p2);
        if (dist < min_dist) {
            min_dist = dist;
            closest = closest_point_on_segment(point, p1, p2);
        }
    }
    
    return closest;
}

bool line_intersects_polygon(Vec2 start, Vec2 end, const Polygon& polygon) {
    if (!polygon.is_valid()) return false;
    
    // Check if line segment intersects any edge of the polygon
    size_t n = polygon.points.size();
    for (size_t i = 0; i < n; i++) {
        Vec2 p1 = polygon.points[i];
        Vec2 p2 = polygon.points[(i + 1) % n];
        
        if (segments_intersect(start, end, p1, p2)) {
            return true;
        }
    }
    
    return false;
}

bool point_on_polygon_edge(Vec2 point, const Polygon& polygon, float epsilon) {
    if (!polygon.is_valid()) return false;
    
    size_t n = polygon.points.size();
    for (size_t i = 0; i < n; i++) {
        Vec2 p1 = polygon.points[i];
        Vec2 p2 = polygon.points[(i + 1) % n];
        
        float dist = distance_point_to_segment(point, p1, p2);
        if (dist <= epsilon) {
            return true;
        }
    }
    
    return false;
}

bool polygons_intersect(const Polygon& poly1, const Polygon& poly2) {
    if (!poly1.is_valid() || !poly2.is_valid()) return false;
    
    // Check if any vertex of poly1 is inside poly2
    for (const auto& point : poly1.points) {
        if (point_in_polygon(point, poly2)) {
            return true;
        }
    }
    
    // Check if any vertex of poly2 is inside poly1
    for (const auto& point : poly2.points) {
        if (point_in_polygon(point, poly1)) {
            return true;
        }
    }
    
    // Check if any edges intersect
    for (size_t i = 0; i < poly1.points.size(); i++) {
        Vec2 p1 = poly1.points[i];
        Vec2 p2 = poly1.points[(i + 1) % poly1.points.size()];
        
        if (line_intersects_polygon(p1, p2, poly2)) {
            return true;
        }
    }
    
    return false;
}

bool point_in_any_polygon(Vec2 point, const std::vector<Polygon>& polygons) {
    for (const auto& poly : polygons) {
        if (point_in_polygon(point, poly)) {
            return true;
        }
    }
    return false;
}

bool polygon_inside_polygon(const Polygon& inner, const Polygon& outer) {
    // Check if all vertices of inner polygon are inside outer polygon
    for (const auto& vertex : inner.points) {
        if (!point_in_polygon(vertex, outer)) {
            return false;
        }
    }
    return true;
}

bool point_in_walkable_area(Vec2 point, const std::vector<Polygon>& polygons) {
    if (polygons.empty()) return false;
    
    // Count how many polygons contain this point
    // If point is in an odd number of polygons, it's walkable
    // If point is in an even number > 0, it's in a "hole"
    int containment_count = 0;
    
    for (const auto& poly : polygons) {
        if (point_in_polygon(point, poly)) {
            containment_count++;
        }
    }
    
    // Odd = walkable, Even = hole (or outside if 0)
    return (containment_count % 2) == 1;
}

Vec2 closest_point_on_any_polygon(Vec2 point, const std::vector<Polygon>& polygons) {
    if (polygons.empty()) return point;
    
    Vec2 closest = closest_point_on_polygon(point, polygons[0]);
    float min_dist = distance_point_to_segment(point, closest, closest);
    
    for (const auto& poly : polygons) {
        Vec2 candidate = closest_point_on_polygon(point, poly);
        float dist = distance_point_to_segment(point, candidate, candidate);
        if (dist < min_dist) {
            min_dist = dist;
            closest = candidate;
        }
    }
    
    return closest;
}

EdgeInfo find_closest_edge(Vec2 point, const std::vector<Polygon>& polygons) {
    EdgeInfo result;
    result.valid = false;
    result.is_hole = false;
    
    if (polygons.empty()) return result;
    
    float min_dist = 1e10f;
    size_t best_poly_idx = 0;
    
    for (size_t poly_idx = 0; poly_idx < polygons.size(); poly_idx++) {
        const auto& poly = polygons[poly_idx];
        if (!poly.is_valid()) continue;
        
        size_t n = poly.points.size();
        for (size_t i = 0; i < n; i++) {
            Vec2 p1 = poly.points[i];
            Vec2 p2 = poly.points[(i + 1) % n];
            
            float dist = distance_point_to_segment(point, p1, p2);
            if (dist < min_dist) {
                min_dist = dist;
                result.start = p1;
                result.end = p2;
                result.valid = true;
                best_poly_idx = poly_idx;
                
                // Calculate outward-facing normal (perpendicular to edge)
                // For counter-clockwise polygon, outward normal is to the right of edge direction
                Vec2 edge_vec = Vec2(p2.x - p1.x, p2.y - p1.y);
                float edge_len = std::sqrt(edge_vec.x * edge_vec.x + edge_vec.y * edge_vec.y);
                if (edge_len > 0.0001f) {
                    edge_vec.x /= edge_len;
                    edge_vec.y /= edge_len;
                    // Perpendicular: rotate 90 degrees (right-hand rule for outward)
                    result.normal = Vec2(-edge_vec.y, edge_vec.x);
                } else {
                    result.normal = Vec2(0.0f, 0.0f);
                }
            }
        }
    }
    
    // Check if the best polygon is a hole (inside another polygon)
    if (result.valid && polygons.size() > 1) {
        const Polygon& best_poly = polygons[best_poly_idx];
        for (size_t other_idx = 0; other_idx < polygons.size(); other_idx++) {
            if (other_idx == best_poly_idx) continue;
            if (polygon_inside_polygon(best_poly, polygons[other_idx])) {
                result.is_hole = true;
                break;
            }
        }
    }
    
    return result;
}

Vec2 get_edge_direction_toward_target(const EdgeInfo& edge, Vec2 target) {
    if (!edge.valid) return Vec2(0.0f, 0.0f);
    
    // Edge vector from start to end
    Vec2 edge_vec = Vec2(edge.end.x - edge.start.x, edge.end.y - edge.start.y);
    float edge_len_sq = edge_vec.x * edge_vec.x + edge_vec.y * edge_vec.y;
    
    if (edge_len_sq < 0.0001f) return Vec2(0.0f, 0.0f);
    
    float edge_len = std::sqrt(edge_len_sq);
    Vec2 edge_dir = Vec2(edge_vec.x / edge_len, edge_vec.y / edge_len);
    
    // Vector from edge start to target
    Vec2 to_target = Vec2(target.x - edge.start.x, target.y - edge.start.y);
    
    // Dot product: positive = target is in direction of edge_dir from start
    float dot = edge_dir.x * to_target.x + edge_dir.y * to_target.y;
    
    // Return direction along edge that leads toward target
    if (dot >= 0.0f) {
        return edge_dir;
    } else {
        return Vec2(-edge_dir.x, -edge_dir.y);
    }
}

EdgeInfo find_next_edge_at_vertex(const EdgeInfo& current_edge, Vec2 vertex, Vec2 target, 
                                   const std::vector<Polygon>& polygons) {
    EdgeInfo result;
    result.valid = false;
    result.is_hole = false;
    
    if (polygons.empty()) return result;
    
    const float vertex_epsilon = 2.0f;
    float best_score = -1e10f;
    size_t best_poly_idx = 0;
    
    // Direction from vertex to target
    Vec2 to_target = Vec2(target.x - vertex.x, target.y - vertex.y);
    float to_target_len = std::sqrt(to_target.x * to_target.x + to_target.y * to_target.y);
    if (to_target_len > 0.001f) {
        to_target.x /= to_target_len;
        to_target.y /= to_target_len;
    }
    
    for (size_t poly_idx = 0; poly_idx < polygons.size(); poly_idx++) {
        const auto& poly = polygons[poly_idx];
        if (!poly.is_valid()) continue;
        
        size_t n = poly.points.size();
        for (size_t i = 0; i < n; i++) {
            Vec2 p1 = poly.points[i];
            Vec2 p2 = poly.points[(i + 1) % n];
            
            // Check if this edge connects to our vertex
            bool connects_at_start = (std::abs(p1.x - vertex.x) < vertex_epsilon && 
                                      std::abs(p1.y - vertex.y) < vertex_epsilon);
            bool connects_at_end = (std::abs(p2.x - vertex.x) < vertex_epsilon && 
                                    std::abs(p2.y - vertex.y) < vertex_epsilon);
            
            if (!connects_at_start && !connects_at_end) continue;
            
            // Skip the current edge
            bool is_same_edge = (std::abs(p1.x - current_edge.start.x) < vertex_epsilon &&
                                 std::abs(p1.y - current_edge.start.y) < vertex_epsilon &&
                                 std::abs(p2.x - current_edge.end.x) < vertex_epsilon &&
                                 std::abs(p2.y - current_edge.end.y) < vertex_epsilon) ||
                                (std::abs(p1.x - current_edge.end.x) < vertex_epsilon &&
                                 std::abs(p1.y - current_edge.end.y) < vertex_epsilon &&
                                 std::abs(p2.x - current_edge.start.x) < vertex_epsilon &&
                                 std::abs(p2.y - current_edge.start.y) < vertex_epsilon);
            
            if (is_same_edge) continue;
            
            // Calculate direction along this edge away from vertex
            Vec2 edge_away_dir;
            if (connects_at_start) {
                edge_away_dir = Vec2(p2.x - p1.x, p2.y - p1.y);
            } else {
                edge_away_dir = Vec2(p1.x - p2.x, p1.y - p2.y);
            }
            float edge_len = std::sqrt(edge_away_dir.x * edge_away_dir.x + edge_away_dir.y * edge_away_dir.y);
            if (edge_len < 0.001f) continue;
            edge_away_dir.x /= edge_len;
            edge_away_dir.y /= edge_len;
            
            // Score: how well does this edge direction align with direction to target?
            float score = edge_away_dir.x * to_target.x + edge_away_dir.y * to_target.y;
            
            if (score > best_score) {
                best_score = score;
                result.start = p1;
                result.end = p2;
                result.valid = true;
                best_poly_idx = poly_idx;
                
                // Calculate normal
                Vec2 edge_vec = Vec2(p2.x - p1.x, p2.y - p1.y);
                edge_vec.x /= edge_len;
                edge_vec.y /= edge_len;
                result.normal = Vec2(-edge_vec.y, edge_vec.x);
            }
        }
    }
    
    // Check if the best polygon is a hole (inside another polygon)
    if (result.valid && polygons.size() > 1) {
        const Polygon& best_poly = polygons[best_poly_idx];
        for (size_t other_idx = 0; other_idx < polygons.size(); other_idx++) {
            if (other_idx == best_poly_idx) continue;
            if (polygon_inside_polygon(best_poly, polygons[other_idx])) {
                result.is_hole = true;
                break;
            }
        }
    }
    
    return result;
}

}
