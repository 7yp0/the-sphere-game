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
    scene.background_normal_map = Renderer::load_texture("scenes/test/backgrounds/bg_room_normal_map.png");
    scene.height_map = Renderer::load_height_map("scenes/test/backgrounds/bg_room_height_map.png");
    
    printf("[DEBUG] Height map loaded - TextureID: %u, Size: %ux%u, Valid: %s\n", 
        scene.height_map.texture_id, scene.height_map.width, scene.height_map.height,
        scene.height_map.is_valid() ? "YES" : "NO");
    
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
    bg_box.position = Vec3(300.0f, 520.0f, 0.0f);  // z=0 is default depth
    bg_box.size = Vec2(100.0f, 100.0f);
    bg_box.texture = Renderer::load_texture("scenes/test/props/bg_box.png");
    bg_box.normal_map =Renderer::load_texture("scenes/test/props/bg_box_normal_map.png");
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
    
    // Setup point lights for dynamic scene illumination
    PointLight warm_light;
    warm_light.position = Vec3(640.0f, 380.0f, 0.0f);  // Center screen
    warm_light.color = Vec3(1.0f, 0.6f, 0.2f);   // Orange-red
    warm_light.intensity = 1.2f;                  // Strong
    warm_light.radius = 1500.0f;
    scene.lights.push_back(warm_light);
    
    PointLight cool_light;
    cool_light.position = Vec3(380.0f, 520.0f, 0.0f);  // Near left
    cool_light.color = Vec3(0.2f, 0.8f, 1.0f);   // Cyan-blue
    cool_light.intensity = 1.0f;
    cool_light.radius = 500.0f;
    scene.lights.push_back(cool_light);
    
    PointLight green_light;
    green_light.position = Vec3(1050.0f, 60.0f, 0.0f);  // Right side
    green_light.color = Vec3(0.3f, 1.0f, 0.4f);   // Green
    green_light.intensity = 0.8f;
    green_light.radius = 450.0f;
    scene.lights.push_back(green_light);
    
    g_state.scene = scene;
    
    // Setup height map data in renderer for shader sampling
    if (g_state.scene.height_map.is_valid()) {
        Renderer::set_height_map_data(g_state.scene.height_map.texture_id, g_state.scene.width, g_state.scene.height);
    }
}

}
