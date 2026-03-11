#include "scene.h"
#include "game/game.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "debug/debug_log.h"
#include "config.h"
#include "ecs/ecs.h"

using Game::g_state;

namespace Scene {

// Helper: Create ECS entity from a Prop and add to prop_entities
static ECS::EntityID create_prop_entity(const Prop& prop) {
    ECS::EntityID entity = g_state.ecs_world.create_entity();
    
    // Add Transform2_5D
    auto& transform = g_state.ecs_world.add_component<ECS::Transform2_5DComponent>(entity);
    transform.position = Vec2(prop.position.x, prop.position.y);
    transform.z_depth = prop.position.z;
    transform.scale = Vec2(1.0f, 1.0f);
    
    // Update z_depth from depth map if available
    if (g_state.scene.depth_map.is_valid()) {
        ECS::TransformHelpers::update_z_from_depth_map(
            transform, g_state.scene.depth_map,
            g_state.scene.width, g_state.scene.height);
    }
    
    // Add SpriteComponent
    auto& sprite = g_state.ecs_world.add_component<ECS::SpriteComponent>(entity);
    sprite.texture = prop.texture;
    sprite.normal_map = prop.normal_map;
    sprite.base_size = prop.size;
    sprite.pivot = prop.pivot;
    sprite.visible = true;
    
    // Add ShadowCasterComponent (props cast shadows by default)
    auto& shadow = g_state.ecs_world.add_component<ECS::ShadowCasterComponent>(entity);
    shadow.enabled = true;
    shadow.alpha_threshold = 0.3f;
    shadow.shadow_intensity = 0.7f;
    
    printf("[ECS] Created prop '%s': Entity=%u, pos=(%.0f,%.0f), z=%.2f, shadow_caster=YES\n",
           prop.name.c_str(), entity, 
           transform.position.x, transform.position.y, transform.z_depth);
    
    return entity;
}

// Helper: Create ECS entity for a point light
static ECS::EntityID create_light_entity(Vec3 position, Vec3 color, float intensity, float radius, bool casts_shadows = true) {
    ECS::EntityID entity = g_state.ecs_world.create_entity();
    
    // Lights use Transform3D for free 3D positioning (OpenGL coords)
    auto& transform = g_state.ecs_world.add_component<ECS::Transform3DComponent>(entity);
    transform.position = position;
    
    // Add LightComponent
    auto& light = g_state.ecs_world.add_component<ECS::LightComponent>(entity);
    light.color = color;
    light.intensity = intensity;
    light.radius = radius;
    light.casts_shadows = casts_shadows;
    light.enabled = true;
    
    printf("[ECS] Created light: Entity=%u, pos=(%.2f,%.2f,%.2f), color=(%.1f,%.1f,%.1f), intensity=%.1f, radius=%.1f\n",
           entity, position.x, position.y, position.z, color.x, color.y, color.z, intensity, radius);
    
    return entity;
}

void init_scene_test() {
    Scene scene;
    scene.name = "test";
    // Scene dimensions in BASE resolution (320x180)
    scene.width = Config::BASE_WIDTH;
    scene.height = Config::BASE_HEIGHT;
    scene.background = Renderer::load_texture("scenes/test/backgrounds/bg_room.png");
    scene.background_normal_map = Renderer::load_texture("scenes/test/backgrounds/bg_room_normal_map.png");
    scene.depth_map = Renderer::load_depth_map("scenes/test/backgrounds/bg_room_depth_map.png");
    
    printf("[DEBUG] Depth map loaded - TextureID: %u, Size: %ux%u, Valid: %s\n", 
        scene.depth_map.texture_id, scene.depth_map.width, scene.depth_map.height,
        scene.depth_map.is_valid() ? "YES" : "NO");

    Prop bg_box;
    bg_box.position = Vec3(75.0f, 130.0f, 0.0f);
    bg_box.size = Vec2(25.0f, 25.0f);
    bg_box.texture = Renderer::load_texture("scenes/test/props/bg_box.png");
    bg_box.normal_map =Renderer::load_texture("scenes/test/props/bg_box_normal_map.png");
    bg_box.name = "bg_box";
    bg_box.pivot = PivotPoint::BOTTOM_CENTER;
    scene.props.push_back(bg_box);
    
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
    
    g_state.scene = scene;
    
    // Setup depth map data in renderer for shader sampling
    if (g_state.scene.depth_map.is_valid()) {
        Renderer::set_depth_map_data(g_state.scene.depth_map.texture_id, g_state.scene.width, g_state.scene.height);
    }
    
    // Create ECS entities for all props
    printf("\n[ECS] Creating prop entities from scene...\n");
    g_state.scene.prop_entities.clear();
    for (const auto& prop : g_state.scene.props) {
        ECS::EntityID entity = create_prop_entity(prop);
        g_state.scene.prop_entities.push_back(entity);
    }
    printf("[ECS] Created %zu prop entities\n\n", g_state.scene.prop_entities.size());
    
    // Create ECS entities for lights
    printf("[ECS] Creating light entities...\n");
    g_state.scene.light_entities.clear();
    
    // Warm center light
    ECS::EntityID warm_light = create_light_entity(
        Vec3(0.0f, 0.0f, -0.5f),     // Center screen, slightly in front (OpenGL coords)
        Vec3(1.0f, 0.9f, 0.7f),      // Warm white color
        1.5f,                         // Intensity
        2.0f,                         // Radius (OpenGL units)
        true                          // Casts shadows
    );
    g_state.scene.light_entities.push_back(warm_light);
    
    printf("[ECS] Created %zu light entities\n\n", g_state.scene.light_entities.size());
}

}
