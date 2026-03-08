#include "scene.h"
#include "game/game.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "debug/debug_log.h"

using Game::g_state;

namespace Scene {

void init_scene_test() {
    Scene scene;
    scene.name = "test";
    scene.width = 1280;
    scene.height = 720;
    scene.background = Renderer::load_texture("scenes/test/backgrounds/bg_room.png");
    scene.height_map = Renderer::load_texture("scenes/test/backgrounds/bg_room_height_map.png");
    
    HorizonLine scale_down_horizon;
    scale_down_horizon.y_position = 460.0f;
    scale_down_horizon.scale_gradient = 0.005f;
    scale_down_horizon.depth_scale_inverted = false;
    scene.horizons.push_back(scale_down_horizon);

    HorizonLine scale_up_horizon;
    scale_up_horizon.y_position = 480.0f;
    scale_up_horizon.scale_gradient = 0.005f; 
    scale_up_horizon.depth_scale_inverted = false;
    scene.horizons.push_back(scale_up_horizon);
    
    Prop bg_box;
    bg_box.position = Vec2(300.0f, 520.0f);
    bg_box.size = Vec2(100.0f, 100.0f);
    bg_box.texture = Renderer::load_texture("scenes/test/props/bg_box.png");
    bg_box.name = "bg_box";
    bg_box.pivot = PivotPoint::BOTTOM_CENTER;
    scene.props.push_back(bg_box);
    
    // Setup scene geometry (walkable areas and hotspots)
    Collision::Polygon main_walkable;
    main_walkable.points = {
        Vec2(100.0f, 100.0f),
        Vec2(1180.0f, 100.0f),
        Vec2(1180.0f, 650.0f),
        Vec2(100.0f, 650.0f)
    };
    scene.geometry.walkable_areas.push_back(main_walkable);
    
    // Hotspot for the box
    Hotspot box_hotspot;
    box_hotspot.name = "box_hotspot";
    box_hotspot.enabled = true;
    box_hotspot.interaction_distance = 1.0f;
    box_hotspot.bounds.points = {
        Vec2(250.0f, 420.0f),   // Bottom-left
        Vec2(350.0f, 420.0f),   // Bottom-right
        Vec2(350.0f, 620.0f),   // Top-right
        Vec2(250.0f, 620.0f)    // Top-left
    };
    box_hotspot.callback = []() {
        DEBUG_LOG("Player interacted with box!");
    };
    scene.geometry.hotspots.push_back(box_hotspot);
    
    g_state.scene = scene;
}

}
