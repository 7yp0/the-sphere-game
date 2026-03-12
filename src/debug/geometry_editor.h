#pragma once

#include "types.h"
#include "collision/polygon_utils.h"
#include "ecs/ecs.h"
#include <vector>
#include <string>

namespace GeometryEditor {

enum class EditorMode {
    NONE,               // Not editing
    CREATING_WALKABLE,  // Adding vertices to new walkable area
    CREATING_HOTSPOT,   // Adding vertices to new hotspot
    CREATING_OBSTACLE,  // Adding vertices to new obstacle
    SELECT_ENTITY       // Selecting/moving entities (props, lights, player)
};

enum class SelectionType {
    NONE,
    WALKABLE_AREA,
    HOTSPOT,
    OBSTACLE
};

enum class EntitySelectionType {
    NONE,
    PROP,
    POINT_LIGHT,
    PROJECTOR_LIGHT,
    PLAYER
};

struct EditorState {
    EditorMode mode = EditorMode::NONE;
    
    // Currently being created polygon
    std::vector<Vec2> current_polygon_points;
    std::string current_polygon_name;
    
    // Selection state (polygons)
    SelectionType selection_type = SelectionType::NONE;
    int selected_polygon_index = -1;
    int selected_vertex_index = -1;
    int hovered_vertex_index = -1;
    
    // Entity selection state
    EntitySelectionType entity_selection_type = EntitySelectionType::NONE;
    ECS::EntityID selected_entity = ECS::INVALID_ENTITY;
    int selected_entity_index = -1;  // Index in prop_entities/light_entities array
    
    // Drag state
    bool dragging = false;
    Vec2 drag_start;
    Vec2 drag_offset;
    
    // Double-click detection (frame-based)
    int last_click_frame = -1000;  // Frame number of last click
    Vec2 last_click_pos = Vec2(0, 0);
    
    // Editor active state
    bool is_active = false;
};

// Initialize editor state
void init();

// Update editor input (call every frame when debug mode is active)
void update(Vec2 mouse_base_coords);

// Handle mouse click (in base resolution coordinates)
void on_mouse_click(Vec2 pos);

// Handle right-click (delete vertex)
void on_mouse_right_click(Vec2 pos);

// Handle mouse release
void on_mouse_release();

// Render editor overlay (polygons, vertices, preview)
void render();

// Get current editor mode as string
const char* get_mode_string();

// Check if editor is currently active
bool is_active();

// Toggle editor on/off
void toggle();

// Save geometry to JSON
bool save_geometry(const char* scene_name);

// Load geometry from JSON
bool load_geometry(const char* scene_name);

// Save entities (props, lights) to JSON
bool save_entities(const char* scene_name);

// Load entities from JSON (returns false if file doesn't exist)
bool load_entities(const char* scene_name);

// Access editor state (for debug display)
const EditorState& get_state();

}
