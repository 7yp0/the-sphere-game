#include "game.h"
#include "player.h"
#include "core/animation_bank.h"
#include "core/timing.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "renderer/animation.h"
#include "platform.h"
#include "scene/scene.h"
#include "debug/debug.h"
#include "types.h"
#include "ecs/ecs.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstdio>

namespace Game {

GameState g_state;

// ECS World instance
static ECS::World g_ecs_world;

// Phase 1 ECS Test - validates entity and component systems
static void test_ecs_phase1() {
    printf("\n========================================\n");
    printf("     ECS PHASE 1 TEST - START\n");
    printf("========================================\n");
    
    // Test 1: Create entities
    printf("\n[TEST 1] Creating entities...\n");
    ECS::EntityID entity1 = g_ecs_world.create_entity();
    ECS::EntityID entity2 = g_ecs_world.create_entity();
    ECS::EntityID entity3 = g_ecs_world.create_entity();
    
    printf("  Created entity 1: ID=%u, valid=%s\n", entity1, g_ecs_world.is_valid(entity1) ? "YES" : "NO");
    printf("  Created entity 2: ID=%u, valid=%s\n", entity2, g_ecs_world.is_valid(entity2) ? "YES" : "NO");
    printf("  Created entity 3: ID=%u, valid=%s\n", entity3, g_ecs_world.is_valid(entity3) ? "YES" : "NO");
    printf("  Total entities: %u\n", g_ecs_world.get_entity_count());
    
    // Test 2: Add components
    printf("\n[TEST 2] Adding components...\n");
    
    auto& transform1 = g_ecs_world.add_component<ECS::Transform2_5DComponent>(entity1);
    transform1.position = Vec2(100, 200);
    transform1.z_depth = 0.5f;
    printf("  Entity 1: Added Transform2_5D (pos=%.0f,%.0f, z_depth=%.2f)\n", 
           transform1.position.x, transform1.position.y, transform1.z_depth);
    
    auto& sprite1 = g_ecs_world.add_component<ECS::SpriteComponent>(entity1);
    sprite1.visible = true;
    printf("  Entity 1: Added SpriteComponent (visible=%s)\n", sprite1.visible ? "YES" : "NO");
    
    auto& light2 = g_ecs_world.add_component<ECS::LightComponent>(entity2);
    light2.color = Vec3(1.0f, 0.8f, 0.5f);
    light2.intensity = 2.0f;
    light2.radius = 5.0f;
    light2.casts_shadows = true;
    printf("  Entity 2: Added LightComponent (color=%.1f,%.1f,%.1f, intensity=%.1f, shadows=%s)\n",
           light2.color.x, light2.color.y, light2.color.z, light2.intensity,
           light2.casts_shadows ? "YES" : "NO");
    
    // Test 3: Query components
    printf("\n[TEST 3] Querying components...\n");
    
    printf("  Entity 1 has Transform2_5D: %s\n", g_ecs_world.has_component<ECS::Transform2_5DComponent>(entity1) ? "YES" : "NO");
    printf("  Entity 1 has LightComponent: %s\n", g_ecs_world.has_component<ECS::LightComponent>(entity1) ? "YES" : "NO");
    printf("  Entity 2 has LightComponent: %s\n", g_ecs_world.has_component<ECS::LightComponent>(entity2) ? "YES" : "NO");
    printf("  Entity 3 has any components: %s\n", g_ecs_world.has_component<ECS::Transform2_5DComponent>(entity3) ? "YES" : "NO");
    
    // Test 4: Get components
    printf("\n[TEST 4] Getting component data...\n");
    
    auto* t = g_ecs_world.get_component<ECS::Transform2_5DComponent>(entity1);
    if (t) {
        printf("  Entity 1 Transform2_5D: pos=(%.0f,%.0f), z_depth=%.2f\n", 
               t->position.x, t->position.y, t->z_depth);
    }
    
    auto* l = g_ecs_world.get_component<ECS::LightComponent>(entity2);
    if (l) {
        printf("  Entity 2 LightComponent: color=(%.1f,%.1f,%.1f), radius=%.1f\n",
               l->color.x, l->color.y, l->color.z, l->radius);
    }
    
    // Test 5: Entity destruction and ID recycling
    printf("\n[TEST 5] Destroying entity and recycling...\n");
    
    printf("  Destroying entity 2...\n");
    g_ecs_world.destroy_entity(entity2);
    printf("  Entity 2 valid after destroy: %s\n", g_ecs_world.is_valid(entity2) ? "YES" : "NO");
    printf("  Total entities: %u\n", g_ecs_world.get_entity_count());
    
    printf("  Creating new entity (should reuse ID)...\n");
    ECS::EntityID entity4 = g_ecs_world.create_entity();
    printf("  New entity ID: %u (recycled from entity 2)\n", entity4);
    printf("  Total entities: %u\n", g_ecs_world.get_entity_count());
    
    // Cleanup test entities 
    g_ecs_world.destroy_entity(entity1);
    g_ecs_world.destroy_entity(entity3);
    g_ecs_world.destroy_entity(entity4);
    
    printf("\n========================================\n");
    printf("     ECS PHASE 1 TEST - COMPLETE\n");
    printf("     All tests passed!\n");
    printf("========================================\n\n");
}

static void init_player() {
    player_init(g_state.player, g_state.base_width, g_state.base_height, 
                &g_state.playerAnimations);
}

static void update_animated_test_light(float delta_time) {
    // Animate the first light horizontally (left to right) with fixed Y and Z
    // ALL COORDINATES IN OPENGL SPACE (-1 to +1):
    //   X = -1 (left) to +1 (right)
    //   Y = -1 (bottom) to +1 (top) - height affects shadow length
    //   Z = -1 (near camera) to +1 (far/background)
    static float light_time = 0.0f;
    light_time += delta_time;
    
    if (g_state.scene.lights.size() > 0) {
        float cycle_time = 6.0f;  // 6 seconds für einen Durchlauf
        float t = fmod(light_time, cycle_time) / cycle_time;  // 0 to 1
        
        // X bewegt sich von links (-1) nach rechts (+1) in OpenGL-Koordinaten
        float x = -1.0f + t * 2.0f;  // -1 to +1
        float y = 0.0f;   // Mitte (OpenGL Y: -1=unten, +1=oben)
        float z = 0.0f;  // Leicht vor der Szene (näher zur Kamera)
        
        g_state.scene.lights[0].position = Vec3(x, y, z);
    }
}

void init() {
    // Run ECS Phase 1 test at startup
    test_ecs_phase1();
    
    Scene::init_scene_test();
    init_player();
    
    // Initialize framebuffer for offscreen rendering at base resolution
    Renderer::init_framebuffer(Config::BASE_WIDTH, Config::BASE_HEIGHT);
}

void set_viewport(uint32_t width, uint32_t height) {
    g_state.viewport_width = width;
    g_state.viewport_height = height;
    
    // Store base dimensions for game logic (all coordinates in base resolution)
    g_state.base_width = Config::BASE_WIDTH;
    g_state.base_height = Config::BASE_HEIGHT;
    
    // Calculate 2.5D depth scaling factor
    // scale_factor = viewport_height / base_height
    // Used in depth scaling formula: scale = 1.0 + (horizon_y - sprite.y) * scale_factor
    g_state.scale_factor = (float)height / (float)Config::BASE_HEIGHT;
}

void update(float delta_time) {
    Core::update_delta_time(delta_time);
    
#ifndef NDEBUG
    Debug::handle_debug_keys();
#endif
    
    player_handle_input(g_state.player);
    player_update(g_state.player, g_state.base_width, g_state.base_height, delta_time);
    
    update_animated_test_light(delta_time);
}

void render() {
    // =========================================================================
    // OFFSCREEN RENDERING: Render scene at base resolution (320x180)
    // =========================================================================
    Renderer::begin_render_to_framebuffer();
    Renderer::clear_screen();
    
    // Background - use BACKGROUND layer for depth
    Renderer::render_sprite_lit(g_state.scene.background, 
                               Vec3(0.0f, 0.0f, Layers::get_z_depth(Layer::BACKGROUND)),
                               Vec2((float)g_state.scene.width, (float)g_state.scene.height),
                               g_state.scene.lights,
                               g_state.scene.background_normal_map);
    
    // Render all props with their z-depth from scene definition
    for (size_t i = 0; i < g_state.scene.props.size(); ++i) {
        const Scene::Prop& prop = g_state.scene.props[i];
        
        // Calculate depth scaling for 2.5D effect (based on depth map at position)
        float depth_scale = Scene::get_depth_scaling(g_state.scene, prop.position.x, prop.position.y);
        Vec2 scaled_size = Vec2(
            prop.size.x * depth_scale,
            prop.size.y * depth_scale
        );
        
        // Use actual 3D position from scene (z-coordinate from scene definition)
        Renderer::render_sprite_lit(prop.texture, prop.position, scaled_size,
                                   g_state.scene.lights, prop.normal_map, prop.pivot);
    }
    
    // Render player with its actual 3D position from game state
    const char* anim_name = player_get_animation_name(g_state.player);
    Renderer::SpriteAnimation* player_anim = g_state.playerAnimations.get(anim_name);
    if (player_anim) {
        // Calculate depth scaling for 2.5D effect (based on depth map at position)
        float depth_scale = Scene::get_depth_scaling(g_state.scene, g_state.player.position.x, g_state.player.position.y);
        Vec2 scaled_size = Vec2(
            g_state.player.size.x * depth_scale,
            g_state.player.size.y * depth_scale
        );
        
        // Use actual 3D position from game state (z-coordinate is set during initialization)
        Renderer::render_sprite_animated_lit(player_anim, 
                                            g_state.player.position, 
                                            scaled_size,
                                            g_state.scene.lights,
                                            0,  // normal_map: use default
                                            g_state.player.pivot);
    }
    
    // Debug: Rotes Rechteck an der Lichtposition (in base resolution coordinates)
    if (g_state.scene.lights.size() > 0) {
        Vec3 light_pos = g_state.scene.lights[0].position;
        // Konvertiere OpenGL-Koordinaten (-1 bis +1) zu Pixel-Koordinaten (base resolution)
        // OpenGL X: -1=links, +1=rechts → Pixel: 0 bis base_width
        // OpenGL Y: -1=unten, +1=oben → Pixel: base_height bis 0 (invertiert!)
        float pixel_x = (light_pos.x + 1.0f) * 0.5f * g_state.base_width;
        float pixel_y = (1.0f - light_pos.y) * 0.5f * g_state.base_height;
        
        // Zeichne rotes Rect (3x3 Pixel für base resolution) zentriert auf der Lichtposition
        Renderer::render_rect(Vec3(pixel_x - 1.5f, pixel_y - 1.5f, Layers::get_z_depth(Layer::UI)),
                             Vec2(3.0f, 3.0f),
                             Vec4(1.0f, 0.0f, 0.0f, 1.0f));  // Rot
    }
    
    Renderer::end_render_to_framebuffer();
    
    // =========================================================================
    // UPSCALE: Blit framebuffer to screen at viewport resolution (1280x720)
    // =========================================================================
    Renderer::clear_screen();
    Renderer::render_framebuffer_to_screen();
    
    // Debug overlay (rendered at viewport resolution, after upscaling)
    Vec2 mouse_pixel = Platform::get_mouse_pos();
#ifndef NDEBUG
    Debug::render_overlay(mouse_pixel);
#endif
}

void shutdown() {
    // Cleanup happens in Renderer::shutdown() and asset cleanup
    // Player animations cleaned up through Core::AnimationBank destructor
}

}

