#include "geometry_editor.h"
#include "debug_log.h"
#include "renderer/renderer.h"
#include "renderer/text.h"
#include "game/game.h"
#include "platform.h"
#include "config.h"
#include <cstdio>
#include <cmath>
#include <cfloat>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unistd.h>

// For finding executable path on macOS
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace GeometryEditor {

static EditorState g_state;
static Vec2 g_mouse_pos;

// macOS virtual key codes
#ifdef __APPLE__
    static constexpr int KEY_W = 13;
    static constexpr int KEY_H = 4;
    static constexpr int KEY_O = 31;
    static constexpr int KEY_F = 3;
    static constexpr int KEY_R = 15;
    static constexpr int KEY_ESC = 53;
    static constexpr int KEY_DELETE = 51;
#else
    static constexpr int KEY_W = 0x57;
    static constexpr int KEY_H = 0x48;
    static constexpr int KEY_O = 0x4F;
    static constexpr int KEY_F = 0x46;
    static constexpr int KEY_R = 0x52;
    static constexpr int KEY_ESC = 0x1B;
    static constexpr int KEY_DELETE = 0x2E;
#endif

// Previous key states for edge detection
static bool s_prev_w = false;
static bool s_prev_h = false;
static bool s_prev_o = false;
static bool s_prev_f = false;
static bool s_prev_r = false;
static bool s_prev_esc = false;
static bool s_prev_del = false;

static constexpr float VERTEX_SELECT_RADIUS = 8.0f;  // Pixels in base resolution
static constexpr float EDGE_SELECT_DISTANCE = 6.0f;  // Distance from edge to detect click
static int s_polygon_counter = 0;

// Helper function to get project root directory
static std::string get_project_root() {
#ifdef __APPLE__
    // Get the path to the executable
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        std::string exe_path(path);
        // Find "build" in path and go to parent
        size_t pos = exe_path.find("/build/");
        if (pos != std::string::npos) {
            return exe_path.substr(0, pos) + "/";
        }
        // Try finding .app bundle
        pos = exe_path.find(".app/");
        if (pos != std::string::npos) {
            // Go back to find the real project root before .app
            std::string before_app = exe_path.substr(0, pos);
            size_t last_slash = before_app.rfind("/");
            if (last_slash != std::string::npos) {
                before_app = before_app.substr(0, last_slash);
                // Check if this is inside build/Debug
                size_t build_pos = before_app.find("/build/");
                if (build_pos != std::string::npos) {
                    return before_app.substr(0, build_pos) + "/";
                }
            }
        }
    }
    // Fallback: try current working directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::string cwd_str(cwd);
        size_t pos = cwd_str.find("/build/");
        if (pos != std::string::npos) {
            return cwd_str.substr(0, pos) + "/";
        }
    }
    // Last resort: current directory (will likely fail gracefully)
    DEBUG_LOG("[GeoEditor] Warning: Could not determine project root");
    return "./";
#else
    // Windows: use relative path as before
    return "../../";
#endif
}

void init() {
    g_state = EditorState();
    s_polygon_counter = 0;
}

const EditorState& get_state() {
    return g_state;
}

bool is_active() {
    return g_state.is_active;
}

void toggle() {
    g_state.is_active = !g_state.is_active;
    if (!g_state.is_active) {
        // Reset creation state when deactivating
        g_state.mode = EditorMode::NONE;
        g_state.current_polygon_points.clear();
        g_state.selected_polygon_index = -1;
        g_state.selected_vertex_index = -1;
    }
}

const char* get_mode_string() {
    switch (g_state.mode) {
        case EditorMode::CREATING_WALKABLE: return "CREATING WALKABLE";
        case EditorMode::CREATING_HOTSPOT: return "CREATING HOTSPOT";
        case EditorMode::CREATING_OBSTACLE: return "CREATING OBSTACLE";
        default: return "SELECT";
    }
}

// Calculate polygon area using Shoelace formula (returns absolute value)
static float polygon_area(const Collision::Polygon& poly) {
    float area = 0.0f;
    size_t n = poly.points.size();
    if (n < 3) return 0.0f;
    
    for (size_t i = 0; i < n; i++) {
        size_t j = (i + 1) % n;
        area += poly.points[i].x * poly.points[j].y;
        area -= poly.points[j].x * poly.points[i].y;
    }
    return fabsf(area) * 0.5f;
}

// Find the smallest polygon containing the point (for selection)
static bool find_polygon_containing_point(Vec2 pos, int& out_poly_idx, SelectionType& out_type) {
    const auto& scene = Game::g_state.scene;
    
    float smallest_area = FLT_MAX;
    int best_idx = -1;
    SelectionType best_type = SelectionType::NONE;
    
    // Check walkable areas
    for (size_t i = 0; i < scene.geometry.walkable_areas.size(); i++) {
        const auto& poly = scene.geometry.walkable_areas[i];
        if (Collision::point_in_polygon(pos, poly)) {
            float area = polygon_area(poly);
            if (area < smallest_area) {
                smallest_area = area;
                best_idx = (int)i;
                best_type = SelectionType::WALKABLE_AREA;
            }
        }
    }
    
    // Check obstacles
    for (size_t i = 0; i < scene.geometry.obstacles.size(); i++) {
        const auto& poly = scene.geometry.obstacles[i];
        if (Collision::point_in_polygon(pos, poly)) {
            float area = polygon_area(poly);
            if (area < smallest_area) {
                smallest_area = area;
                best_idx = (int)i;
                best_type = SelectionType::OBSTACLE;
            }
        }
    }
    
    // Check hotspots
    for (size_t i = 0; i < scene.geometry.hotspots.size(); i++) {
        const auto& poly = scene.geometry.hotspots[i].bounds;
        if (Collision::point_in_polygon(pos, poly)) {
            float area = polygon_area(poly);
            if (area < smallest_area) {
                smallest_area = area;
                best_idx = (int)i;
                best_type = SelectionType::HOTSPOT;
            }
        }
    }
    
    if (best_idx >= 0) {
        out_poly_idx = best_idx;
        out_type = best_type;
        return true;
    }
    return false;
}

// Find vertex near position - ONLY searches the currently selected polygon
static bool find_vertex_at(Vec2 pos, int& out_vert_idx) {
    // Only search if a polygon is selected
    if (g_state.selected_polygon_index < 0 || g_state.selection_type == SelectionType::NONE) {
        return false;
    }
    
    const auto& scene = Game::g_state.scene;
    float radius_sq = VERTEX_SELECT_RADIUS * VERTEX_SELECT_RADIUS;
    
    const Collision::Polygon* poly = nullptr;
    
    switch (g_state.selection_type) {
        case SelectionType::WALKABLE_AREA:
            if (g_state.selected_polygon_index < (int)scene.geometry.walkable_areas.size()) {
                poly = &scene.geometry.walkable_areas[g_state.selected_polygon_index];
            }
            break;
        case SelectionType::OBSTACLE:
            if (g_state.selected_polygon_index < (int)scene.geometry.obstacles.size()) {
                poly = &scene.geometry.obstacles[g_state.selected_polygon_index];
            }
            break;
        case SelectionType::HOTSPOT:
            if (g_state.selected_polygon_index < (int)scene.geometry.hotspots.size()) {
                poly = &scene.geometry.hotspots[g_state.selected_polygon_index].bounds;
            }
            break;
        default:
            return false;
    }
    
    if (!poly) return false;
    
    for (size_t vi = 0; vi < poly->points.size(); vi++) {
        float dx = pos.x - poly->points[vi].x;
        float dy = pos.y - poly->points[vi].y;
        if (dx*dx + dy*dy <= radius_sq) {
            out_vert_idx = (int)vi;
            return true;
        }
    }
    
    return false;
}

// Calculate distance from point to line segment
static float point_to_segment_distance(Vec2 p, Vec2 a, Vec2 b) {
    Vec2 ab(b.x - a.x, b.y - a.y);
    Vec2 ap(p.x - a.x, p.y - a.y);
    
    float ab_sq = ab.x * ab.x + ab.y * ab.y;
    if (ab_sq < 0.0001f) {
        // a and b are same point
        return sqrtf(ap.x * ap.x + ap.y * ap.y);
    }
    
    // Project point onto line, clamped to segment
    float t = (ap.x * ab.x + ap.y * ab.y) / ab_sq;
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    
    // Closest point on segment
    Vec2 closest(a.x + t * ab.x, a.y + t * ab.y);
    float dx = p.x - closest.x;
    float dy = p.y - closest.y;
    return sqrtf(dx * dx + dy * dy);
}

// Find edge near position - ONLY searches the currently selected polygon
static bool find_edge_at(Vec2 pos, int& out_edge_idx) {
    // Only search if a polygon is selected
    if (g_state.selected_polygon_index < 0 || g_state.selection_type == SelectionType::NONE) {
        return false;
    }
    
    const auto& scene = Game::g_state.scene;
    const Collision::Polygon* poly = nullptr;
    
    switch (g_state.selection_type) {
        case SelectionType::WALKABLE_AREA:
            if (g_state.selected_polygon_index < (int)scene.geometry.walkable_areas.size()) {
                poly = &scene.geometry.walkable_areas[g_state.selected_polygon_index];
            }
            break;
        case SelectionType::OBSTACLE:
            if (g_state.selected_polygon_index < (int)scene.geometry.obstacles.size()) {
                poly = &scene.geometry.obstacles[g_state.selected_polygon_index];
            }
            break;
        case SelectionType::HOTSPOT:
            if (g_state.selected_polygon_index < (int)scene.geometry.hotspots.size()) {
                poly = &scene.geometry.hotspots[g_state.selected_polygon_index].bounds;
            }
            break;
        default:
            return false;
    }
    
    if (!poly) return false;
    
    size_t n = poly->points.size();
    for (size_t ei = 0; ei < n; ei++) {
        Vec2 a = poly->points[ei];
        Vec2 b = poly->points[(ei + 1) % n];
        float dist = point_to_segment_distance(pos, a, b);
        if (dist <= EDGE_SELECT_DISTANCE) {
            out_edge_idx = (int)ei;
            return true;
        }
    }
    
    return false;
}

// Check if first vertex of current polygon is clicked (to close polygon)
static bool is_closing_polygon(Vec2 pos) {
    if (g_state.current_polygon_points.size() < 3) return false;
    
    Vec2 first = g_state.current_polygon_points[0];
    float dx = pos.x - first.x;
    float dy = pos.y - first.y;
    return (dx*dx + dy*dy) <= (VERTEX_SELECT_RADIUS * VERTEX_SELECT_RADIUS);
}

// Finish creating current polygon
static void finish_polygon() {
    if (g_state.current_polygon_points.size() < 3) {
        DEBUG_ERROR("[GeoEditor] Cannot create polygon with less than 3 vertices");
        g_state.current_polygon_points.clear();
        g_state.mode = EditorMode::NONE;
        return;
    }
    
    auto& scene = Game::g_state.scene;
    
    if (g_state.mode == EditorMode::CREATING_WALKABLE) {
        Collision::Polygon poly(g_state.current_polygon_points);
        if (!Collision::is_polygon_convex(poly)) {
            DEBUG_ERROR("[GeoEditor] WARNING: Walkable area is NOT CONVEX! Collision may behave incorrectly.");
        }
        scene.geometry.walkable_areas.push_back(poly);
        DEBUG_INFO("[GeoEditor] Created walkable area with %zu vertices", g_state.current_polygon_points.size());
    } else if (g_state.mode == EditorMode::CREATING_OBSTACLE) {
        Collision::Polygon poly(g_state.current_polygon_points);
        if (!Collision::is_polygon_convex(poly)) {
            DEBUG_ERROR("[GeoEditor] WARNING: Obstacle is NOT CONVEX! Sliding may behave incorrectly.");
        }
        scene.geometry.obstacles.push_back(poly);
        DEBUG_INFO("[GeoEditor] Created obstacle with %zu vertices", g_state.current_polygon_points.size());
    } else if (g_state.mode == EditorMode::CREATING_HOTSPOT) {
        Scene::Hotspot hotspot;
        hotspot.name = g_state.current_polygon_name;
        hotspot.bounds = Collision::Polygon(g_state.current_polygon_points);
        hotspot.interaction_distance = 20.0f;  // Default distance
        hotspot.enabled = true;
        scene.geometry.hotspots.push_back(hotspot);
        DEBUG_INFO("[GeoEditor] Created hotspot '%s' with %zu vertices", 
               hotspot.name.c_str(), g_state.current_polygon_points.size());
    }
    
    // Auto-save
    save_geometry(scene.name.c_str());
    
    g_state.current_polygon_points.clear();
    g_state.mode = EditorMode::NONE;
}

// Delete selected polygon
static void delete_selected() {
    if (g_state.selected_polygon_index < 0) return;
    
    auto& scene = Game::g_state.scene;
    
    if (g_state.selection_type == SelectionType::WALKABLE_AREA) {
        if (g_state.selected_polygon_index < (int)scene.geometry.walkable_areas.size()) {
            scene.geometry.walkable_areas.erase(
                scene.geometry.walkable_areas.begin() + g_state.selected_polygon_index);
            DEBUG_INFO("[GeoEditor] Deleted walkable area %d", g_state.selected_polygon_index);
        }
    } else if (g_state.selection_type == SelectionType::OBSTACLE) {
        if (g_state.selected_polygon_index < (int)scene.geometry.obstacles.size()) {
            scene.geometry.obstacles.erase(
                scene.geometry.obstacles.begin() + g_state.selected_polygon_index);
            DEBUG_INFO("[GeoEditor] Deleted obstacle %d", g_state.selected_polygon_index);
        }
    } else if (g_state.selection_type == SelectionType::HOTSPOT) {
        if (g_state.selected_polygon_index < (int)scene.geometry.hotspots.size()) {
            DEBUG_INFO("[GeoEditor] Deleted hotspot '%s'", 
                   scene.geometry.hotspots[g_state.selected_polygon_index].name.c_str());
            scene.geometry.hotspots.erase(
                scene.geometry.hotspots.begin() + g_state.selected_polygon_index);
        }
    }
    
    g_state.selected_polygon_index = -1;
    g_state.selected_vertex_index = -1;
    g_state.selection_type = SelectionType::NONE;
    
    // Auto-save
    save_geometry(scene.name.c_str());
}

// Insert vertex on edge at clicked position (uses currently selected polygon)
static void insert_vertex_on_edge(Vec2 click_pos, int edge_idx) {
    auto& scene = Game::g_state.scene;
    int poly_idx = g_state.selected_polygon_index;
    
    if (g_state.selection_type == SelectionType::WALKABLE_AREA) {
        if (poly_idx < (int)scene.geometry.walkable_areas.size()) {
            auto& poly = scene.geometry.walkable_areas[poly_idx];
            
            // Insert at click position (after edge_idx vertex)
            poly.points.insert(poly.points.begin() + edge_idx + 1, click_pos);
            DEBUG_INFO("[GeoEditor] Inserted vertex at (%.1f, %.1f) on edge %d of walkable area %d", 
                   click_pos.x, click_pos.y, edge_idx, poly_idx);
            
            // Select the new vertex for immediate dragging
            g_state.selected_vertex_index = edge_idx + 1;
            g_state.dragging = true;
            g_state.drag_start = click_pos;
        }
    } else if (g_state.selection_type == SelectionType::OBSTACLE) {
        if (poly_idx < (int)scene.geometry.obstacles.size()) {
            auto& poly = scene.geometry.obstacles[poly_idx];
            
            poly.points.insert(poly.points.begin() + edge_idx + 1, click_pos);
            DEBUG_INFO("[GeoEditor] Inserted vertex at (%.1f, %.1f) on edge %d of obstacle %d", 
                   click_pos.x, click_pos.y, edge_idx, poly_idx);
            
            g_state.selected_vertex_index = edge_idx + 1;
            g_state.dragging = true;
            g_state.drag_start = click_pos;
        }
    } else if (g_state.selection_type == SelectionType::HOTSPOT) {
        if (poly_idx < (int)scene.geometry.hotspots.size()) {
            auto& hotspot = scene.geometry.hotspots[poly_idx];
            
            hotspot.bounds.points.insert(hotspot.bounds.points.begin() + edge_idx + 1, click_pos);
            DEBUG_INFO("[GeoEditor] Inserted vertex at (%.1f, %.1f) on edge %d of hotspot '%s'", 
                   click_pos.x, click_pos.y, edge_idx, hotspot.name.c_str());
            
            g_state.selected_vertex_index = edge_idx + 1;
            g_state.dragging = true;
            g_state.drag_start = click_pos;
        }
    }
    
    // Auto-save
    save_geometry(scene.name.c_str());
}

// Delete a single vertex (right-click)
static void delete_vertex(int poly_idx, int vert_idx, SelectionType sel_type) {
    auto& scene = Game::g_state.scene;
    
    if (sel_type == SelectionType::WALKABLE_AREA) {
        if (poly_idx < (int)scene.geometry.walkable_areas.size()) {
            auto& poly = scene.geometry.walkable_areas[poly_idx];
            if (poly.points.size() <= 3) {
                DEBUG_ERROR("[GeoEditor] Cannot delete vertex: polygon needs at least 3 vertices");
                return;
            }
            if (vert_idx < (int)poly.points.size()) {
                poly.points.erase(poly.points.begin() + vert_idx);
                DEBUG_INFO("[GeoEditor] Deleted vertex %d from walkable area %d", vert_idx, poly_idx);
            }
        }
    } else if (sel_type == SelectionType::OBSTACLE) {
        if (poly_idx < (int)scene.geometry.obstacles.size()) {
            auto& poly = scene.geometry.obstacles[poly_idx];
            if (poly.points.size() <= 3) {
                DEBUG_ERROR("[GeoEditor] Cannot delete vertex: polygon needs at least 3 vertices");
                return;
            }
            if (vert_idx < (int)poly.points.size()) {
                poly.points.erase(poly.points.begin() + vert_idx);
                DEBUG_INFO("[GeoEditor] Deleted vertex %d from obstacle %d", vert_idx, poly_idx);
            }
        }
    } else if (sel_type == SelectionType::HOTSPOT) {
        if (poly_idx < (int)scene.geometry.hotspots.size()) {
            auto& hotspot = scene.geometry.hotspots[poly_idx];
            if (hotspot.bounds.points.size() <= 3) {
                DEBUG_ERROR("[GeoEditor] Cannot delete vertex: polygon needs at least 3 vertices");
                return;
            }
            if (vert_idx < (int)hotspot.bounds.points.size()) {
                hotspot.bounds.points.erase(hotspot.bounds.points.begin() + vert_idx);
                DEBUG_INFO("[GeoEditor] Deleted vertex %d from hotspot '%s'", vert_idx, hotspot.name.c_str());
            }
        }
    }
    
    // Clear selection
    g_state.selected_polygon_index = -1;
    g_state.selected_vertex_index = -1;
    g_state.selection_type = SelectionType::NONE;
    
    // Auto-save
    save_geometry(scene.name.c_str());
}

void update(Vec2 mouse_base_coords) {
    if (!g_state.is_active) return;
    
    g_mouse_pos = mouse_base_coords;
    
    // Edge-detect key presses
    bool key_w = Platform::key_pressed(KEY_W);
    bool key_h = Platform::key_pressed(KEY_H);
    bool key_o = Platform::key_pressed(KEY_O);
    bool key_f = Platform::key_pressed(KEY_F);
    bool key_r = Platform::key_pressed(KEY_R);
    bool key_esc = Platform::key_pressed(KEY_ESC);
    bool key_del = Platform::key_pressed(KEY_DELETE);
    
    // R - Reload geometry from JSON (preserves callbacks)
    if (key_r && !s_prev_r && g_state.mode == EditorMode::NONE) {
        load_geometry(Game::g_state.scene.name.c_str());
    }
    
    // W - Start creating walkable area
    if (key_w && !s_prev_w && g_state.mode == EditorMode::NONE) {
        g_state.mode = EditorMode::CREATING_WALKABLE;
        g_state.current_polygon_points.clear();
        g_state.current_polygon_name = "walkable_" + std::to_string(++s_polygon_counter);
        // Deselect any selected polygon
        g_state.selected_polygon_index = -1;
        g_state.selected_vertex_index = -1;
        g_state.selection_type = SelectionType::NONE;
        DEBUG_INFO("[GeoEditor] Creating new walkable area");
    }
    
    // O - Start creating obstacle
    if (key_o && !s_prev_o && g_state.mode == EditorMode::NONE) {
        g_state.mode = EditorMode::CREATING_OBSTACLE;
        g_state.current_polygon_points.clear();
        g_state.current_polygon_name = "obstacle_" + std::to_string(++s_polygon_counter);
        // Deselect any selected polygon
        g_state.selected_polygon_index = -1;
        g_state.selected_vertex_index = -1;
        g_state.selection_type = SelectionType::NONE;
        DEBUG_INFO("[GeoEditor] Creating new obstacle");
    }
    
    // H - Start creating hotspot
    if (key_h && !s_prev_h && g_state.mode == EditorMode::NONE) {
        g_state.mode = EditorMode::CREATING_HOTSPOT;
        g_state.current_polygon_points.clear();
        g_state.current_polygon_name = "hotspot_" + std::to_string(++s_polygon_counter);
        // Deselect any selected polygon
        g_state.selected_polygon_index = -1;
        g_state.selected_vertex_index = -1;
        g_state.selection_type = SelectionType::NONE;
        DEBUG_INFO("[GeoEditor] Creating new hotspot");
    }
    
    // F - Finish polygon
    if (key_f && !s_prev_f && g_state.mode != EditorMode::NONE) {
        finish_polygon();
    }
    
    // ESC - Cancel/deselect
    if (key_esc && !s_prev_esc) {
        if (g_state.mode != EditorMode::NONE) {
            g_state.mode = EditorMode::NONE;
            g_state.current_polygon_points.clear();
            DEBUG_INFO("[GeoEditor] Cancelled polygon creation");
        } else {
            g_state.selected_polygon_index = -1;
            g_state.selected_vertex_index = -1;
            g_state.selection_type = SelectionType::NONE;
        }
    }
    
    // DEL - Delete selected
    if (key_del && !s_prev_del) {
        delete_selected();
    }
    
    // Update drag
    if (g_state.dragging && g_state.selected_vertex_index >= 0) {
        auto& scene = Game::g_state.scene;
        Vec2 new_pos = mouse_base_coords;
        
        if (g_state.selection_type == SelectionType::WALKABLE_AREA) {
            if (g_state.selected_polygon_index < (int)scene.geometry.walkable_areas.size()) {
                auto& poly = scene.geometry.walkable_areas[g_state.selected_polygon_index];
                if (g_state.selected_vertex_index < (int)poly.points.size()) {
                    poly.points[g_state.selected_vertex_index] = new_pos;
                }
            }
        } else if (g_state.selection_type == SelectionType::OBSTACLE) {
            if (g_state.selected_polygon_index < (int)scene.geometry.obstacles.size()) {
                auto& poly = scene.geometry.obstacles[g_state.selected_polygon_index];
                if (g_state.selected_vertex_index < (int)poly.points.size()) {
                    poly.points[g_state.selected_vertex_index] = new_pos;
                }
            }
        } else if (g_state.selection_type == SelectionType::HOTSPOT) {
            if (g_state.selected_polygon_index < (int)scene.geometry.hotspots.size()) {
                auto& hotspot = scene.geometry.hotspots[g_state.selected_polygon_index];
                if (g_state.selected_vertex_index < (int)hotspot.bounds.points.size()) {
                    hotspot.bounds.points[g_state.selected_vertex_index] = new_pos;
                }
            }
        }
    }
    
    // Find hovered vertex (only on selected polygon)
    int vert_idx;
    if (find_vertex_at(mouse_base_coords, vert_idx)) {
        g_state.hovered_vertex_index = vert_idx;
    } else {
        g_state.hovered_vertex_index = -1;
    }
    
    s_prev_w = key_w;
    s_prev_h = key_h;
    s_prev_o = key_o;
    s_prev_f = key_f;
    s_prev_r = key_r;
    s_prev_esc = key_esc;
    s_prev_del = key_del;
}

void on_mouse_click(Vec2 pos) {
    if (!g_state.is_active) return;
    
    // Shift+Click on selected hotspot sets target_position
    if (Platform::shift_down() && 
        g_state.selection_type == SelectionType::HOTSPOT && 
        g_state.selected_polygon_index >= 0) {
        auto& scene = Game::g_state.scene;
        if (g_state.selected_polygon_index < (int)scene.geometry.hotspots.size()) {
            auto& hotspot = scene.geometry.hotspots[g_state.selected_polygon_index];
            hotspot.target_position = pos;
            hotspot.has_target_position = true;
            DEBUG_INFO("[GeoEditor] Set target position for hotspot '%s' to (%.1f, %.1f)", 
                   hotspot.name.c_str(), pos.x, pos.y);
            save_geometry(scene.name.c_str());
            return;
        }
    }
    
    // If creating polygon
    if (g_state.mode != EditorMode::NONE) {
        // Check if clicking first vertex to close
        if (is_closing_polygon(pos)) {
            finish_polygon();
            return;
        }
        
        // Add vertex
        g_state.current_polygon_points.push_back(pos);
        DEBUG_INFO("[GeoEditor] Added vertex (%.1f, %.1f) - total: %zu", 
               pos.x, pos.y, g_state.current_polygon_points.size());
        return;
    }
    
    // If a polygon is already selected, try to interact with its vertices/edges
    if (g_state.selected_polygon_index >= 0 && g_state.selection_type != SelectionType::NONE) {
        // Try to select vertex of the selected polygon
        int vert_idx;
        if (find_vertex_at(pos, vert_idx)) {
            g_state.selected_vertex_index = vert_idx;
            g_state.dragging = true;
            g_state.drag_start = pos;
            const char* type_name = (g_state.selection_type == SelectionType::HOTSPOT) ? "hotspot" :
                                    (g_state.selection_type == SelectionType::OBSTACLE) ? "obstacle" : "walkable";
            DEBUG_INFO("[GeoEditor] Started dragging %s %d, vertex %d", 
                   type_name, g_state.selected_polygon_index, vert_idx);
            return;
        }
        
        // Try to click on edge to insert vertex
        int edge_idx;
        if (find_edge_at(pos, edge_idx)) {
            insert_vertex_on_edge(pos, edge_idx);
            return;
        }
    }
    
    // Try to select a polygon (smallest area wins if overlapping)
    int poly_idx;
    SelectionType sel_type;
    if (find_polygon_containing_point(pos, poly_idx, sel_type)) {
        g_state.selected_polygon_index = poly_idx;
        g_state.selected_vertex_index = -1;  // No vertex selected yet
        g_state.selection_type = sel_type;
        
        const char* type_name = (sel_type == SelectionType::HOTSPOT) ? "hotspot" :
                                (sel_type == SelectionType::OBSTACLE) ? "obstacle" : "walkable area";
        if (sel_type == SelectionType::HOTSPOT) {
            const auto& scene = Game::g_state.scene;
            DEBUG_INFO("[GeoEditor] Selected %s '%s' (clicked inside)", 
                   type_name, scene.geometry.hotspots[poly_idx].name.c_str());
        } else {
            DEBUG_INFO("[GeoEditor] Selected %s %d (clicked inside)", type_name, poly_idx);
        }
        return;
    }
    
    // Deselect
    g_state.selected_polygon_index = -1;
    g_state.selected_vertex_index = -1;
    g_state.selection_type = SelectionType::NONE;
}

void on_mouse_right_click(Vec2 pos) {
    if (!g_state.is_active) return;
    if (g_state.mode != EditorMode::NONE) return;  // Don't delete while creating
    
    // Right-click on selected hotspot: delete target_position if it has one
    if (g_state.selection_type == SelectionType::HOTSPOT && 
        g_state.selected_polygon_index >= 0) {
        auto& scene = Game::g_state.scene;
        if (g_state.selected_polygon_index < (int)scene.geometry.hotspots.size()) {
            auto& hotspot = scene.geometry.hotspots[g_state.selected_polygon_index];
            // Check if click is inside the hotspot or near target_position
            if (hotspot.has_target_position) {
                float dx = pos.x - hotspot.target_position.x;
                float dy = pos.y - hotspot.target_position.y;
                float dist_sq = dx*dx + dy*dy;
                // Delete if clicking near target marker or inside hotspot
                if (dist_sq <= 100.0f || Collision::point_in_polygon(pos, hotspot.bounds)) {
                    hotspot.has_target_position = false;
                    hotspot.target_position = Vec2(0.0f, 0.0f);
                    DEBUG_INFO("[GeoEditor] Deleted target position for hotspot '%s'", 
                           hotspot.name.c_str());
                    save_geometry(scene.name.c_str());
                    return;
                }
            }
        }
    }
    
    // Try to find vertex at position (only on selected polygon)
    int vert_idx;
    if (find_vertex_at(pos, vert_idx)) {
        delete_vertex(g_state.selected_polygon_index, vert_idx, g_state.selection_type);
    }
}

void on_mouse_release() {
    if (g_state.dragging) {
        g_state.dragging = false;
        // Auto-save after drag
        save_geometry(Game::g_state.scene.name.c_str());
    }
}

void render() {
    if (!g_state.is_active) return;
    
    float scale_x = (float)Config::VIEWPORT_WIDTH / (float)Config::BASE_WIDTH;
    float scale_y = (float)Config::VIEWPORT_HEIGHT / (float)Config::BASE_HEIGHT;
    float ui_z = Layers::get_z_depth(Layer::UI);
    
    // Draw current polygon being created (yellow preview)
    if (g_state.mode != EditorMode::NONE && !g_state.current_polygon_points.empty()) {
        Vec4 preview_color = Vec4(1.0f, 1.0f, 0.0f, 0.9f);
        
        // Draw existing edges
        for (size_t i = 0; i < g_state.current_polygon_points.size(); i++) {
            Vec3 p1(g_state.current_polygon_points[i].x * scale_x, 
                    g_state.current_polygon_points[i].y * scale_y, ui_z);
            
            if (i + 1 < g_state.current_polygon_points.size()) {
                Vec3 p2(g_state.current_polygon_points[i + 1].x * scale_x,
                        g_state.current_polygon_points[i + 1].y * scale_y, ui_z);
                Renderer::render_line(p1, p2, preview_color, 2.0f);
            }
            
            // Vertices
            Renderer::render_rect(p1, Vec2(8.0f, 8.0f), preview_color);
        }
        
        // Draw preview line to mouse
        Vec3 last_pt(g_state.current_polygon_points.back().x * scale_x,
                     g_state.current_polygon_points.back().y * scale_y, ui_z);
        Vec3 mouse_pt(g_mouse_pos.x * scale_x, g_mouse_pos.y * scale_y, ui_z);
        Renderer::render_line(last_pt, mouse_pt, Vec4(1.0f, 1.0f, 0.0f, 0.5f), 1.0f);
        
        // If near first vertex, show closing preview
        if (g_state.current_polygon_points.size() >= 3) {
            Vec3 first_pt(g_state.current_polygon_points[0].x * scale_x,
                          g_state.current_polygon_points[0].y * scale_y, ui_z);
            float dx = g_mouse_pos.x - g_state.current_polygon_points[0].x;
            float dy = g_mouse_pos.y - g_state.current_polygon_points[0].y;
            if (dx*dx + dy*dy <= VERTEX_SELECT_RADIUS * VERTEX_SELECT_RADIUS) {
                Renderer::render_rect(first_pt, Vec2(12.0f, 12.0f), Vec4(0.0f, 1.0f, 0.0f, 0.9f));
            }
        }
    }
    
    // Highlight selected vertex (cyan)
    if (g_state.selected_vertex_index >= 0) {
        const auto& scene = Game::g_state.scene;
        Vec2 vert_pos;
        bool found = false;
        
        if (g_state.selection_type == SelectionType::WALKABLE_AREA) {
            if (g_state.selected_polygon_index < (int)scene.geometry.walkable_areas.size()) {
                const auto& poly = scene.geometry.walkable_areas[g_state.selected_polygon_index];
                if (g_state.selected_vertex_index < (int)poly.points.size()) {
                    vert_pos = poly.points[g_state.selected_vertex_index];
                    found = true;
                }
            }
        } else if (g_state.selection_type == SelectionType::OBSTACLE) {
            if (g_state.selected_polygon_index < (int)scene.geometry.obstacles.size()) {
                const auto& poly = scene.geometry.obstacles[g_state.selected_polygon_index];
                if (g_state.selected_vertex_index < (int)poly.points.size()) {
                    vert_pos = poly.points[g_state.selected_vertex_index];
                    found = true;
                }
            }
        } else if (g_state.selection_type == SelectionType::HOTSPOT) {
            if (g_state.selected_polygon_index < (int)scene.geometry.hotspots.size()) {
                const auto& hotspot = scene.geometry.hotspots[g_state.selected_polygon_index];
                if (g_state.selected_vertex_index < (int)hotspot.bounds.points.size()) {
                    vert_pos = hotspot.bounds.points[g_state.selected_vertex_index];
                    found = true;
                }
            }
        }
        
        if (found) {
            Vec3 pos(vert_pos.x * scale_x, vert_pos.y * scale_y, ui_z);
            Renderer::render_rect(pos, Vec2(12.0f, 12.0f), Vec4(0.0f, 1.0f, 1.0f, 1.0f));
        }
    }
    
    // Draw target_position markers for hotspots (magenta cross)
    const auto& scene = Game::g_state.scene;
    for (size_t i = 0; i < scene.geometry.hotspots.size(); i++) {
        const auto& hotspot = scene.geometry.hotspots[i];
        if (hotspot.has_target_position) {
            Vec3 target_pos(hotspot.target_position.x * scale_x, 
                            hotspot.target_position.y * scale_y, ui_z);
            
            // Different color if this hotspot is selected
            Vec4 color = (g_state.selection_type == SelectionType::HOTSPOT && 
                          g_state.selected_polygon_index == (int)i)
                ? Vec4(1.0f, 0.0f, 1.0f, 1.0f)   // Bright magenta for selected
                : Vec4(1.0f, 0.5f, 1.0f, 0.7f);  // Faded magenta for others
            
            // Draw cross marker
            float size = 8.0f;
            Renderer::render_line(Vec3(target_pos.x - size, target_pos.y, ui_z),
                                  Vec3(target_pos.x + size, target_pos.y, ui_z), color, 2.0f);
            Renderer::render_line(Vec3(target_pos.x, target_pos.y - size, ui_z),
                                  Vec3(target_pos.x, target_pos.y + size, ui_z), color, 2.0f);
        }
    }
    
    // Draw mode indicator with black background
    char mode_text[128];
    if (g_state.selection_type == SelectionType::HOTSPOT && g_state.selected_polygon_index >= 0) {
        const auto& hotspot = scene.geometry.hotspots[g_state.selected_polygon_index];
        snprintf(mode_text, sizeof(mode_text), "[HOTSPOT: %s] Shift+Click to set target", 
                 hotspot.name.c_str());
    } else {
        snprintf(mode_text, sizeof(mode_text), "[%s]", get_mode_string());
    }
    
    // Black semi-transparent background for mode indicator
    float mode_y = Config::VIEWPORT_HEIGHT - 25.0f;
    Vec3 mode_bg_pos = Vec3(0.0f, mode_y - 5.0f, ui_z);
    Vec2 mode_bg_size = Vec2(Config::VIEWPORT_WIDTH, 28.0f);
    Vec4 mode_bg_color = Vec4(0.0f, 0.0f, 0.0f, 0.7f);
    Renderer::render_rect(mode_bg_pos, mode_bg_size, mode_bg_color);
    
    Renderer::render_text(mode_text, Vec2(10.0f, mode_y), 1.0f);
}

// Simple JSON writer (no external library needed)
bool save_geometry(const char* scene_name) {
    if (!scene_name || scene_name[0] == '\0') {
        DEBUG_ERROR("[GeoEditor] Cannot save: no scene name");
        return false;
    }
    
    const auto& scene = Game::g_state.scene;
    
    // Build absolute path to assets directory
    std::string root = get_project_root();
    std::string path = root + "assets/scenes/";
    path += scene_name;
    path += "/geometry.json";
    
    DEBUG_INFO("[GeoEditor] Saving to: %s", path.c_str());
    
    std::ofstream file(path);
    if (!file.is_open()) {
        DEBUG_ERROR("[GeoEditor] Failed to open %s for writing", path.c_str());
        return false;
    }
    
    file << "{\n";
    
    // Walkable areas
    file << "  \"walkable_areas\": [\n";
    for (size_t i = 0; i < scene.geometry.walkable_areas.size(); i++) {
        const auto& poly = scene.geometry.walkable_areas[i];
        file << "    {\n";
        file << "      \"name\": \"walkable_" << i << "\",\n";
        file << "      \"points\": [";
        for (size_t j = 0; j < poly.points.size(); j++) {
            file << "[" << poly.points[j].x << ", " << poly.points[j].y << "]";
            if (j + 1 < poly.points.size()) file << ", ";
        }
        file << "]\n";
        file << "    }";
        if (i + 1 < scene.geometry.walkable_areas.size()) file << ",";
        file << "\n";
    }
    file << "  ],\n";
    
    // Obstacles
    file << "  \"obstacles\": [\n";
    for (size_t i = 0; i < scene.geometry.obstacles.size(); i++) {
        const auto& poly = scene.geometry.obstacles[i];
        file << "    {\n";
        file << "      \"name\": \"obstacle_" << i << "\",\n";
        file << "      \"points\": [";
        for (size_t j = 0; j < poly.points.size(); j++) {
            file << "[" << poly.points[j].x << ", " << poly.points[j].y << "]";
            if (j + 1 < poly.points.size()) file << ", ";
        }
        file << "]\n";
        file << "    }";
        if (i + 1 < scene.geometry.obstacles.size()) file << ",";
        file << "\n";
    }
    file << "  ],\n";
    
    // Hotspots
    file << "  \"hotspots\": [\n";
    for (size_t i = 0; i < scene.geometry.hotspots.size(); i++) {
        const auto& hotspot = scene.geometry.hotspots[i];
        file << "    {\n";
        file << "      \"name\": \"" << hotspot.name << "\",\n";
        file << "      \"interaction_distance\": " << hotspot.interaction_distance << ",\n";
        if (hotspot.has_target_position) {
            file << "      \"target_position\": [" << hotspot.target_position.x << ", " << hotspot.target_position.y << "],\n";
        }
        file << "      \"points\": [";
        for (size_t j = 0; j < hotspot.bounds.points.size(); j++) {
            file << "[" << hotspot.bounds.points[j].x << ", " << hotspot.bounds.points[j].y << "]";
            if (j + 1 < hotspot.bounds.points.size()) file << ", ";
        }
        file << "]\n";
        file << "    }";
        if (i + 1 < scene.geometry.hotspots.size()) file << ",";
        file << "\n";
    }
    file << "  ]\n";
    
    file << "}\n";
    file.close();
    
    DEBUG_INFO("[GeoEditor] Saved geometry to %s", path.c_str());
    return true;
}

// Simple JSON parser (minimal, handles our format)
bool load_geometry(const char* scene_name) {
    if (!scene_name || scene_name[0] == '\0') {
        DEBUG_ERROR("[GeoEditor] Cannot load: no scene name");
        return false;
    }
    
    // Build absolute path to assets directory
    std::string root = get_project_root();
    std::string path = root + "assets/scenes/";
    path += scene_name;
    path += "/geometry.json";
    
    DEBUG_INFO("[GeoEditor] Loading from: %s", path.c_str());
    
    std::ifstream file(path);
    if (!file.is_open()) {
        DEBUG_LOG("[GeoEditor] No geometry file at %s (will create on save)", path.c_str());
        return false;
    }
    
    // Read entire file
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    auto& scene = Game::g_state.scene;
    
    // Store existing callbacks before clearing (so reload preserves them)
    std::unordered_map<std::string, std::function<void()>> saved_callbacks;
    for (const auto& hotspot : scene.geometry.hotspots) {
        if (hotspot.callback) {
            saved_callbacks[hotspot.name] = hotspot.callback;
        }
    }
    
    scene.geometry.walkable_areas.clear();
    scene.geometry.obstacles.clear();
    scene.geometry.hotspots.clear();
    
    // Parse walkable_areas section
    size_t walkable_start = content.find("\"walkable_areas\"");
    size_t obstacles_start = content.find("\"obstacles\"");
    size_t hotspots_start = content.find("\"hotspots\"");
    
    if (walkable_start != std::string::npos) {
        // Find array content - need to find matching bracket
        size_t arr_start = content.find('[', walkable_start);
        size_t arr_end = arr_start;
        int bracket_count = 1;
        for (size_t i = arr_start + 1; i < content.size() && bracket_count > 0; i++) {
            if (content[i] == '[') bracket_count++;
            else if (content[i] == ']') bracket_count--;
            if (bracket_count == 0) arr_end = i;
        }
        
        // Find each polygon object
        size_t pos = arr_start;
        while (pos < arr_end) {
            size_t obj_start = content.find('{', pos);
            if (obj_start >= arr_end || obj_start == std::string::npos) break;
            
            size_t obj_end = content.find('}', obj_start);
            if (obj_end >= content.size()) break;
            
            // Find points array within this object
            size_t points_key = content.find("\"points\"", obj_start);
            if (points_key < obj_end) {
                size_t pts_start = content.find('[', points_key);
                // Find matching closing bracket for points array
                size_t pts_end = pts_start;
                int pts_bracket_count = 1;
                for (size_t i = pts_start + 1; i < content.size() && pts_bracket_count > 0; i++) {
                    if (content[i] == '[') pts_bracket_count++;
                    else if (content[i] == ']') pts_bracket_count--;
                    if (pts_bracket_count == 0) pts_end = i;
                }
                
                Collision::Polygon poly;
                size_t p = pts_start + 1;
                while (p < pts_end) {
                    size_t inner_start = content.find('[', p);
                    if (inner_start >= pts_end || inner_start == std::string::npos) break;
                    
                    size_t inner_end = content.find(']', inner_start);
                    if (inner_end >= content.size()) break;
                    
                    // Parse x, y
                    std::string coords = content.substr(inner_start + 1, inner_end - inner_start - 1);
                    float x = 0, y = 0;
                    if (sscanf(coords.c_str(), "%f, %f", &x, &y) == 2 ||
                        sscanf(coords.c_str(), "%f,%f", &x, &y) == 2) {
                        poly.points.push_back(Vec2(x, y));
                    }
                    
                    p = inner_end + 1;
                }
                
                if (poly.is_valid()) {
                    if (!Collision::is_polygon_convex(poly)) {
                        DEBUG_ERROR("[GeoEditor] WARNING: Loaded walkable area #%zu is NOT CONVEX!", 
                                   scene.geometry.walkable_areas.size());
                    }
                    scene.geometry.walkable_areas.push_back(poly);
                }
            }
            
            pos = obj_end + 1;
        }
    }
    
    // Parse obstacles section
    if (obstacles_start != std::string::npos) {
        size_t arr_start = content.find('[', obstacles_start);
        size_t arr_end = arr_start;
        int bracket_count = 1;
        for (size_t i = arr_start + 1; i < content.size() && bracket_count > 0; i++) {
            if (content[i] == '[') bracket_count++;
            else if (content[i] == ']') bracket_count--;
            if (bracket_count == 0) arr_end = i;
        }
        
        size_t pos = arr_start;
        while (pos < arr_end) {
            size_t obj_start = content.find('{', pos);
            if (obj_start >= arr_end || obj_start == std::string::npos) break;
            
            size_t obj_end = content.find('}', obj_start);
            if (obj_end >= content.size()) break;
            
            // Find points array within this object
            size_t points_key = content.find("\"points\"", obj_start);
            if (points_key < obj_end) {
                size_t pts_start = content.find('[', points_key);
                size_t pts_end = pts_start;
                int pts_bracket_count = 1;
                for (size_t i = pts_start + 1; i < content.size() && pts_bracket_count > 0; i++) {
                    if (content[i] == '[') pts_bracket_count++;
                    else if (content[i] == ']') pts_bracket_count--;
                    if (pts_bracket_count == 0) pts_end = i;
                }
                
                Collision::Polygon poly;
                size_t p = pts_start + 1;
                while (p < pts_end) {
                    size_t inner_start = content.find('[', p);
                    if (inner_start >= pts_end || inner_start == std::string::npos) break;
                    
                    size_t inner_end = content.find(']', inner_start);
                    if (inner_end >= content.size()) break;
                    
                    std::string coords = content.substr(inner_start + 1, inner_end - inner_start - 1);
                    float x = 0, y = 0;
                    if (sscanf(coords.c_str(), "%f, %f", &x, &y) == 2 ||
                        sscanf(coords.c_str(), "%f,%f", &x, &y) == 2) {
                        poly.points.push_back(Vec2(x, y));
                    }
                    
                    p = inner_end + 1;
                }
                
                if (poly.is_valid()) {
                    if (!Collision::is_polygon_convex(poly)) {
                        DEBUG_ERROR("[GeoEditor] WARNING: Loaded obstacle #%zu is NOT CONVEX!", 
                                   scene.geometry.obstacles.size());
                    }
                    scene.geometry.obstacles.push_back(poly);
                }
            }
            
            pos = obj_end + 1;
        }
    }
    
    // Parse hotspots section (similar logic)
    if (hotspots_start != std::string::npos) {
        size_t arr_start = content.find('[', hotspots_start);
        size_t final_end = content.rfind(']');
        
        size_t pos = arr_start;
        while (pos < final_end) {
            size_t obj_start = content.find('{', pos);
            if (obj_start >= final_end || obj_start == std::string::npos) break;
            
            size_t obj_end = content.find('}', obj_start);
            if (obj_end >= content.size()) break;
            
            Scene::Hotspot hotspot;
            hotspot.enabled = true;
            
            // Parse name
            size_t name_key = content.find("\"name\"", obj_start);
            if (name_key < obj_end) {
                size_t name_start = content.find('\"', name_key + 6);
                size_t name_end = content.find('\"', name_start + 1);
                hotspot.name = content.substr(name_start + 1, name_end - name_start - 1);
            }
            
            // Parse interaction_distance
            size_t dist_key = content.find("\"interaction_distance\"", obj_start);
            if (dist_key < obj_end) {
                size_t colon = content.find(':', dist_key);
                float dist = 0;
                sscanf(content.c_str() + colon + 1, "%f", &dist);
                hotspot.interaction_distance = dist;
            }
            
            // Parse target_position (optional)
            size_t target_key = content.find("\"target_position\"", obj_start);
            if (target_key < obj_end && target_key != std::string::npos) {
                size_t arr_start = content.find('[', target_key);
                size_t arr_end = content.find(']', arr_start);
                if (arr_start < obj_end && arr_end < obj_end) {
                    std::string coords = content.substr(arr_start + 1, arr_end - arr_start - 1);
                    float x = 0, y = 0;
                    if (sscanf(coords.c_str(), "%f, %f", &x, &y) == 2 ||
                        sscanf(coords.c_str(), "%f,%f", &x, &y) == 2) {
                        hotspot.target_position = Vec2(x, y);
                        hotspot.has_target_position = true;
                    }
                }
            }
            
            // Parse points
            size_t points_key = content.find("\"points\"", obj_start);
            if (points_key < obj_end) {
                size_t pts_start = content.find('[', points_key);
                size_t pts_end = pts_start;
                int bracket_count = 1;
                for (size_t i = pts_start + 1; i < content.size() && bracket_count > 0; i++) {
                    if (content[i] == '[') bracket_count++;
                    else if (content[i] == ']') bracket_count--;
                    if (bracket_count == 0) pts_end = i;
                }
                
                size_t p = pts_start + 1;
                while (p < pts_end) {
                    size_t inner_start = content.find('[', p);
                    if (inner_start >= pts_end || inner_start == std::string::npos) break;
                    
                    size_t inner_end = content.find(']', inner_start);
                    
                    std::string coords = content.substr(inner_start + 1, inner_end - inner_start - 1);
                    float x = 0, y = 0;
                    if (sscanf(coords.c_str(), "%f, %f", &x, &y) == 2 ||
                        sscanf(coords.c_str(), "%f,%f", &x, &y) == 2) {
                        hotspot.bounds.points.push_back(Vec2(x, y));
                    }
                    
                    p = inner_end + 1;
                }
            }
            
            if (hotspot.bounds.is_valid()) {
                scene.geometry.hotspots.push_back(hotspot);
            }
            
            pos = obj_end + 1;
        }
    }
    
    // Restore saved callbacks by matching hotspot names
    for (auto& hotspot : scene.geometry.hotspots) {
        auto it = saved_callbacks.find(hotspot.name);
        if (it != saved_callbacks.end()) {
            hotspot.callback = it->second;
            DEBUG_LOG("[GeoEditor] Restored callback for hotspot '%s'", hotspot.name.c_str());
        }
    }
    
    DEBUG_INFO("[GeoEditor] Loaded geometry from %s: %zu walkable areas, %zu obstacles, %zu hotspots",
           path.c_str(), scene.geometry.walkable_areas.size(), 
           scene.geometry.obstacles.size(), scene.geometry.hotspots.size());
    
    return true;
}

}
