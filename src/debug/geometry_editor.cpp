#include "geometry_editor.h"
#include "debug_log.h"
#include "renderer/renderer.h"
#include "renderer/text.h"
#include "game/game.h"
#include "platform.h"
#include "config.h"
#include "ecs/entity_factory.h"
#include <cstdio>
#include <cmath>
#include <cfloat>
#include <fstream>
#include <sstream>
#include <unordered_map>
#ifndef _WIN32
#include <unistd.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

// For finding executable path on macOS
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace GeometryEditor {

static EditorState g_state;
static Vec2 g_mouse_pos;
static int g_frame_counter = 0;  // For double-click detection

// macOS virtual key codes
#ifdef __APPLE__
    static constexpr int KEY_W = 13;
    static constexpr int KEY_H = 4;
    static constexpr int KEY_O = 31;
    static constexpr int KEY_F = 3;
    static constexpr int KEY_R = 15;
    static constexpr int KEY_E = 14;
    static constexpr int KEY_ESC = 53;
    static constexpr int KEY_DELETE = 51;
    static constexpr int KEY_MINUS = 27;      // - key
    static constexpr int KEY_EQUALS = 24;     // = key (+ with shift)
#else
    static constexpr int KEY_W = 0x57;
    static constexpr int KEY_H = 0x48;
    static constexpr int KEY_O = 0x4F;
    static constexpr int KEY_F = 0x46;
    static constexpr int KEY_R = 0x52;
    static constexpr int KEY_E = 0x45;
    static constexpr int KEY_ESC = 0x1B;
    static constexpr int KEY_DELETE = 0x2E;
    static constexpr int KEY_MINUS = 0xBD;
    static constexpr int KEY_EQUALS = 0xBB;
#endif

// Previous key states for edge detection
static bool s_prev_w = false;
static bool s_prev_h = false;
static bool s_prev_o = false;
static bool s_prev_f = false;
static bool s_prev_r = false;
static bool s_prev_e = false;
static bool s_prev_esc = false;
static bool s_prev_del = false;
static bool s_prev_minus = false;
static bool s_prev_equals = false;

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
#elif defined(_WIN32)
    // Windows: Get executable path and find project root
    char path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, path, MAX_PATH)) {
        std::string exe_path(path);
        // Normalize backslashes to forward slashes
        for (auto& c : exe_path) {
            if (c == '\\') c = '/';
        }
        // Find "build" in path and go to parent
        // e.g. C:/Users/user/project/build/Debug/game.exe -> C:/Users/user/project/
        size_t pos = exe_path.find("/build/");
        if (pos != std::string::npos) {
            return exe_path.substr(0, pos) + "/";
        }
    }
    // Fallback: relative path (less reliable)
    DEBUG_LOG("[GeoEditor] Warning: Could not determine project root from exe path");
    return "../../";
#else
    // Linux or other: use relative path
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
        // Reset entity selection
        g_state.entity_selection_type = EntitySelectionType::NONE;
        g_state.selected_entity = ECS::INVALID_ENTITY;
        g_state.selected_entity_index = -1;
    }
}

const char* get_mode_string() {
    switch (g_state.mode) {
        case EditorMode::CREATING_WALKABLE: return "CREATING WALKABLE";
        case EditorMode::CREATING_HOTSPOT: return "CREATING HOTSPOT";
        case EditorMode::CREATING_OBSTACLE: return "CREATING OBSTACLE";
        case EditorMode::SELECT_ENTITY: return "ENTITY MODE";
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

// ============================================================================
// ENTITY EDITOR HELPERS
// ============================================================================

static constexpr float ENTITY_SELECT_RADIUS = 20.0f;  // Pixels for entity selection

// Find entity (prop, light, player) near position
static bool find_entity_at(Vec2 pos, EntitySelectionType& out_type, int& out_index, ECS::EntityID& out_entity) {
    auto& ecs = Game::g_state.ecs_world;
    auto& scene = Game::g_state.scene;
    
    float best_dist_sq = ENTITY_SELECT_RADIUS * ENTITY_SELECT_RADIUS;
    out_type = EntitySelectionType::NONE;
    out_index = -1;
    out_entity = ECS::INVALID_ENTITY;
    
    // Check player first (priority)
    if (Game::g_state.player_entity != ECS::INVALID_ENTITY) {
        auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(Game::g_state.player_entity);
        if (transform) {
            float dx = pos.x - transform->position.x;
            float dy = pos.y - transform->position.y;
            float dist_sq = dx*dx + dy*dy;
            if (dist_sq < best_dist_sq) {
                best_dist_sq = dist_sq;
                out_type = EntitySelectionType::PLAYER;
                out_index = 0;
                out_entity = Game::g_state.player_entity;
            }
        }
    }
    
    // Check props
    for (size_t i = 0; i < scene.prop_entities.size(); i++) {
        auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(scene.prop_entities[i]);
        if (!transform) continue;
        
        float dx = pos.x - transform->position.x;
        float dy = pos.y - transform->position.y;
        float dist_sq = dx*dx + dy*dy;
        if (dist_sq < best_dist_sq) {
            best_dist_sq = dist_sq;
            out_type = EntitySelectionType::PROP;
            out_index = (int)i;
            out_entity = scene.prop_entities[i];
        }
    }
    
    // Check point lights (need to convert OpenGL to pixel coords for comparison)
    for (size_t i = 0; i < scene.light_entities.size(); i++) {
        auto* transform = ecs.get_component<ECS::Transform3DComponent>(scene.light_entities[i]);
        if (!transform) continue;
        
        // Convert OpenGL coords to pixel coords for comparison
        float pixel_x = (transform->position.x + 1.0f) * 0.5f * Config::BASE_WIDTH;
        float pixel_y = (1.0f - transform->position.y) * 0.5f * Config::BASE_HEIGHT;
        
        float dx = pos.x - pixel_x;
        float dy = pos.y - pixel_y;
        float dist_sq = dx*dx + dy*dy;
        if (dist_sq < best_dist_sq) {
            best_dist_sq = dist_sq;
            out_type = EntitySelectionType::POINT_LIGHT;
            out_index = (int)i;
            out_entity = scene.light_entities[i];
        }
    }
    
    // Check projector lights
    for (size_t i = 0; i < scene.projector_light_entities.size(); i++) {
        auto* transform = ecs.get_component<ECS::Transform3DComponent>(scene.projector_light_entities[i]);
        if (!transform) continue;
        
        float pixel_x = (transform->position.x + 1.0f) * 0.5f * Config::BASE_WIDTH;
        float pixel_y = (1.0f - transform->position.y) * 0.5f * Config::BASE_HEIGHT;
        
        float dx = pos.x - pixel_x;
        float dy = pos.y - pixel_y;
        float dist_sq = dx*dx + dy*dy;
        if (dist_sq < best_dist_sq) {
            best_dist_sq = dist_sq;
            out_type = EntitySelectionType::PROJECTOR_LIGHT;
            out_index = (int)i;
            out_entity = scene.projector_light_entities[i];
        }
    }
    
    return out_type != EntitySelectionType::NONE;
}

// Adjust Z-depth of selected light (only for lights, objects get Z from depth map)
static void adjust_entity_z(float delta) {
    if (g_state.selected_entity == ECS::INVALID_ENTITY) return;
    
    auto& ecs = Game::g_state.ecs_world;
    
    // 2.5D Objects: adjust elevation (height above ground)
    if (g_state.entity_selection_type == EntitySelectionType::PROP ||
        g_state.entity_selection_type == EntitySelectionType::PLAYER) {
        auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(g_state.selected_entity);
        if (transform) {
            transform->elevation += delta;
            transform->elevation = fmaxf(-0.5f, fminf(1.0f, transform->elevation));  // Clamp to reasonable range
            DEBUG_INFO("[GeoEditor] Object elevation: %.3f", transform->elevation);
        }
    }
    // 3D Lights: adjust Z position in render space
    else if (g_state.entity_selection_type == EntitySelectionType::POINT_LIGHT ||
             g_state.entity_selection_type == EntitySelectionType::PROJECTOR_LIGHT) {
        auto* transform = ecs.get_component<ECS::Transform3DComponent>(g_state.selected_entity);
        if (transform) {
            transform->position.z += delta;
            transform->position.z = fmaxf(-1.0f, fminf(1.0f, transform->position.z));
            DEBUG_INFO("[GeoEditor] Light Z-position: %.3f", transform->position.z);
        }
    }
}

// Adjust entity Z by mouse drag delta (for Shift+Drag on lights/objects)
static void adjust_entity_z_by_mouse_delta(float delta_pixels) {
    if (g_state.selected_entity == ECS::INVALID_ENTITY) return;
    
    auto& ecs = Game::g_state.ecs_world;
    
    // 2.5D Objects: adjust elevation via drag
    if (g_state.entity_selection_type == EntitySelectionType::PROP ||
        g_state.entity_selection_type == EntitySelectionType::PLAYER) {
        float elev_delta = -delta_pixels / 500.0f;  // 500 pixels = 1.0 elevation
        auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(g_state.selected_entity);
        if (transform) {
            transform->elevation += elev_delta;
            transform->elevation = fmaxf(-0.5f, fminf(1.0f, transform->elevation));
        }
    }
    // 3D Lights: adjust Z-position via drag
    else if (g_state.entity_selection_type == EntitySelectionType::POINT_LIGHT ||
             g_state.entity_selection_type == EntitySelectionType::PROJECTOR_LIGHT) {
        float z_delta = -delta_pixels / 1000.0f;  // 1000 pixels = 1.0 Z-unit
        auto* transform = ecs.get_component<ECS::Transform3DComponent>(g_state.selected_entity);
        if (transform) {
            transform->position.z += z_delta;
            transform->position.z = fmaxf(-1.0f, fminf(1.0f, transform->position.z));
        }
    }
}

// Move entity position (pixel coords for props/player, converts to OpenGL for lights)
// For 2.5D objects: Store only X,Y, Z is calculated from depth map during rendering
static void move_entity_to(Vec2 new_pos) {
    if (g_state.selected_entity == ECS::INVALID_ENTITY) return;
    
    auto& ecs = Game::g_state.ecs_world;
    
    if (g_state.entity_selection_type == EntitySelectionType::PROP ||
        g_state.entity_selection_type == EntitySelectionType::PLAYER) {
        auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(g_state.selected_entity);
        if (transform) {
            transform->position.x = new_pos.x;
            transform->position.y = new_pos.y;
            // Z is calculated from depth map during rendering
        }
    }
    else if (g_state.entity_selection_type == EntitySelectionType::POINT_LIGHT ||
             g_state.entity_selection_type == EntitySelectionType::PROJECTOR_LIGHT) {
        auto* transform = ecs.get_component<ECS::Transform3DComponent>(g_state.selected_entity);
        if (transform) {
            // Convert pixel to OpenGL coords (only X/Y, Z stays unchanged)
            transform->position.x = (new_pos.x / Config::BASE_WIDTH) * 2.0f - 1.0f;
            transform->position.y = 1.0f - (new_pos.y / Config::BASE_HEIGHT) * 2.0f;
        }
    }
}

// Get entity name for display
static const char* get_entity_type_name(EntitySelectionType type) {
    switch (type) {
        case EntitySelectionType::PROP: return "PROP";
        case EntitySelectionType::POINT_LIGHT: return "POINT LIGHT";
        case EntitySelectionType::PROJECTOR_LIGHT: return "PROJECTOR LIGHT";
        case EntitySelectionType::PLAYER: return "PLAYER";
        default: return "NONE";
    }
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
    g_frame_counter++;  // Increment frame counter for double-click detection
    
    if (!g_state.is_active) return;
    
    g_mouse_pos = mouse_base_coords;
    
    // Edge-detect key presses
    bool key_w = Platform::key_pressed(KEY_W);
    bool key_h = Platform::key_pressed(KEY_H);
    bool key_o = Platform::key_pressed(KEY_O);
    bool key_f = Platform::key_pressed(KEY_F);
    bool key_r = Platform::key_pressed(KEY_R);
    bool key_e = Platform::key_pressed(KEY_E);
    bool key_esc = Platform::key_pressed(KEY_ESC);
    bool key_del = Platform::key_pressed(KEY_DELETE);
    bool key_minus = Platform::key_pressed(KEY_MINUS);
    bool key_equals = Platform::key_pressed(KEY_EQUALS);
    
    // Get scroll delta for Z-axis adjustment
    float scroll_delta = Platform::scroll_delta();
    
    // R - Reload geometry from JSON (preserves callbacks)
    if (key_r && !s_prev_r && g_state.mode == EditorMode::NONE) {
        load_geometry(Game::g_state.scene.name.c_str());
    }
    
    // E - Toggle Entity mode
    if (key_e && !s_prev_e) {
        if (g_state.mode == EditorMode::SELECT_ENTITY) {
            // Exit entity mode
            g_state.mode = EditorMode::NONE;
            g_state.entity_selection_type = EntitySelectionType::NONE;
            g_state.selected_entity = ECS::INVALID_ENTITY;
            g_state.selected_entity_index = -1;
            DEBUG_INFO("[GeoEditor] Exited entity mode");
        } else if (g_state.mode == EditorMode::NONE) {
            // Enter entity mode
            g_state.mode = EditorMode::SELECT_ENTITY;
            // Clear polygon selection
            g_state.selected_polygon_index = -1;
            g_state.selected_vertex_index = -1;
            g_state.selection_type = SelectionType::NONE;
            DEBUG_INFO("[GeoEditor] Entered entity mode (E=exit, click=select, drag=move, scroll/+/-=Z)");
        }
    }
    
    // Z-axis adjustment for entities (elevation for objects, Z for lights)
    if (g_state.mode == EditorMode::SELECT_ENTITY && 
        g_state.selected_entity != ECS::INVALID_ENTITY) {
        
        float z_change = 0.0f;
        
        // Scroll wheel (trackpad or mouse)
        if (fabsf(scroll_delta) > 0.1f) {
            if (g_state.entity_selection_type == EntitySelectionType::POINT_LIGHT ||
                g_state.entity_selection_type == EntitySelectionType::PROJECTOR_LIGHT) {
                z_change = scroll_delta * 0.01f;  // Smaller step for lights
            } else {
                z_change = scroll_delta * 0.005f;  // Smaller step for objects elevation
            }
        }
        
        // + key - move toward camera (Z/elevation decreases)
        if (key_equals && !s_prev_equals) {
            z_change = -0.01f;
        }
        
        // - key - move away from camera (Z/elevation increases)
        if (key_minus && !s_prev_minus) {
            z_change = 0.01f;
        }
        
        if (z_change != 0.0f) {
            adjust_entity_z(z_change);
            save_entities(Game::g_state.scene.name.c_str());
        }
    }
    
    // W - Start creating walkable area (only if not in entity mode)
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
    
    // Update drag (polygon vertices)
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
    
    // Update drag (entities in entity mode)
    if (g_state.dragging && g_state.mode == EditorMode::SELECT_ENTITY && 
        g_state.selected_entity != ECS::INVALID_ENTITY) {
        
        // Check if Shift is held (for elevation/Z movement)
        if (Platform::shift_down()) {
            // Shift+Drag: adjust elevation/Z based on mouse Y movement
            float delta_y = mouse_base_coords.y - g_state.drag_start.y;
            adjust_entity_z_by_mouse_delta(delta_y);
            // Don't change X/Y position
        } else {
            // Normal drag: move X/Y
            move_entity_to(mouse_base_coords);
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
    s_prev_e = key_e;
    s_prev_esc = key_esc;
    s_prev_del = key_del;
    s_prev_minus = key_minus;
    s_prev_equals = key_equals;
}

void on_mouse_click(Vec2 pos) {
    if (!g_state.is_active) return;
    
    // =========================================================================
    // ENTITY MODE: Select and drag entities
    // =========================================================================
    if (g_state.mode == EditorMode::SELECT_ENTITY) {
        // Cmd/Ctrl+Click on selected projector light: set direction to clicked point
        if (Platform::control_down() && 
            g_state.entity_selection_type == EntitySelectionType::PROJECTOR_LIGHT &&
            g_state.selected_entity != ECS::INVALID_ENTITY) {
            
            auto& ecs = Game::g_state.ecs_world;
            auto& scene = Game::g_state.scene;
            auto* projector = ecs.get_component<ECS::ProjectorLightComponent>(g_state.selected_entity);
            auto* transform = ecs.get_component<ECS::Transform3DComponent>(g_state.selected_entity);
            
            if (projector && transform && scene.depth_map.is_valid()) {
                // Get target Z from depth map at clicked position
                float target_z = ECS::TransformHelpers::get_z_from_depth_map(
                    scene.depth_map, pos.x, pos.y, scene.width, scene.height);
                
                // Convert click position to OpenGL coords for target
                Vec2 target_gl = Coords::pixel_to_opengl(pos, Config::BASE_WIDTH, Config::BASE_HEIGHT);
                
                // Calculate direction from position to clicked target
                Vec3 target(target_gl.x, target_gl.y, target_z);
                Vec3 dir = Vec3(
                    target.x - transform->position.x,
                    target.y - transform->position.y,
                    target.z - transform->position.z
                );
                float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
                if (len > 0.001f) {
                    projector->direction = Vec3(dir.x/len, dir.y/len, dir.z/len);
                    projector->target_distance = len;  // Store actual distance for visualization
                }
                
                DEBUG_INFO("[GeoEditor] Set projector direction at (%.1f, %.1f) → dir (%.2f, %.2f, %.2f)", 
                       pos.x, pos.y, projector->direction.x, projector->direction.y, projector->direction.z);
                save_entities(scene.name.c_str());
                return;
            }
        }
        
        // Check for double-click (same entity, same position, within ~18 frames = 0.3 seconds @ 60fps)
        const int DOUBLE_CLICK_FRAME_THRESHOLD = 18;
        const float DOUBLE_CLICK_DISTANCE = 15.0f;
        
        // Find what's at this click position
        EntitySelectionType type;
        int index;
        ECS::EntityID entity;
        bool found_entity = find_entity_at(pos, type, index, entity);
        
        // Check if this is a double-click on the same entity
        if (found_entity && entity == g_state.selected_entity &&
            (g_frame_counter - g_state.last_click_frame) < DOUBLE_CLICK_FRAME_THRESHOLD) {
            float dx = pos.x - g_state.last_click_pos.x;
            float dy = pos.y - g_state.last_click_pos.y;
            float dist_sq = dx*dx + dy*dy;
            if (dist_sq < DOUBLE_CLICK_DISTANCE * DOUBLE_CLICK_DISTANCE) {
                // Double-click detected! Reset elevation/Z to 0
                auto& ecs = Game::g_state.ecs_world;
                if (type == EntitySelectionType::PROP || type == EntitySelectionType::PLAYER) {
                    auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(entity);
                    if (transform) {
                        transform->elevation = 0.0f;
                        DEBUG_INFO("[GeoEditor] Double-click: Reset elevation to 0.0f");
                    }
                } else if (type == EntitySelectionType::POINT_LIGHT || type == EntitySelectionType::PROJECTOR_LIGHT) {
                    auto* transform = ecs.get_component<ECS::Transform3DComponent>(entity);
                    if (transform) {
                        transform->position.z = 0.0f;
                        DEBUG_INFO("[GeoEditor] Double-click: Reset Z to 0.0f");
                    }
                }
                // Don't drag after double-click
                g_state.last_click_frame = -1000;  // Reset timing
                return;
            }
        }
        
        // Update last click info for next potential double-click
        g_state.last_click_frame = g_frame_counter;
        g_state.last_click_pos = pos;
        
        // If we already have an entity selected and click on it, start dragging
        if (g_state.selected_entity != ECS::INVALID_ENTITY) {
            if (find_entity_at(pos, type, index, entity) && entity == g_state.selected_entity) {
                g_state.dragging = true;
                g_state.drag_start = pos;
                DEBUG_INFO("[GeoEditor] Started dragging %s", get_entity_type_name(type));
                return;
            }
        }
        
        // Try to select an entity
        if (find_entity_at(pos, type, index, entity)) {
            g_state.entity_selection_type = type;
            g_state.selected_entity_index = index;
            g_state.selected_entity = entity;
            g_state.dragging = true;
            g_state.drag_start = pos;
            
            // Get Z value for display
            float z_value = 0.0f;
            auto& ecs = Game::g_state.ecs_world;
            if (type == EntitySelectionType::PROP || type == EntitySelectionType::PLAYER) {
                auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(entity);
                if (transform) z_value = transform->elevation;
            } else {
                auto* transform = ecs.get_component<ECS::Transform3DComponent>(entity);
                if (transform) z_value = transform->position.z;
            }
            
            DEBUG_INFO("[GeoEditor] Selected %s #%d (Z=%.2f) - drag to move, scroll/+/-=Z", 
                   get_entity_type_name(type), index, z_value);
            return;
        }
        
        // Deselect entity
        g_state.entity_selection_type = EntitySelectionType::NONE;
        g_state.selected_entity = ECS::INVALID_ENTITY;
        g_state.selected_entity_index = -1;
        g_state.last_click_frame = -1000;  // Reset timing
        return;
    }
    
    // =========================================================================
    // POLYGON MODE (original code)
    // =========================================================================
    
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
        if (g_state.mode == EditorMode::SELECT_ENTITY) {
            // Save entities when dragging entities
            save_entities(Game::g_state.scene.name.c_str());
        } else {
            // Save geometry when dragging polygons
            save_geometry(Game::g_state.scene.name.c_str());
        }
    }
}

void render() {
    if (!g_state.is_active) return;
    
    uint32_t window_w = Platform::get_window_width();
    uint32_t window_h = Platform::get_window_height();
    float scale_x = (float)window_w / (float)Config::BASE_WIDTH;
    float scale_y = (float)window_h / (float)Config::BASE_HEIGHT;
    float ui_z = ZDepth::GAME_HUD;
    
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
    
    // =========================================================================
    // ENTITY MODE: Draw entity markers
    // =========================================================================
    if (g_state.mode == EditorMode::SELECT_ENTITY) {
        auto& ecs = Game::g_state.ecs_world;
        
        // Draw prop markers (green boxes)
        for (size_t i = 0; i < scene.prop_entities.size(); i++) {
            auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(scene.prop_entities[i]);
            if (!transform) continue;
            
            Vec3 pos(transform->position.x * scale_x, transform->position.y * scale_y, ui_z);
            bool is_selected = (g_state.entity_selection_type == EntitySelectionType::PROP && 
                               g_state.selected_entity_index == (int)i);
            
            Vec4 color = is_selected ? Vec4(0.0f, 1.0f, 0.0f, 1.0f) : Vec4(0.0f, 0.7f, 0.0f, 0.6f);
            float size = is_selected ? 16.0f : 12.0f;
            Renderer::render_rect(pos, Vec2(size, size), color);
        }
        
        // Draw point light markers (yellow circles/diamonds)
        for (size_t i = 0; i < scene.light_entities.size(); i++) {
            auto* transform = ecs.get_component<ECS::Transform3DComponent>(scene.light_entities[i]);
            if (!transform) continue;
            
            float pixel_x = (transform->position.x + 1.0f) * 0.5f * Config::BASE_WIDTH;
            float pixel_y = (1.0f - transform->position.y) * 0.5f * Config::BASE_HEIGHT;
            Vec3 pos(pixel_x * scale_x, pixel_y * scale_y, ui_z);
            
            bool is_selected = (g_state.entity_selection_type == EntitySelectionType::POINT_LIGHT && 
                               g_state.selected_entity_index == (int)i);
            
            Vec4 color = is_selected ? Vec4(1.0f, 1.0f, 0.0f, 1.0f) : Vec4(0.8f, 0.8f, 0.0f, 0.6f);
            float size = is_selected ? 16.0f : 12.0f;
            
            // Draw diamond shape for lights
            Renderer::render_line(Vec3(pos.x - size, pos.y, ui_z), Vec3(pos.x, pos.y - size, ui_z), color, 2.0f);
            Renderer::render_line(Vec3(pos.x, pos.y - size, ui_z), Vec3(pos.x + size, pos.y, ui_z), color, 2.0f);
            Renderer::render_line(Vec3(pos.x + size, pos.y, ui_z), Vec3(pos.x, pos.y + size, ui_z), color, 2.0f);
            Renderer::render_line(Vec3(pos.x, pos.y + size, ui_z), Vec3(pos.x - size, pos.y, ui_z), color, 2.0f);
            
            // For selected light, show line to floor (2.5D projection)
            if (is_selected && scene.depth_map.is_valid()) {
                // Find floor Y: search DOWNWARD from light, find first Y where Z >= light.Z
                float floor_y = ECS::TransformHelpers::find_floor_y_below(
                    scene.depth_map, pixel_x, pixel_y, transform->position.z,
                    scene.width, scene.height);
                
                // Draw line if floor found and there's meaningful distance
                if (floor_y >= 0.0f && floor_y > pixel_y + 1.0f) {
                    // Draw vertical line from light DOWN to floor position
                    Vec4 line_color(0.5f, 1.0f, 1.0f, 1.0f);  // Bright cyan
                    Vec3 floor_pos(pos.x, floor_y * scale_y, ui_z);  // Floor in scaled pixel coords
                    Renderer::render_line(pos, floor_pos, line_color, 1.0f);
                    
                    // Subtle ring around light marker
                    Vec4 ring_color(0.5f, 1.0f, 1.0f, 0.3f);  // Faded cyan
                    float ring_size = size + 5.0f;
                    const int segments = 12;
                    for (int s = 0; s < segments; s++) {
                        float a1 = (float)s / segments * 6.28318f;
                        float a2 = (float)(s + 1) / segments * 6.28318f;
                        Renderer::render_line(
                            Vec3(pos.x + cosf(a1) * ring_size, pos.y + sinf(a1) * ring_size, ui_z),
                            Vec3(pos.x + cosf(a2) * ring_size, pos.y + sinf(a2) * ring_size, ui_z),
                            ring_color, 1.0f);
                    }
                }
            }
        }
        
        // Draw projector light markers (orange triangles)
        for (size_t i = 0; i < scene.projector_light_entities.size(); i++) {
            auto* transform = ecs.get_component<ECS::Transform3DComponent>(scene.projector_light_entities[i]);
            if (!transform) continue;
            
            float pixel_x = (transform->position.x + 1.0f) * 0.5f * Config::BASE_WIDTH;
            float pixel_y = (1.0f - transform->position.y) * 0.5f * Config::BASE_HEIGHT;
            Vec3 pos(pixel_x * scale_x, pixel_y * scale_y, ui_z);
            
            bool is_selected = (g_state.entity_selection_type == EntitySelectionType::PROJECTOR_LIGHT && 
                               g_state.selected_entity_index == (int)i);
            
            Vec4 color = is_selected ? Vec4(1.0f, 0.5f, 0.0f, 1.0f) : Vec4(0.8f, 0.4f, 0.0f, 0.6f);
            float size = is_selected ? 16.0f : 12.0f;
            
            // Draw triangle for projector (pointing in direction)
            Renderer::render_line(Vec3(pos.x, pos.y - size, ui_z), Vec3(pos.x - size, pos.y + size, ui_z), color, 2.0f);
            Renderer::render_line(Vec3(pos.x - size, pos.y + size, ui_z), Vec3(pos.x + size, pos.y + size, ui_z), color, 2.0f);
            Renderer::render_line(Vec3(pos.x + size, pos.y + size, ui_z), Vec3(pos.x, pos.y - size, ui_z), color, 2.0f);
            
            // For selected projector, show line to floor (2.5D projection)
            if (is_selected && scene.depth_map.is_valid()) {
                // Find floor Y: search DOWNWARD from projector, find first Y where Z >= projector.Z
                float floor_y = ECS::TransformHelpers::find_floor_y_below(
                    scene.depth_map, pixel_x, pixel_y, transform->position.z,
                    scene.width, scene.height);
                
                // Draw line if floor found and there's meaningful distance
                if (floor_y >= 0.0f && floor_y > pixel_y + 1.0f) {
                    // Draw vertical line from projector DOWN to floor position
                    Vec4 line_color(1.0f, 0.7f, 0.5f, 1.0f);  // Bright orange
                    Vec3 floor_pos(pos.x, floor_y * scale_y, ui_z);  // Floor in scaled pixel coords
                    Renderer::render_line(pos, floor_pos, line_color, 1.0f);
                    
                    // Subtle ring around projector marker
                    Vec4 ring_color(1.0f, 0.7f, 0.5f, 0.3f);  // Faded light orange
                    float ring_size = size + 5.0f;
                    const int segments = 12;
                    for (int s = 0; s < segments; s++) {
                        float a1 = (float)s / segments * 6.28318f;
                        float a2 = (float)(s + 1) / segments * 6.28318f;
                        Renderer::render_line(
                            Vec3(pos.x + cosf(a1) * ring_size, pos.y + sinf(a1) * ring_size, ui_z),
                            Vec3(pos.x + cosf(a2) * ring_size, pos.y + sinf(a2) * ring_size, ui_z),
                            ring_color, 1.0f);
                    }
                }
            }
            
            // Draw direction line (always show direction)
            auto* projector = ecs.get_component<ECS::ProjectorLightComponent>(scene.projector_light_entities[i]);
            if (projector) {
                // Calculate endpoint in OpenGL 3D space: position + direction * target_distance
                Vec2 target_gl(
                    transform->position.x + projector->direction.x * projector->target_distance,
                    transform->position.y + projector->direction.y * projector->target_distance
                );
                
                // Convert OpenGL coords back to pixel coords
                // Note: opengl_to_pixel has wrong Y formula, so we do inverse of pixel_to_opengl manually:
                // gl_y = 1 - 2*pixel_y/height  =>  pixel_y = (1 - gl_y) * height / 2
                float target_pixel_x = (target_gl.x + 1.0f) * 0.5f * Config::BASE_WIDTH;
                float target_pixel_y = (1.0f - target_gl.y) * 0.5f * Config::BASE_HEIGHT;
                
                Vec3 target_pos(target_pixel_x * scale_x, target_pixel_y * scale_y, ui_z);
                
                // Draw line from projector to direction endpoint
                Vec4 dir_color = is_selected ? Vec4(1.0f, 1.0f, 0.0f, 1.0f) : Vec4(1.0f, 0.8f, 0.3f, 0.5f);
                Renderer::render_line(pos, target_pos, dir_color, is_selected ? 2.0f : 1.0f);
                
                // Draw arrow head at endpoint
                float arrow_size = is_selected ? 6.0f : 4.0f;
                Renderer::render_line(
                    Vec3(target_pos.x - arrow_size, target_pos.y - arrow_size, ui_z),
                    Vec3(target_pos.x + arrow_size, target_pos.y + arrow_size, ui_z), dir_color, 2.0f);
                Renderer::render_line(
                    Vec3(target_pos.x + arrow_size, target_pos.y - arrow_size, ui_z),
                    Vec3(target_pos.x - arrow_size, target_pos.y + arrow_size, ui_z), dir_color, 2.0f);
            }
        }
        
        // Draw player marker (cyan circle)
        if (Game::g_state.player_entity != ECS::INVALID_ENTITY) {
            auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(Game::g_state.player_entity);
            if (transform) {
                Vec3 pos(transform->position.x * scale_x, transform->position.y * scale_y, ui_z);
                bool is_selected = (g_state.entity_selection_type == EntitySelectionType::PLAYER);
                
                Vec4 color = is_selected ? Vec4(0.0f, 1.0f, 1.0f, 1.0f) : Vec4(0.0f, 0.7f, 0.7f, 0.6f);
                float size = is_selected ? 18.0f : 14.0f;
                
                // Draw circle (approximated with lines)
                const int segments = 8;
                for (int s = 0; s < segments; s++) {
                    float a1 = (float)s / segments * 6.28318f;
                    float a2 = (float)(s + 1) / segments * 6.28318f;
                    Renderer::render_line(
                        Vec3(pos.x + cosf(a1) * size, pos.y + sinf(a1) * size, ui_z),
                        Vec3(pos.x + cosf(a2) * size, pos.y + sinf(a2) * size, ui_z),
                        color, 2.0f);
                }
                
                // For selected player, no need to visualize elevation as it's shown in status bar
            }
        }
    }
    
    // Draw mode indicator with black background
    char mode_text[256];
    if (g_state.mode == EditorMode::SELECT_ENTITY && g_state.selected_entity != ECS::INVALID_ENTITY) {
        // Show selected entity info with position, Z value, etc.
        auto& ecs = Game::g_state.ecs_world;
        bool is_2_5d = (g_state.entity_selection_type == EntitySelectionType::PROP ||
                        g_state.entity_selection_type == EntitySelectionType::PLAYER);
        
        if (is_2_5d) {
            auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(g_state.selected_entity);
            float x = transform ? transform->position.x : 0.0f;
            float y = transform ? transform->position.y : 0.0f;
            float elev = transform ? transform->elevation : 0.0f;
            
            // 2.5D Objects: Show position and elevation
            snprintf(mode_text, sizeof(mode_text), "[%s #%d] Pos=(%.0f,%.0f) Elev=%.2f | Drag(XY) Shift+Drag(Elev) +/-", 
                     get_entity_type_name(g_state.entity_selection_type), 
                     g_state.selected_entity_index, x, y, elev);
        } else {
            auto* transform = ecs.get_component<ECS::Transform3DComponent>(g_state.selected_entity);
            // Convert OpenGL coords back to pixel for display
            float px = transform ? (transform->position.x + 1.0f) * 0.5f * Config::BASE_WIDTH : 0.0f;
            float py = transform ? (1.0f - transform->position.y) * 0.5f * Config::BASE_HEIGHT : 0.0f;
            float z = transform ? transform->position.z : 0.0f;
            
            // 3D Lights: Show position and Z
            if (g_state.entity_selection_type == EntitySelectionType::PROJECTOR_LIGHT) {
                // Projector light: Show Shift+Click hint for target
                snprintf(mode_text, sizeof(mode_text), "[PROJECTOR #%d] Pos=(%.0f,%.0f) Z=%.2f | Shift+Click=set target, Drag=move", 
                         g_state.selected_entity_index, px, py, z);
            } else {
                snprintf(mode_text, sizeof(mode_text), "[%s #%d] Pos=(%.0f,%.0f) Z=%.2f | Drag(XY) Shift+Drag(Z) +/-", 
                         get_entity_type_name(g_state.entity_selection_type), 
                         g_state.selected_entity_index, px, py, z);
            }
        }
    } else if (g_state.mode == EditorMode::SELECT_ENTITY) {
        snprintf(mode_text, sizeof(mode_text), "[ENTITY MODE] Click to select, E=exit");
    } else if (g_state.selection_type == SelectionType::HOTSPOT && g_state.selected_polygon_index >= 0) {
        const auto& hotspot = scene.geometry.hotspots[g_state.selected_polygon_index];
        snprintf(mode_text, sizeof(mode_text), "[HOTSPOT: %s] Shift+Click to set target", 
                 hotspot.name.c_str());
    } else {
        snprintf(mode_text, sizeof(mode_text), "[%s] W=walkable O=obstacle H=hotspot E=entity R=reload", get_mode_string());
    }
    
    // Black semi-transparent background for mode indicator
    float mode_y = (float)window_h - 25.0f;
    Vec3 mode_bg_pos = Vec3(0.0f, mode_y - 5.0f, ui_z);
    Vec2 mode_bg_size = Vec2((float)window_w, 28.0f);
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
        if (!hotspot.tooltip_key.empty()) {
            file << "      \"tooltip_key\": \"" << hotspot.tooltip_key << "\",\n";
        }
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
            
            // Parse tooltip_key (optional)
            size_t tooltip_key = content.find("\"tooltip_key\"", obj_start);
            if (tooltip_key < obj_end && tooltip_key != std::string::npos) {
                size_t tt_start = content.find('\"', tooltip_key + 13);
                size_t tt_end = content.find('\"', tt_start + 1);
                hotspot.tooltip_key = content.substr(tt_start + 1, tt_end - tt_start - 1);
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

// ============================================================================
// ENTITY SERIALIZATION
// ============================================================================

bool save_entities(const char* scene_name) {
    if (!scene_name || scene_name[0] == '\0') {
        DEBUG_ERROR("[GeoEditor] Cannot save entities: no scene name");
        return false;
    }
    
    const auto& scene = Game::g_state.scene;
    auto& ecs = Game::g_state.ecs_world;
    
    // Build absolute path to assets directory
    std::string root = get_project_root();
    std::string path = root + "assets/scenes/";
    path += scene_name;
    path += "/entities.json";
    
    std::ofstream file(path);
    if (!file.is_open()) {
        DEBUG_ERROR("[GeoEditor] Failed to open %s for writing", path.c_str());
        return false;
    }
    
    file << "{\n";
    
    // Save props
    file << "  \"props\": [\n";
    for (size_t i = 0; i < scene.prop_entities.size(); i++) {
        auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(scene.prop_entities[i]);
        if (!transform) continue;
        
        file << "    {\n";
        file << "      \"index\": " << i << ",\n";
        file << "      \"position\": [" << transform->position.x << ", " << transform->position.y << "],\n";
        file << "      \"elevation\": " << transform->elevation << "\n";
        file << "    }";
        if (i + 1 < scene.prop_entities.size()) file << ",";
        file << "\n";
    }
    file << "  ],\n";
    
    // Save point lights
    file << "  \"point_lights\": [\n";
    for (size_t i = 0; i < scene.light_entities.size(); i++) {
        auto* transform = ecs.get_component<ECS::Transform3DComponent>(scene.light_entities[i]);
        auto* light = ecs.get_component<ECS::LightComponent>(scene.light_entities[i]);
        if (!transform) continue;
        
        file << "    {\n";
        file << "      \"index\": " << i << ",\n";
        file << "      \"position\": [" << transform->position.x << ", " << transform->position.y << ", " << transform->position.z << "]";
        if (light) {
            file << ",\n";
            file << "      \"color\": [" << light->color.x << ", " << light->color.y << ", " << light->color.z << "],\n";
            file << "      \"intensity\": " << light->intensity << ",\n";
            file << "      \"radius\": " << light->radius << ",\n";
            file << "      \"casts_shadows\": " << (light->casts_shadows ? "true" : "false") << "\n";
        } else {
            file << "\n";
        }
        file << "    }";
        if (i + 1 < scene.light_entities.size()) file << ",";
        file << "\n";
    }
    file << "  ],\n";
    
    // Save projector lights
    file << "  \"projector_lights\": [\n";
    for (size_t i = 0; i < scene.projector_light_entities.size(); i++) {
        auto* transform = ecs.get_component<ECS::Transform3DComponent>(scene.projector_light_entities[i]);
        auto* projector = ecs.get_component<ECS::ProjectorLightComponent>(scene.projector_light_entities[i]);
        if (!transform) continue;
        
        file << "    {\n";
        file << "      \"index\": " << i << ",\n";
        file << "      \"position\": [" << transform->position.x << ", " << transform->position.y << ", " << transform->position.z << "]";
        if (projector) {
            file << ",\n";
            file << "      \"direction\": [" << projector->direction.x << ", " << projector->direction.y << ", " << projector->direction.z << "],\n";
            file << "      \"target_distance\": " << projector->target_distance << ",\n";
            file << "      \"color\": [" << projector->color.x << ", " << projector->color.y << ", " << projector->color.z << "],\n";
            file << "      \"intensity\": " << projector->intensity << "\n";
        } else {
            file << "\n";
        }
        file << "    }";
        if (i + 1 < scene.projector_light_entities.size()) file << ",";
        file << "\n";
    }
    file << "  ],\n";
    
    // Save player position
    file << "  \"player\": {\n";
    if (Game::g_state.player_entity != ECS::INVALID_ENTITY) {
        auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(Game::g_state.player_entity);
        if (transform) {
            file << "    \"position\": [" << transform->position.x << ", " << transform->position.y << "],\n";
            file << "    \"elevation\": " << transform->elevation << "\n";
        }
    }
    file << "  }\n";
    
    file << "}\n";
    file.close();
    
    DEBUG_INFO("[GeoEditor] Saved entities to %s", path.c_str());
    return true;
}

bool load_entities(const char* scene_name) {
    if (!scene_name || scene_name[0] == '\0') {
        DEBUG_ERROR("[GeoEditor] Cannot load entities: no scene name");
        return false;
    }
    
    // Build absolute path to assets directory
    std::string root = get_project_root();
    std::string path = root + "assets/scenes/";
    path += scene_name;
    path += "/entities.json";
    
    std::ifstream file(path);
    if (!file.is_open()) {
        DEBUG_LOG("[GeoEditor] No entities file at %s (will create on save)", path.c_str());
        return false;
    }
    
    // Read entire file
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    auto& scene = Game::g_state.scene;
    auto& ecs = Game::g_state.ecs_world;
    
    // Parse props section
    size_t props_start = content.find("\"props\"");
    if (props_start != std::string::npos) {
        size_t arr_start = content.find('[', props_start);
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
            
            // Parse index
            int index = -1;
            size_t idx_key = content.find("\"index\"", obj_start);
            if (idx_key < obj_end) {
                size_t colon = content.find(':', idx_key);
                sscanf(content.c_str() + colon + 1, "%d", &index);
            }
            
            // Parse position
            size_t pos_key = content.find("\"position\"", obj_start);
            if (pos_key < obj_end && index >= 0 && index < (int)scene.prop_entities.size()) {
                size_t pos_arr_start = content.find('[', pos_key);
                size_t pos_arr_end = content.find(']', pos_arr_start);
                std::string coords = content.substr(pos_arr_start + 1, pos_arr_end - pos_arr_start - 1);
                float x = 0, y = 0;
                if (sscanf(coords.c_str(), "%f, %f", &x, &y) == 2 ||
                    sscanf(coords.c_str(), "%f,%f", &x, &y) == 2) {
                    auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(scene.prop_entities[index]);
                    if (transform) {
                        transform->position.x = x;
                        transform->position.y = y;
                    }
                }
            }
            
            // Parse z_depth
            size_t z_key = content.find("\"z_depth\"", obj_start);
            if (z_key < obj_end && index >= 0 && index < (int)scene.prop_entities.size()) {
                size_t colon = content.find(':', z_key);
                float z = 0;
                sscanf(content.c_str() + colon + 1, "%f", &z);
                auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(scene.prop_entities[index]);
                if (transform) {
                    transform->z_depth = z;
                }
            }
            
            // Parse elevation
            size_t elev_key = content.find("\"elevation\"", obj_start);
            if (elev_key < obj_end && index >= 0 && index < (int)scene.prop_entities.size()) {
                size_t colon = content.find(':', elev_key);
                float elev = 0;
                sscanf(content.c_str() + colon + 1, "%f", &elev);
                auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(scene.prop_entities[index]);
                if (transform) {
                    transform->elevation = elev;
                }
            }
            
            pos = obj_end + 1;
        }
    }
    
    // Parse point_lights section
    size_t lights_start = content.find("\"point_lights\"");
    if (lights_start != std::string::npos) {
        size_t arr_start = content.find('[', lights_start);
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
            
            // Parse index
            int index = -1;
            size_t idx_key = content.find("\"index\"", obj_start);
            if (idx_key < obj_end) {
                size_t colon = content.find(':', idx_key);
                sscanf(content.c_str() + colon + 1, "%d", &index);
            }
            
            // Parse position (3D)
            size_t pos_key = content.find("\"position\"", obj_start);
            if (pos_key < obj_end && index >= 0 && index < (int)scene.light_entities.size()) {
                size_t pos_arr_start = content.find('[', pos_key);
                size_t pos_arr_end = content.find(']', pos_arr_start);
                std::string coords = content.substr(pos_arr_start + 1, pos_arr_end - pos_arr_start - 1);
                float x = 0, y = 0, z = 0;
                if (sscanf(coords.c_str(), "%f, %f, %f", &x, &y, &z) == 3 ||
                    sscanf(coords.c_str(), "%f,%f,%f", &x, &y, &z) == 3) {
                    auto* transform = ecs.get_component<ECS::Transform3DComponent>(scene.light_entities[index]);
                    if (transform) {
                        transform->position = Vec3(x, y, z);
                    }
                }
            }
            
            pos = obj_end + 1;
        }
    }
    
    // Parse projector_lights section
    size_t projector_start = content.find("\"projector_lights\"");
    if (projector_start != std::string::npos) {
        size_t arr_start = content.find('[', projector_start);
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
            
            // Parse index
            int index = -1;
            size_t idx_key = content.find("\"index\"", obj_start);
            if (idx_key < obj_end) {
                size_t colon = content.find(':', idx_key);
                sscanf(content.c_str() + colon + 1, "%d", &index);
            }
            
            // Parse position (3D)
            size_t pos_key = content.find("\"position\"", obj_start);
            if (pos_key < obj_end && index >= 0 && index < (int)scene.projector_light_entities.size()) {
                size_t pos_arr_start = content.find('[', pos_key);
                size_t pos_arr_end = content.find(']', pos_arr_start);
                std::string coords = content.substr(pos_arr_start + 1, pos_arr_end - pos_arr_start - 1);
                float x = 0, y = 0, z = 0;
                if (sscanf(coords.c_str(), "%f, %f, %f", &x, &y, &z) == 3 ||
                    sscanf(coords.c_str(), "%f,%f,%f", &x, &y, &z) == 3) {
                    auto* transform = ecs.get_component<ECS::Transform3DComponent>(scene.projector_light_entities[index]);
                    if (transform) {
                        transform->position = Vec3(x, y, z);
                    }
                }
            }
            
            // Parse direction
            size_t dir_key = content.find("\"direction\"", obj_start);
            if (dir_key < obj_end && index >= 0 && index < (int)scene.projector_light_entities.size()) {
                size_t dir_arr_start = content.find('[', dir_key);
                size_t dir_arr_end = content.find(']', dir_arr_start);
                std::string coords = content.substr(dir_arr_start + 1, dir_arr_end - dir_arr_start - 1);
                float x = 0, y = 0, z = 0;
                if (sscanf(coords.c_str(), "%f, %f, %f", &x, &y, &z) == 3 ||
                    sscanf(coords.c_str(), "%f,%f,%f", &x, &y, &z) == 3) {
                    auto* projector = ecs.get_component<ECS::ProjectorLightComponent>(scene.projector_light_entities[index]);
                    if (projector) {
                        // Normalize direction
                        float len = sqrtf(x*x + y*y + z*z);
                        if (len > 0.001f) {
                            projector->direction = Vec3(x/len, y/len, z/len);
                        }
                    }
                }
            }
            
            // Parse target_distance
            size_t dist_key = content.find("\"target_distance\"", obj_start);
            if (dist_key < obj_end && index >= 0 && index < (int)scene.projector_light_entities.size()) {
                size_t colon = content.find(':', dist_key);
                float dist = 0.5f;
                sscanf(content.c_str() + colon + 1, "%f", &dist);
                auto* projector = ecs.get_component<ECS::ProjectorLightComponent>(scene.projector_light_entities[index]);
                if (projector) {
                    projector->target_distance = dist;
                }
            }
            
            pos = obj_end + 1;
        }
    }
    
    // Parse player section
    size_t player_start = content.find("\"player\"");
    if (player_start != std::string::npos && Game::g_state.player_entity != ECS::INVALID_ENTITY) {
        size_t obj_start = content.find('{', player_start);
        size_t obj_end = content.find('}', obj_start);
        
        // Parse position
        size_t pos_key = content.find("\"position\"", obj_start);
        if (pos_key < obj_end) {
            size_t pos_arr_start = content.find('[', pos_key);
            size_t pos_arr_end = content.find(']', pos_arr_start);
            std::string coords = content.substr(pos_arr_start + 1, pos_arr_end - pos_arr_start - 1);
            float x = 0, y = 0;
            if (sscanf(coords.c_str(), "%f, %f", &x, &y) == 2 ||
                sscanf(coords.c_str(), "%f,%f", &x, &y) == 2) {
                auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(Game::g_state.player_entity);
                if (transform) {
                    transform->position.x = x;
                    transform->position.y = y;
                }
            }
        }
        
        // Parse z_depth
        size_t z_key = content.find("\"z_depth\"", obj_start);
        if (z_key < obj_end) {
            size_t colon = content.find(':', z_key);
            float z = 0;
            sscanf(content.c_str() + colon + 1, "%f", &z);
            auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(Game::g_state.player_entity);
            if (transform) {
                transform->z_depth = z;
            }
        }
        
        // Parse elevation
        size_t elev_key = content.find("\"elevation\"", obj_start);
        if (elev_key < obj_end) {
            size_t colon = content.find(':', elev_key);
            float elev = 0;
            sscanf(content.c_str() + colon + 1, "%f", &elev);
            auto* transform = ecs.get_component<ECS::Transform2_5DComponent>(Game::g_state.player_entity);
            if (transform) {
                transform->elevation = elev;
            }
        }
    }
    
    DEBUG_INFO("[GeoEditor] Loaded entity positions from %s", path.c_str());
    return true;
}

}
