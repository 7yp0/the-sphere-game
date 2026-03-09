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
    scene.depth_map = Renderer::load_depth_map("scenes/test/backgrounds/bg_room_depth_map.png");
    
    printf("[DEBUG] Depth map loaded - TextureID: %u, Size: %ux%u, Valid: %s\n", 
        scene.depth_map.texture_id, scene.depth_map.width, scene.depth_map.height,
        scene.depth_map.is_valid() ? "YES" : "NO");
    
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
    // COORDINATES IN OPENGL SPACE (-1 to +1):
    //   X = -1 (left) to +1 (right)
    //   Y = -1 (bottom) to +1 (top) - height affects shadow length
    //   Z = -1 (near camera) to +1 (far/background)
    PointLight warm_light;
    warm_light.position = Vec3(0.0f, 0.0f, -0.5f);  // Center screen, slightly in front
    warm_light.color = Vec3(1.0f, 0.9f, 0.7f);   // Warm white
    warm_light.intensity = 1.5f;                  // Strong
    warm_light.radius = 2.0f;                     // OpenGL units (covers most of screen)
    scene.lights.push_back(warm_light);
    
    g_state.scene = scene;
    
    // Setup depth map data in renderer for shader sampling
    if (g_state.scene.depth_map.is_valid()) {
        Renderer::set_depth_map_data(g_state.scene.depth_map.texture_id, g_state.scene.width, g_state.scene.height);
    }
}

}
