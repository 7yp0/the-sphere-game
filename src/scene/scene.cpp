#include "scene.h"
#include "game/game.h"
#include "game/dialogue.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "debug/debug_log.h"
#include "debug/geometry_editor.h"
#include "config.h"
#include "ecs/ecs.h"
#include "ecs/entity_factory.h"
#include "inventory/inventory.h"

using Game::g_state;

namespace Scene {

void init_scene_test() {
    Scene scene;
    scene.name = "test";
    // Scene dimensions in BASE resolution (320x180)
    scene.width = Config::BASE_WIDTH;
    scene.height = Config::BASE_HEIGHT;
    scene.background = Renderer::load_texture("scenes/test/backgrounds/bg_room.png");
    scene.background_normal_map = Renderer::load_texture("scenes/test/backgrounds/bg_room_normal_map.png");
    scene.depth_map = Renderer::load_depth_map("scenes/test/backgrounds/bg_room_depth_map.png");

    // Load textures for props
    Renderer::TextureID box_texture = Renderer::load_texture("scenes/test/props/bg_box.png");
    Renderer::TextureID box_normal = Renderer::load_texture("scenes/test/props/bg_box_normal_map.png");
    Renderer::TextureID tree_texture = Renderer::load_texture("scenes/test/props/prop_tree.png");
    Renderer::TextureID stone_texture = Renderer::load_texture("scenes/test/props/prop_stone.png");
    
    // Assign scene to global state BEFORE loading geometry
    // (so load_geometry can access g_state.scene)
    g_state.scene = scene;
    
    // Load geometry from JSON (walkable areas + hotspots)
    GeometryEditor::load_geometry(scene.name.c_str());
    
    // NOTE: Add more callback registrations here as you create hotspots in the editor:
    // register_hotspot_callback("door", []() { /* door interaction */ });
    // register_hotspot_callback("tree", []() { /* tree interaction */ });
    
    // Setup depth map data in renderer for shader sampling
    if (g_state.scene.depth_map.is_valid()) {
        Renderer::set_depth_map_data(g_state.scene.depth_map.texture_id, g_state.scene.width, g_state.scene.height);
    }
    
    // =========================================================================
    // CREATE ECS ENTITIES USING FACTORY FUNCTIONS
    // =========================================================================
    
    // Box prop - shadow casting (solid shape)
    ECS::EntityID box_entity = ECS::create_shadow_casting_prop(
        Vec2(75.0f, 130.0f),    // Position (pixel coords)
        Vec2(25.0f, 25.0f),     // Size
        box_texture,
        box_normal,
        PivotPoint::BOTTOM_CENTER,
        0.3f,                    // Alpha threshold
        0.7f                     // Shadow intensity
    );
    ECS::update_entity_z_from_depth_map(box_entity, g_state.scene.depth_map, 
                                        g_state.scene.width, g_state.scene.height);
    g_state.scene.prop_entities.push_back(box_entity);
    register_entity("box", box_entity);
    
    // Tree prop - shadow casting with ALPHA TESTING (irregular silhouette)
    ECS::EntityID tree_entity = ECS::create_shadow_casting_prop(
        Vec2(240.0f, 145.0f),   // Position (right side of room)
        Vec2(32.0f, 48.0f),     // Size (tall tree)
        tree_texture,
        0,                       // No normal map
        PivotPoint::BOTTOM_CENTER,
        0.5f,                    // Higher alpha threshold for clean edges
        0.6f                     // Shadow intensity
    );
    ECS::update_entity_z_from_depth_map(tree_entity, g_state.scene.depth_map, 
                                        g_state.scene.width, g_state.scene.height);
    g_state.scene.prop_entities.push_back(tree_entity);
    register_entity("tree", tree_entity);
    
    // Stone prop - STATIC (no shadow casting) - for testing non-shadow props
    ECS::EntityID stone_entity = ECS::create_static_prop(
        Vec2(200.0f, 125.0f),   // Position (floor center-right)
        Vec2(16.0f, 12.0f),     // Size (small stone)
        stone_texture,
        0,                       // No normal map
        PivotPoint::BOTTOM_CENTER
    );
    ECS::update_entity_z_from_depth_map(stone_entity, g_state.scene.depth_map, 
                                        g_state.scene.width, g_state.scene.height);
    g_state.scene.prop_entities.push_back(stone_entity);
    register_entity("stone", stone_entity);
    
    // =========================================================================
    // CREATE LIGHT ENTITIES
    // =========================================================================

    g_state.scene.light_entities.clear();
    
    // Warm center light (SHADOW CASTING - main light)
    ECS::EntityID warm_light = ECS::create_point_light(
        Vec3(0.0f, 0.0f, -0.5f),     // Center screen, slightly in front (OpenGL coords)
        Vec3(1.0f, 0.9f, 0.7f),      // Warm white color
        1.5f,                         // Intensity
        2.0f,                         // Radius (OpenGL units)
        true                          // Casts shadows
    );
    g_state.scene.light_entities.push_back(warm_light);
    register_entity("warm_light", warm_light);
    
    // Blue fill light (NO SHADOWS - ambient fill)
    ECS::EntityID fill_light = ECS::create_point_light(
        Vec3(0.5f, -0.3f, 0.0f),     // Right side, lower (OpenGL coords)
        Vec3(0.4f, 0.5f, 0.8f),      // Cool blue color
        0.8f,                         // Lower intensity
        1.5f,                         // Radius
        false                         // NO shadow casting
    );
    g_state.scene.light_entities.push_back(fill_light);
    register_entity("fill_light", fill_light);
    
    // =========================================================================
    // Create projector lights (window lights)
    // =========================================================================
    g_state.scene.projector_light_entities.clear();
    
    // Load the window cookie texture
    Renderer::TextureID window_cookie = Renderer::load_texture("lights/window_cookie.png");
    
    // Create window light on the left wall
    // Position higher up, projecting diagonally right and down onto the floor
    ECS::EntityID window_light = ECS::create_projector_light(
        Vec2(20.0f, 35.0f),          // Left side, higher up (pixels)
        0.0f,                        // Z depth (near camera, like a window on the wall)
        Vec3(1.0f, -0.9f, 0.0f),    // Direction: pointing right (+X) and into room (+Z)
        Vec3(0.0f, 1.0f, -0.25f),      // Up: pointing up (+Y)
        Vec3(1.0f, 0.95f, 0.85f),    // Warm sunlight color
        2.5f,                         // Intensity
        20.0f,                        // FOV (narrower for cleaner pattern)
        1.0f,                         // Aspect ratio (square window)
        3.0f,                         // Range (in Z units, covers room depth)
        window_cookie
    );
    g_state.scene.projector_light_entities.push_back(window_light);
    register_entity("window_light", window_light);
    
    // Load entity positions from JSON (overwrites hardcoded positions if file exists)
    GeometryEditor::load_entities(scene.name.c_str());
    
    // Register inventory items for this scene
    Inventory::register_item("stone", "A smooth grey stone", "ui/items/item_stone.png");
    Inventory::register_item("note", "A torn note with strange symbols", "ui/items/item_note.png");
    
    // Player starts with the mysterious note
    Inventory::add_item("note");
    
    // Make the note non-selectable - clicking it shows its contents
    Inventory::set_item_on_use("note", []() {
        DEBUG_LOG("You read the note: 'The sphere awakens at midnight...'");
    });

    // Register hotspot callbacks by name
    // (geometry comes from JSON, callbacks are defined here in code)
    register_hotspot_callback("box_hotspot", []() {
        DEBUG_LOG("Player interacted with box!");
    });

    register_hotspot_callback("hotspot_tree", []() {
        Dialogue::say(g_state.player_entity, "dialogue.sphere.hello", 2.0f);
    });

    register_hotspot_callback("stone_hotspot", []() {
        // Add stone to inventory
        if (Inventory::add_item("stone") >= 0) {
            // Hide stone prop by name
            set_entity_visible("stone", false);
            
            // Disable the hotspot so it can't be clicked again
            set_hotspot_enabled("stone_hotspot", false);
        } else {
            DEBUG_LOG("Inventory full or Item not registered!");
        }
    });

    register_hotspot_item_callback("box_hotspot", "stone", []() {
        Inventory::remove_item("stone");
    });
    
    // Set default invalid combination feedback
    Inventory::set_invalid_combination_callback([](const std::string& item1, const std::string& item2) {
        Dialogue::say(g_state.player_entity, "generic.cant_combine");
        // TODO: Play error sound, show speech bubble, etc.
    });
}

bool register_hotspot_callback(const std::string& hotspot_name, std::function<void()> callback) {
    for (auto& hotspot : g_state.scene.geometry.hotspots) {
        if (hotspot.name == hotspot_name) {
            hotspot.callback = callback;
            return true;
        }
    }
    DEBUG_ERROR("[Scene] Hotspot '%s' not found - callback not registered", hotspot_name.c_str());
    return false;
}

bool register_hotspot_item_callback(const std::string& hotspot_name, const std::string& item_id, std::function<void()> callback) {
    for (auto& hotspot : g_state.scene.geometry.hotspots) {
        if (hotspot.name == hotspot_name) {
            hotspot.item_callbacks[item_id] = callback;
            return true;
        }
    }
    DEBUG_ERROR("[Scene] Hotspot '%s' not found - item callback not registered", hotspot_name.c_str());
    return false;
}

bool set_hotspot_enabled(const std::string& hotspot_name, bool enabled) {
    for (auto& hotspot : g_state.scene.geometry.hotspots) {
        if (hotspot.name == hotspot_name) {
            hotspot.enabled = enabled;
            return true;
        }
    }
    DEBUG_ERROR("[Scene] Hotspot '%s' not found", hotspot_name.c_str());
    return false;
}

Hotspot* get_hotspot(const std::string& hotspot_name) {
    for (auto& hotspot : g_state.scene.geometry.hotspots) {
        if (hotspot.name == hotspot_name) {
            return &hotspot;
        }
    }
    return nullptr;
}

void register_entity(const std::string& name, ECS::EntityID entity) {
    g_state.scene.named_entities[name] = entity;
}

ECS::EntityID get_entity(const std::string& name) {
    auto it = g_state.scene.named_entities.find(name);
    if (it != g_state.scene.named_entities.end()) {
        return it->second;
    }
    DEBUG_ERROR("[Scene] Entity '%s' not found", name.c_str());
    return ECS::INVALID_ENTITY;
}

bool set_entity_visible(const std::string& name, bool visible) {
    ECS::EntityID entity = get_entity(name);
    if (entity == ECS::INVALID_ENTITY) {
        return false;
    }
    
    auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(entity);
    if (sprite) {
        sprite->visible = visible;
        return true;
    }
    return false;
}

}
