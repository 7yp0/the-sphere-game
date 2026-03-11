#pragma once

#include "types.h"
#include <vector>

namespace Collision {

struct Polygon {
    std::vector<Vec2> points;
    
    Polygon() = default;
    explicit Polygon(const std::vector<Vec2>& vertices) : points(vertices) {}
    
    bool is_valid() const { return points.size() >= 3; }
    size_t vertex_count() const { return points.size(); }
};

// Point-in-polygon test using ray casting algorithm
// Returns true if point is inside the polygon (including boundary)
bool point_in_polygon(Vec2 point, const Polygon& polygon);

// Find the closest point on the polygon boundary to the given point
// Returns the closest point on the polygon perimeter
Vec2 closest_point_on_polygon(Vec2 point, const Polygon& polygon);

// Check if a line segment intersects with the polygon boundary
// start: line segment start point
// end: line segment end point
// Returns true if the line segment intersects any polygon edge
bool line_intersects_polygon(Vec2 start, Vec2 end, const Polygon& polygon);

// Check if a point is on the polygon edge (within epsilon tolerance)
// Returns true if point lies on any edge
bool point_on_polygon_edge(Vec2 point, const Polygon& polygon, float epsilon = 0.1f);

// Polygon-polygon collision test
// Returns true if the two polygons overlap or touch
bool polygons_intersect(const Polygon& poly1, const Polygon& poly2);

// Utility: Check if point is inside any polygon in the list
bool point_in_any_polygon(Vec2 point, const std::vector<Polygon>& polygons);

// Utility: Check if point is in walkable area (handles holes)
// A polygon completely inside another acts as a hole/obstacle
bool point_in_walkable_area(Vec2 point, const std::vector<Polygon>& polygons);

// Check if polygon A is completely inside polygon B
bool polygon_inside_polygon(const Polygon& inner, const Polygon& outer);

// Utility: Find the closest point across all polygons
// Returns closest point on any polygon boundary
Vec2 closest_point_on_any_polygon(Vec2 point, const std::vector<Polygon>& polygons);

// Edge collision detection result
struct EdgeInfo {
    Vec2 start;           // Edge start point
    Vec2 end;             // Edge end point
    Vec2 normal;          // Outward-facing normal
    bool valid = false;   // True if edge was found
};

// Find the closest edge on any polygon walkable boundary
// For walkable areas, this returns the edge closest to the point
// point: the position to test
// polygons: array of walkable area polygons
EdgeInfo find_closest_edge(Vec2 point, const std::vector<Polygon>& polygons);

// Calculate which direction along an edge leads toward the target
// Returns normalized direction vector along edge (from edge_start toward edge_end or reverse)
// edge: the edge info
// target: the target position the player wants to reach
Vec2 get_edge_direction_toward_target(const EdgeInfo& edge, Vec2 target);

// Find the next edge at a vertex that best continues toward the target
// current_edge: the edge we just finished following
// vertex: the endpoint we reached (either current_edge.start or current_edge.end)
// target: the target position we're trying to reach
// polygons: the walkable area polygons
// Returns: the next edge to follow, or invalid EdgeInfo if none found
EdgeInfo find_next_edge_at_vertex(const EdgeInfo& current_edge, Vec2 vertex, Vec2 target, 
                                   const std::vector<Polygon>& polygons);

}
