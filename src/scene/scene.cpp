#include "scene.h"
#include "game/game.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "debug/debug_log.h"
#include "config.h"
#include "ecs/ecs.h"
#include "ecs/entity_factory.h"

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
    
    printf("[SCENE] Depth map loaded - TextureID: %u, Size: %ux%u, Valid: %s\n", 
        scene.depth_map.texture_id, scene.depth_map.width, scene.depth_map.height,
        scene.depth_map.is_valid() ? "YES" : "NO");

    // Load textures for props
    Renderer::TextureID box_texture = Renderer::load_texture("scenes/test/props/bg_box.png");
    Renderer::TextureID box_normal = Renderer::load_texture("scenes/test/props/bg_box_normal_map.png");
    
    // Setup scene geometry (walkable areas and hotspots)
    Collision::Polygon main_walkable;
    main_walkable.points = {
        Vec2(25.0f, 25.0f),
        Vec2(295.0f, 25.0f),
        Vec2(295.0f, 162.5f),
        Vec2(25.0f, 162.5f)
    };
    scene.geometry.walkable_areas.push_back(main_walkable);

    Hotspot box_hotspot;
    box_hotspot.name = "box_hotspot";
    box_hotspot.enabled = true;
    box_hotspot.interaction_distance = 0.25f;
    box_hotspot.bounds.points = {
        Vec2(62.5f, 105.0f),
        Vec2(87.5f, 105.0f),
        Vec2(87.5f, 155.0f),
        Vec2(62.5f, 155.0f)
    };
    box_hotspot.callback = []() {
        DEBUG_LOG("Player interacted with box!");
    };
    scene.geometry.hotspots.push_back(box_hotspot);
    
    // Assign scene to global state BEFORE creating entities
    // (factory functions access g_state.scene for depth map)
    g_state.scene = scene;
    
    // Setup depth map data in renderer for shader sampling
    if (g_state.scene.depth_map.is_valid()) {
        Renderer::set_depth_map_data(g_state.scene.depth_map.texture_id, g_state.scene.width, g_state.scene.height);
    }
    
    // =========================================================================
    // CREATE ECS ENTITIES USING FACTORY FUNCTIONS
    // =========================================================================
    
    printf("\n[ECS] Creating prop entities via factory functions...\n");
    g_state.scene.prop_entities.clear();
    
    // Box prop - shadow casting
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
    printf("[ECS] Created box prop: Entity=%u\n", box_entity);
    
    printf("[ECS] Created %zu prop entities\n\n", g_state.scene.prop_entities.size());
    
    // =========================================================================
    // CREATE LIGHT ENTITIES
    // =========================================================================
    
    printf("[ECS] Creating light entities via factory functions...\n");
    g_state.scene.light_entities.clear();
    
    // Warm center light
    ECS::EntityID warm_light = ECS::create_point_light(
        Vec3(0.0f, 0.0f, -0.5f),     // Center screen, slightly in front (OpenGL coords)
        Vec3(1.0f, 0.9f, 0.7f),      // Warm white color
        1.5f,                         // Intensity
        2.0f,                         // Radius (OpenGL units)
        true                          // Casts shadows
    );
    g_state.scene.light_entities.push_back(warm_light);
    printf("[ECS] Created warm light: Entity=%u\n", warm_light);
    
    printf("[ECS] Created %zu light entities\n\n", g_state.scene.light_entities.size());
}

}
