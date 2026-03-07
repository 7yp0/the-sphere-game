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

}
