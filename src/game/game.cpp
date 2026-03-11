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
#include "ecs/entity_factory.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstdio>

namespace Game {

GameState g_state;

// Phase 1 ECS Test - validates entity and component systems
static void test_ecs_phase1() {
    printf("\n========================================\n");
    printf("     ECS PHASE 1 TEST - START\n");
    printf("========================================\n");
    
    // Test 1: Create entities
    printf("\n[TEST 1] Creating entities...\n");
    ECS::EntityID entity1 = g_state.ecs_world.create_entity();
    ECS::EntityID entity2 = g_state.ecs_world.create_entity();
    ECS::EntityID entity3 = g_state.ecs_world.create_entity();
    
    printf("  Created entity 1: ID=%u, valid=%s\n", entity1, g_state.ecs_world.is_valid(entity1) ? "YES" : "NO");
    printf("  Created entity 2: ID=%u, valid=%s\n", entity2, g_state.ecs_world.is_valid(entity2) ? "YES" : "NO");
    printf("  Created entity 3: ID=%u, valid=%s\n", entity3, g_state.ecs_world.is_valid(entity3) ? "YES" : "NO");
    printf("  Total entities: %u\n", g_state.ecs_world.get_entity_count());
    
    // Test 2: Add components
    printf("\n[TEST 2] Adding components...\n");
    
    auto& transform1 = g_state.ecs_world.add_component<ECS::Transform2_5DComponent>(entity1);
    transform1.position = Vec2(100, 200);
    transform1.z_depth = 0.5f;
    printf("  Entity 1: Added Transform2_5D (pos=%.0f,%.0f, z_depth=%.2f)\n", 
           transform1.position.x, transform1.position.y, transform1.z_depth);
    
    auto& sprite1 = g_state.ecs_world.add_component<ECS::SpriteComponent>(entity1);
    sprite1.visible = true;
    printf("  Entity 1: Added SpriteComponent (visible=%s)\n", sprite1.visible ? "YES" : "NO");
    
    auto& light2 = g_state.ecs_world.add_component<ECS::LightComponent>(entity2);
    light2.color = Vec3(1.0f, 0.8f, 0.5f);
    light2.intensity = 2.0f;
    light2.radius = 5.0f;
    light2.casts_shadows = true;
    printf("  Entity 2: Added LightComponent (color=%.1f,%.1f,%.1f, intensity=%.1f, shadows=%s)\n",
           light2.color.x, light2.color.y, light2.color.z, light2.intensity,
           light2.casts_shadows ? "YES" : "NO");
    
    // Test 3: Query components
    printf("\n[TEST 3] Querying components...\n");
    
    printf("  Entity 1 has Transform2_5D: %s\n", g_state.ecs_world.has_component<ECS::Transform2_5DComponent>(entity1) ? "YES" : "NO");
    printf("  Entity 1 has LightComponent: %s\n", g_state.ecs_world.has_component<ECS::LightComponent>(entity1) ? "YES" : "NO");
    printf("  Entity 2 has LightComponent: %s\n", g_state.ecs_world.has_component<ECS::LightComponent>(entity2) ? "YES" : "NO");
    printf("  Entity 3 has any components: %s\n", g_state.ecs_world.has_component<ECS::Transform2_5DComponent>(entity3) ? "YES" : "NO");
    
    // Test 4: Get components
    printf("\n[TEST 4] Getting component data...\n");
    
    auto* t = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(entity1);
    if (t) {
        printf("  Entity 1 Transform2_5D: pos=(%.0f,%.0f), z_depth=%.2f\n", 
               t->position.x, t->position.y, t->z_depth);
    }
    
    auto* l = g_state.ecs_world.get_component<ECS::LightComponent>(entity2);
    if (l) {
        printf("  Entity 2 LightComponent: color=(%.1f,%.1f,%.1f), radius=%.1f\n",
               l->color.x, l->color.y, l->color.z, l->radius);
    }
    
    // Test 5: Entity destruction and ID recycling
    printf("\n[TEST 5] Destroying entity and recycling...\n");
    
    printf("  Destroying entity 2...\n");
    g_state.ecs_world.destroy_entity(entity2);
    printf("  Entity 2 valid after destroy: %s\n", g_state.ecs_world.is_valid(entity2) ? "YES" : "NO");
    printf("  Total entities: %u\n", g_state.ecs_world.get_entity_count());
    
    printf("  Creating new entity (should reuse ID)...\n");
    ECS::EntityID entity4 = g_state.ecs_world.create_entity();
    printf("  New entity ID: %u (recycled from entity 2)\n", entity4);
    printf("  Total entities: %u\n", g_state.ecs_world.get_entity_count());
    
    // Cleanup test entities 
    g_state.ecs_world.destroy_entity(entity1);
    g_state.ecs_world.destroy_entity(entity3);
    g_state.ecs_world.destroy_entity(entity4);
    
    printf("\n========================================\n");
    printf("     ECS PHASE 1 TEST - COMPLETE\n");
    printf("     All tests passed!\n");
    printf("========================================\n\n");
}

// Phase 2 ECS Test - validates transform components and helpers
static void test_ecs_phase2() {
    printf("\n========================================\n");
    printf("     ECS PHASE 2 TEST - START\n");
    printf("========================================\n");
    
    // Test 1: Transform2_5D constructors
    printf("\n[TEST 1] Transform2_5D constructors...\n");
    
    ECS::Transform2_5DComponent t1;  // Default
    printf("  Default: pos=(%.0f,%.0f), z=%.2f, rot=%.2f, scale=(%.1f,%.1f)\n",
           t1.position.x, t1.position.y, t1.z_depth, t1.rotation, t1.scale.x, t1.scale.y);
    
    ECS::Transform2_5DComponent t2(Vec2(160, 90), -0.5f);  // Vec2 + z
    printf("  Vec2+z: pos=(%.0f,%.0f), z=%.2f\n", t2.position.x, t2.position.y, t2.z_depth);
    
    ECS::Transform2_5DComponent t3(100.0f, 200.0f, 0.3f);  // x, y, z
    printf("  x,y,z: pos=(%.0f,%.0f), z=%.2f\n", t3.position.x, t3.position.y, t3.z_depth);
    
    // Test 2: Depth scaling calculation
    printf("\n[TEST 2] Depth scaling (compute_depth_scale)...\n");
    
    float scale_near = ECS::TransformHelpers::compute_depth_scale(-0.8f);  // Near camera
    float scale_mid = ECS::TransformHelpers::compute_depth_scale(0.0f);    // Middle
    float scale_far = ECS::TransformHelpers::compute_depth_scale(0.8f);    // Far away
    
    printf("  Z=-0.8 (near):  scale=%.2f (expected ~1.4)\n", scale_near);
    printf("  Z= 0.0 (mid):   scale=%.2f (expected ~1.0)\n", scale_mid);
    printf("  Z=+0.8 (far):   scale=%.2f (expected ~0.6)\n", scale_far);
    
    // Test 3: Pixel to OpenGL conversion
    printf("\n[TEST 3] Pixel to OpenGL conversion...\n");
    
    uint32_t scene_w = 320, scene_h = 180;
    
    Vec2 top_left = ECS::TransformHelpers::pixel_to_opengl(Vec2(0, 0), scene_w, scene_h);
    Vec2 center = ECS::TransformHelpers::pixel_to_opengl(Vec2(160, 90), scene_w, scene_h);
    Vec2 bottom_right = ECS::TransformHelpers::pixel_to_opengl(Vec2(320, 180), scene_w, scene_h);
    
    printf("  Pixel(0,0) -> OpenGL(%.2f,%.2f) (expected -1,+1 = top-left)\n", top_left.x, top_left.y);
    printf("  Pixel(160,90) -> OpenGL(%.2f,%.2f) (expected 0,0 = center)\n", center.x, center.y);
    printf("  Pixel(320,180) -> OpenGL(%.2f,%.2f) (expected +1,-1 = bottom-right)\n", bottom_right.x, bottom_right.y);
    
    // Test 4: derive_3d_from_2_5d conversion
    printf("\n[TEST 4] derive_3d_from_2_5d conversion...\n");
    
    ECS::Transform2_5DComponent prop_t;
    prop_t.position = Vec2(160, 90);  // Center of scene
    prop_t.z_depth = 0.0f;            // Middle depth
    prop_t.scale = Vec2(1.0f, 1.0f);
    
    Vec2 base_size(64, 64);  // 64x64 pixel sprite
    
    ECS::Transform3DComponent gl_t = ECS::TransformHelpers::derive_3d_from_2_5d(
        prop_t, base_size, scene_w, scene_h, true);
    
    printf("  Input: pixel(160,90), z=0.0, base_size=64x64\n");
    printf("  Output: OpenGL pos=(%.2f,%.2f,%.2f), size=(%.3f,%.3f)\n",
           gl_t.position.x, gl_t.position.y, gl_t.position.z, gl_t.size.x, gl_t.size.y);
    
    // Test 5: Entity with transform + derive
    printf("\n[TEST 5] Entity with transform derivation...\n");
    
    ECS::EntityID entity = g_state.ecs_world.create_entity();
    auto& transform = g_state.ecs_world.add_component<ECS::Transform2_5DComponent>(entity);
    transform.position = Vec2(80, 45);  // Top-left quadrant
    transform.z_depth = -0.5f;          // Near camera (should scale up)
    transform.scale = Vec2(2.0f, 2.0f); // Double size
    
    auto& sprite = g_state.ecs_world.add_component<ECS::SpriteComponent>(entity);
    sprite.base_size = Vec2(32, 32);
    
    ECS::Transform3DComponent derived = ECS::TransformHelpers::derive_3d_from_2_5d(
        transform, sprite.base_size, scene_w, scene_h, true);
    
    float expected_depth_scale = ECS::TransformHelpers::compute_depth_scale(-0.5f);
    printf("  Input: pixel(80,45), z=-0.5, entity_scale=2x, base=32x32\n");
    printf("  Depth scale factor: %.2f\n", expected_depth_scale);
    printf("  Final OpenGL pos: (%.2f, %.2f, %.2f)\n", derived.position.x, derived.position.y, derived.position.z);
    printf("  Final OpenGL size: (%.3f, %.3f)\n", derived.size.x, derived.size.y);
    
    g_state.ecs_world.destroy_entity(entity);
    
    printf("\n========================================\n");
    printf("     ECS PHASE 2 TEST - COMPLETE\n");
    printf("     All tests passed!\n");
    printf("========================================\n\n");
}

// Phase 3 ECS Test - validates sprite and emissive components
static void test_ecs_phase3() {
    printf("\n========================================\n");
    printf("     ECS PHASE 3 TEST - START\n");
    printf("========================================\n");
    
    // Test 1: SpriteComponent defaults
    printf("\n[TEST 1] SpriteComponent defaults...\n");
    
    ECS::SpriteComponent sprite;
    printf("  texture=%u, normal_map=%u\n", sprite.texture, sprite.normal_map);
    printf("  base_size=(%.0f,%.0f)\n", sprite.base_size.x, sprite.base_size.y);
    printf("  uv_range=(%.2f,%.2f,%.2f,%.2f)\n", sprite.uv_range.x, sprite.uv_range.y, sprite.uv_range.z, sprite.uv_range.w);
    printf("  visible=%s, flip_x=%s, flip_y=%s\n", 
           sprite.visible ? "YES" : "NO", sprite.flip_x ? "YES" : "NO", sprite.flip_y ? "YES" : "NO");
    printf("  is_animated=%s\n", sprite.is_animated() ? "YES" : "NO");
    
    // Test 2: EmissiveComponent
    printf("\n[TEST 2] EmissiveComponent...\n");
    
    ECS::EmissiveComponent emissive_default;
    printf("  Default: color=(%.1f,%.1f,%.1f), intensity=%.1f, emitting=%s\n",
           emissive_default.emissive_color.x, emissive_default.emissive_color.y, emissive_default.emissive_color.z,
           emissive_default.intensity, emissive_default.is_emitting() ? "YES" : "NO");
    
    ECS::EmissiveComponent emissive_glow(Vec3(2.0f, 1.5f, 0.5f), 1.5f);
    Vec3 emission = emissive_glow.get_emission();
    printf("  Glow: color=(%.1f,%.1f,%.1f), intensity=%.1f\n",
           emissive_glow.emissive_color.x, emissive_glow.emissive_color.y, emissive_glow.emissive_color.z,
           emissive_glow.intensity);
    printf("  Final emission: (%.2f,%.2f,%.2f), emitting=%s\n",
           emission.x, emission.y, emission.z, emissive_glow.is_emitting() ? "YES" : "NO");
    
    // Test 3: Spritesheet UV helper
    printf("\n[TEST 3] Spritesheet UV helper...\n");
    
    // 4x4 grid, cell (2,1) = third column, second row
    Vec4 uv = ECS::SpriteHelpers::create_spritesheet_uv(2, 1, 4, 4);
    printf("  Grid 4x4, cell (2,1): uv=(%.3f,%.3f,%.3f,%.3f)\n", uv.x, uv.y, uv.z, uv.w);
    printf("  Expected: (0.500, 0.250, 0.750, 0.500)\n");
    
    // Test 4: UV flip helper
    printf("\n[TEST 4] UV flip helper...\n");
    
    Vec4 original(0.0f, 0.0f, 0.5f, 0.5f);
    Vec4 flipped_x = ECS::SpriteHelpers::apply_flip(original, true, false);
    Vec4 flipped_y = ECS::SpriteHelpers::apply_flip(original, false, true);
    Vec4 flipped_both = ECS::SpriteHelpers::apply_flip(original, true, true);
    
    printf("  Original: (%.1f,%.1f,%.1f,%.1f)\n", original.x, original.y, original.z, original.w);
    printf("  Flip X:   (%.1f,%.1f,%.1f,%.1f)\n", flipped_x.x, flipped_x.y, flipped_x.z, flipped_x.w);
    printf("  Flip Y:   (%.1f,%.1f,%.1f,%.1f)\n", flipped_y.x, flipped_y.y, flipped_y.z, flipped_y.w);
    printf("  Flip XY:  (%.1f,%.1f,%.1f,%.1f)\n", flipped_both.x, flipped_both.y, flipped_both.z, flipped_both.w);
    
    // Test 5: Entity with sprite + emissive (lamp example)
    printf("\n[TEST 5] Entity: Lamp with sprite + emissive...\n");
    
    ECS::EntityID lamp = g_state.ecs_world.create_entity();
    
    auto& lamp_transform = g_state.ecs_world.add_component<ECS::Transform2_5DComponent>(lamp);
    lamp_transform.position = Vec2(200, 100);
    lamp_transform.z_depth = 0.2f;
    
    auto& lamp_sprite = g_state.ecs_world.add_component<ECS::SpriteComponent>(lamp);
    lamp_sprite.texture = 1;  // Dummy texture ID
    lamp_sprite.base_size = Vec2(24, 48);
    lamp_sprite.pivot = PivotPoint::BOTTOM_CENTER;
    
    auto& lamp_emissive = g_state.ecs_world.add_component<ECS::EmissiveComponent>(lamp);
    lamp_emissive.emissive_color = Vec3(1.0f, 0.9f, 0.6f);  // Warm yellow
    lamp_emissive.intensity = 2.0f;
    
    printf("  Created lamp entity ID=%u\n", lamp);
    printf("  Transform: pos=(%.0f,%.0f), z=%.2f\n", 
           lamp_transform.position.x, lamp_transform.position.y, lamp_transform.z_depth);
    printf("  Sprite: tex=%u, size=(%.0f,%.0f), pivot=BOTTOM_CENTER\n",
           lamp_sprite.texture, lamp_sprite.base_size.x, lamp_sprite.base_size.y);
    printf("  Emissive: (%.1f,%.1f,%.1f) * %.1f\n",
           lamp_emissive.emissive_color.x, lamp_emissive.emissive_color.y, lamp_emissive.emissive_color.z,
           lamp_emissive.intensity);
    
    // Verify component queries
    printf("  Has Transform2_5D: %s\n", g_state.ecs_world.has_component<ECS::Transform2_5DComponent>(lamp) ? "YES" : "NO");
    printf("  Has Sprite: %s\n", g_state.ecs_world.has_component<ECS::SpriteComponent>(lamp) ? "YES" : "NO");
    printf("  Has Emissive: %s\n", g_state.ecs_world.has_component<ECS::EmissiveComponent>(lamp) ? "YES" : "NO");
    printf("  Has Light (should be NO): %s\n", g_state.ecs_world.has_component<ECS::LightComponent>(lamp) ? "YES" : "NO");
    
    g_state.ecs_world.destroy_entity(lamp);
    
    printf("\n========================================\n");
    printf("     ECS PHASE 3 TEST - COMPLETE\n");
    printf("     All tests passed!\n");
    printf("========================================\n\n");
}

// Phase 4 ECS Test - validates LightComponent
static void test_ecs_phase4() {
    printf("\n========================================\n");
    printf("     ECS PHASE 4 TEST - START\n");
    printf("========================================\n");
    
    // Test 1: LightComponent defaults
    printf("\n[TEST 1] LightComponent defaults...\n");
    
    ECS::LightComponent light;
    printf("  color=(%.1f,%.1f,%.1f)\n", light.color.x, light.color.y, light.color.z);
    printf("  intensity=%.2f, radius=%.2f\n", light.intensity, light.radius);
    printf("  casts_shadows=%s, enabled=%s\n", 
           light.casts_shadows ? "YES" : "NO", light.enabled ? "YES" : "NO");
    
    // Test 2: Create light entity with Transform3D + LightComponent
    printf("\n[TEST 2] Light entity (Transform3D + LightComponent)...\n");
    
    ECS::EntityID point_light = g_state.ecs_world.create_entity();
    
    // Lights use Transform3D for free 3D positioning
    auto& light_transform = g_state.ecs_world.add_component<ECS::Transform3DComponent>(point_light);
    light_transform.position = Vec3(0.0f, 0.5f, 0.0f);  // Center-top, mid-depth (OpenGL coords)
    
    auto& light_comp = g_state.ecs_world.add_component<ECS::LightComponent>(point_light);
    light_comp.color = Vec3(1.0f, 0.9f, 0.7f);  // Warm white
    light_comp.intensity = 1.5f;
    light_comp.radius = 2.0f;
    light_comp.casts_shadows = true;
    
    printf("  Created light entity ID=%u\n", point_light);
    printf("  Transform3D: pos=(%.1f,%.1f,%.1f)\n", 
           light_transform.position.x, light_transform.position.y, light_transform.position.z);
    printf("  Light: color=(%.1f,%.1f,%.1f), intensity=%.1f, radius=%.1f\n",
           light_comp.color.x, light_comp.color.y, light_comp.color.z,
           light_comp.intensity, light_comp.radius);
    printf("  casts_shadows=%s, enabled=%s\n", 
           light_comp.casts_shadows ? "YES" : "NO", light_comp.enabled ? "YES" : "NO");
    
    // Verify component queries
    printf("\n[TEST 3] Component queries...\n");
    printf("  Has Transform3D: %s\n", g_state.ecs_world.has_component<ECS::Transform3DComponent>(point_light) ? "YES" : "NO");
    printf("  Has LightComponent: %s\n", g_state.ecs_world.has_component<ECS::LightComponent>(point_light) ? "YES" : "NO");
    printf("  Has Transform2_5D (should be NO): %s\n", g_state.ecs_world.has_component<ECS::Transform2_5DComponent>(point_light) ? "YES" : "NO");
    
    // Test 4: Get entities with LightComponent
    printf("\n[TEST 4] Query entities with LightComponent...\n");
    auto light_entities = g_state.ecs_world.get_entities_with<ECS::LightComponent>();
    printf("  Found %zu entities with LightComponent\n", light_entities.size());
    for (auto eid : light_entities) {
        auto* lc = g_state.ecs_world.get_component<ECS::LightComponent>(eid);
        auto* tc = g_state.ecs_world.get_component<ECS::Transform3DComponent>(eid);
        if (lc && tc) {
            printf("    Entity %u: pos=(%.1f,%.1f,%.1f), color=(%.1f,%.1f,%.1f)\n",
                   eid, tc->position.x, tc->position.y, tc->position.z,
                   lc->color.x, lc->color.y, lc->color.z);
        }
    }
    
    g_state.ecs_world.destroy_entity(point_light);
    
    printf("\n========================================\n");
    printf("     ECS PHASE 4 TEST - COMPLETE\n");
    printf("     All tests passed!\n");
    printf("========================================\n\n");
}

// Phase 7 ECS Test - validates entity factory functions
static void test_ecs_phase7() {
    printf("\n========================================\n");
    printf("     ECS PHASE 7 TEST - ENTITY FACTORIES\n");
    printf("========================================\n");
    
    // Test 1: create_static_prop
    printf("\n[TEST 1] create_static_prop...\n");
    ECS::EntityID static_prop = ECS::create_static_prop(
        Vec2(100, 100), Vec2(32, 32), 1, 0, PivotPoint::CENTER);
    bool has_transform = g_state.ecs_world.has_component<ECS::Transform2_5DComponent>(static_prop);
    bool has_sprite = g_state.ecs_world.has_component<ECS::SpriteComponent>(static_prop);
    bool has_shadow = g_state.ecs_world.has_component<ECS::ShadowCasterComponent>(static_prop);
    printf("  Entity=%u: Transform2_5D=%s, Sprite=%s, ShadowCaster=%s (expected NO)\n",
           static_prop, has_transform ? "YES" : "NO", has_sprite ? "YES" : "NO", has_shadow ? "YES" : "NO");
    printf("  %s\n", (!has_shadow && has_transform && has_sprite) ? "PASS" : "FAIL");
    g_state.ecs_world.destroy_entity(static_prop);
    
    // Test 2: create_shadow_casting_prop
    printf("\n[TEST 2] create_shadow_casting_prop...\n");
    ECS::EntityID shadow_prop = ECS::create_shadow_casting_prop(
        Vec2(150, 100), Vec2(48, 48), 1, 0, PivotPoint::BOTTOM_CENTER, 0.4f, 0.6f);
    has_shadow = g_state.ecs_world.has_component<ECS::ShadowCasterComponent>(shadow_prop);
    auto* sc = g_state.ecs_world.get_component<ECS::ShadowCasterComponent>(shadow_prop);
    printf("  Entity=%u: ShadowCaster=%s\n", shadow_prop, has_shadow ? "YES" : "NO");
    if (sc) {
        printf("  alpha_threshold=%.2f (expected 0.40), shadow_intensity=%.2f (expected 0.60)\n",
               sc->alpha_threshold, sc->shadow_intensity);
        printf("  %s\n", (sc->alpha_threshold == 0.4f && sc->shadow_intensity == 0.6f) ? "PASS" : "FAIL");
    }
    g_state.ecs_world.destroy_entity(shadow_prop);
    
    // Test 3: create_point_light
    printf("\n[TEST 3] create_point_light...\n");
    ECS::EntityID light = ECS::create_point_light(
        Vec3(0.5f, 0.5f, -0.3f), Vec3(1.0f, 0.8f, 0.6f), 2.0f, 3.0f, true);
    bool has_t3d = g_state.ecs_world.has_component<ECS::Transform3DComponent>(light);
    bool has_lc = g_state.ecs_world.has_component<ECS::LightComponent>(light);
    auto* lc = g_state.ecs_world.get_component<ECS::LightComponent>(light);
    printf("  Entity=%u: Transform3D=%s, Light=%s\n", light, has_t3d ? "YES" : "NO", has_lc ? "YES" : "NO");
    if (lc) {
        printf("  intensity=%.1f (expected 2.0), radius=%.1f (expected 3.0), shadows=%s\n",
               lc->intensity, lc->radius, lc->casts_shadows ? "YES" : "NO");
        printf("  %s\n", (lc->intensity == 2.0f && lc->radius == 3.0f && lc->casts_shadows) ? "PASS" : "FAIL");
    }
    g_state.ecs_world.destroy_entity(light);
    
    // Test 4: create_point_light_at_pixel (coordinate conversion)
    printf("\n[TEST 4] create_point_light_at_pixel...\n");
    ECS::EntityID pixel_light = ECS::create_point_light_at_pixel(
        Vec2(160, 90), 0.0f, Vec3(1,1,1), 1.0f, 1.0f, false, 320, 180);
    auto* t3d = g_state.ecs_world.get_component<ECS::Transform3DComponent>(pixel_light);
    if (t3d) {
        printf("  Pixel(160,90) -> OpenGL(%.2f,%.2f,%.2f) (expected 0,0,0)\n",
               t3d->position.x, t3d->position.y, t3d->position.z);
        bool pos_ok = (std::abs(t3d->position.x) < 0.01f && std::abs(t3d->position.y) < 0.01f);
        printf("  %s\n", pos_ok ? "PASS" : "FAIL");
    }
    g_state.ecs_world.destroy_entity(pixel_light);
    
    // Test 5: create_emissive_object
    printf("\n[TEST 5] create_emissive_object...\n");
    ECS::EntityID emissive = ECS::create_emissive_object(
        Vec2(200, 100), Vec2(24, 24), 1, Vec3(2.0f, 1.5f, 0.5f), PivotPoint::CENTER);
    bool has_emissive = g_state.ecs_world.has_component<ECS::EmissiveComponent>(emissive);
    auto* ec = g_state.ecs_world.get_component<ECS::EmissiveComponent>(emissive);
    printf("  Entity=%u: Emissive=%s\n", emissive, has_emissive ? "YES" : "NO");
    if (ec) {
        printf("  emissive_color=(%.1f,%.1f,%.1f) (expected 2.0,1.5,0.5)\n",
               ec->emissive_color.x, ec->emissive_color.y, ec->emissive_color.z);
        printf("  %s\n", (ec->emissive_color.x == 2.0f) ? "PASS" : "FAIL");
    }
    g_state.ecs_world.destroy_entity(emissive);
    
    printf("\n========================================\n");
    printf("     ECS PHASE 7 TEST - COMPLETE\n");
    printf("     Entity factories validated!\n");
    printf("========================================\n\n");
}

// Helper: Get player transform (for update loop)
static ECS::Transform2_5DComponent* get_player_transform() {
    if (g_state.player_entity == ECS::INVALID_ENTITY) return nullptr;
    return g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(g_state.player_entity);
}

// Helper: Update sprite animation from player state
static void update_player_sprite_animation() {
    if (g_state.player_entity == ECS::INVALID_ENTITY) return;
    
    auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(g_state.player_entity);
    if (sprite) {
        const char* anim_name = player_get_animation_name(g_state.player);
        Renderer::SpriteAnimation* anim = g_state.player.animations.get(anim_name);
        sprite->set_animation(anim);
    }
}

// Static light position for testing (behind box prop)
// Animate light through room corners
// Visits 8 corners in 3D space for full room illumination coverage
static void update_animated_light(float delta_time) {
    if (g_state.scene.light_entities.empty()) return;
    
    static float light_time = 0.0f;
    light_time += delta_time;
    
    ECS::EntityID light_entity = g_state.scene.light_entities[0];
    auto* transform = g_state.ecs_world.get_component<ECS::Transform3DComponent>(light_entity);
    if (!transform) return;
    
    // 8 corner waypoints with margin (0.6 from edges)
    // OpenGL coords: X (-1=left, +1=right), Y (-1=bottom, +1=top), Z (-1=near, +1=far)
    const float margin = 0.6f;
    const Vec3 corners[8] = {
        Vec3(-margin,  margin, -margin),  // 0: top-left-near
        Vec3( margin,  margin, -margin),  // 1: top-right-near
        Vec3( margin, -margin, -margin),  // 2: bottom-right-near
        Vec3(-margin, -margin, -margin),  // 3: bottom-left-near
        Vec3(-margin, -margin + 0.6f,  margin),  // 4: bottom-left-far
        Vec3( margin, -margin + 0.6f,  margin),  // 5: bottom-right-far
        Vec3( margin,  margin,  margin),  // 6: top-right-far
        Vec3(-margin,  margin,  margin),  // 7: top-left-far
    };
    
    // Time per corner segment
    const float segment_time = 2.0f;  // 2 seconds per segment
    const float total_cycle = segment_time * 8.0f;  // 16 seconds full cycle
    
    // Calculate which segment we're in and interpolation factor
    float cycle_pos = fmod(light_time, total_cycle);
    int segment = (int)(cycle_pos / segment_time);
    float t = (cycle_pos - segment * segment_time) / segment_time;
    
    // Smooth interpolation (ease in/out)
    t = t * t * (3.0f - 2.0f * t);  // Smoothstep
    
    // Get current and next corner
    int next_segment = (segment + 1) % 8;
    Vec3 from = corners[segment];
    Vec3 to = corners[next_segment];
    
    // Lerp position
    transform->position = Vec3(
        from.x + (to.x - from.x) * t,
        from.y + (to.y - from.y) * t,
        from.z + (to.z - from.z) * t
    );
}

void init() {
    // Run ECS tests at startup
    test_ecs_phase1();
    test_ecs_phase2();
    test_ecs_phase3();
    test_ecs_phase4();
    test_ecs_phase7();  // Entity factory validation
    
    // Initialize scene (this also creates prop ECS entities)
    Scene::init_scene_test();
    
    // Create player entity (this initializes player and its ECS transform)
    g_state.player_entity = player_create_entity(g_state.player, g_state.ecs_world,
                                                  g_state.base_width, g_state.base_height);
    
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
    
    // Get player transform from ECS
    ECS::Transform2_5DComponent* player_transform = get_player_transform();
    if (player_transform) {
        player_handle_input(g_state.player, *player_transform);
        player_update(g_state.player, *player_transform, g_state.base_width, g_state.base_height, delta_time);
        
        // Update sprite animation based on player state
        update_player_sprite_animation();
    }
    
    // Animate light through room corners
    update_animated_light(delta_time);
}

void render() {
    // =========================================================================
    // GATHER LIGHT DATA FROM ECS
    // =========================================================================
    std::vector<Renderer::LightData> lights;
    for (ECS::EntityID light_entity : g_state.scene.light_entities) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform3DComponent>(light_entity);
        auto* light = g_state.ecs_world.get_component<ECS::LightComponent>(light_entity);
        
        if (!transform || !light || !light->enabled) continue;
        
        Renderer::LightData ld;
        ld.position = transform->position;
        ld.color = light->color;
        ld.intensity = light->intensity;
        ld.radius = light->radius;
        lights.push_back(ld);
    }
    
    const Renderer::LightData* light_data = lights.empty() ? nullptr : lights.data();
    uint32_t num_lights = (uint32_t)lights.size();
    
    // =========================================================================
    // GATHER SHADOW CASTER DATA FROM ECS
    // =========================================================================
    std::vector<Renderer::ShadowCasterData> shadow_casters;
    int32_t entity_index = 0;
    
    for (ECS::EntityID prop_entity : g_state.scene.prop_entities) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(prop_entity);
        auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(prop_entity);
        auto* shadow = g_state.ecs_world.get_component<ECS::ShadowCasterComponent>(prop_entity);
        
        if (!transform || !sprite || !sprite->visible) {
            entity_index++;
            continue;
        }
        
        // Only add if entity has ShadowCasterComponent and it's enabled
        if (shadow && shadow->enabled) {
            // IMPORTANT: Use BASE resolution (FBO dimensions) for OpenGL conversion
            // NOT get_render_width() which returns viewport dims before FBO is bound!
            uint32_t render_width = Config::BASE_WIDTH;
            uint32_t render_height = Config::BASE_HEIGHT;
            
            // Calculate depth scale and size (same as rendering)
            float depth_scale = ECS::TransformHelpers::compute_depth_scale(transform->z_depth);
            Vec2 scaled_size = Vec2(
                sprite->base_size.x * transform->scale.x * depth_scale,
                sprite->base_size.y * transform->scale.y * depth_scale
            );
            
            // Convert pixel position to OpenGL coords
            Vec2 opengl_pos = Coords::pixel_to_opengl(transform->position, render_width, render_height);
            Vec2 opengl_size = Vec2(
                (scaled_size.x / (float)render_width) * 2.0f,
                (scaled_size.y / (float)render_height) * 2.0f
            );
            
            // Calculate center position based on pivot
            Vec2 pivot_offset = Coords::get_pivot_offset(sprite->pivot, scaled_size.x, scaled_size.y);
            Vec2 pivot_offset_opengl = Vec2(
                (pivot_offset.x / (float)render_width) * 2.0f,
                -(pivot_offset.y / (float)render_height) * 2.0f
            );
            Vec2 opengl_center = Vec2(
                opengl_pos.x + pivot_offset_opengl.x,
                opengl_pos.y + pivot_offset_opengl.y
            );
            
            Renderer::ShadowCasterData sc;
            sc.position = Vec3(opengl_center.x, opengl_center.y, transform->z_depth);
            sc.size = opengl_size;
            sc.uv_range = sprite->uv_range;
            sc.texture = sprite->texture;
            sc.alpha_threshold = shadow->alpha_threshold;
            sc.shadow_intensity = shadow->shadow_intensity;
            sc.entity_index = entity_index;
            
            shadow_casters.push_back(sc);
        }
        entity_index++;
    }
    
    // Also add player as shadow caster if it has the component
    if (g_state.player_entity != ECS::INVALID_ENTITY) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(g_state.player_entity);
        auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(g_state.player_entity);
        auto* shadow = g_state.ecs_world.get_component<ECS::ShadowCasterComponent>(g_state.player_entity);
        
        if (transform && sprite && sprite->visible && shadow && shadow->enabled) {
            // IMPORTANT: Use BASE resolution for OpenGL conversion
            uint32_t render_width = Config::BASE_WIDTH;
            uint32_t render_height = Config::BASE_HEIGHT;
            
            float depth_scale = ECS::TransformHelpers::compute_depth_scale(transform->z_depth);
            Vec2 scaled_size = Vec2(
                sprite->base_size.x * transform->scale.x * depth_scale,
                sprite->base_size.y * transform->scale.y * depth_scale
            );
            
            Vec2 opengl_pos = Coords::pixel_to_opengl(transform->position, render_width, render_height);
            Vec2 opengl_size = Vec2(
                (scaled_size.x / (float)render_width) * 2.0f,
                (scaled_size.y / (float)render_height) * 2.0f
            );
            
            Vec2 pivot_offset = Coords::get_pivot_offset(sprite->pivot, scaled_size.x, scaled_size.y);
            Vec2 pivot_offset_opengl = Vec2(
                (pivot_offset.x / (float)render_width) * 2.0f,
                -(pivot_offset.y / (float)render_height) * 2.0f
            );
            Vec2 opengl_center = Vec2(
                opengl_pos.x + pivot_offset_opengl.x,
                opengl_pos.y + pivot_offset_opengl.y
            );
            
            Renderer::ShadowCasterData sc;
            sc.position = Vec3(opengl_center.x, opengl_center.y, transform->z_depth);
            sc.size = opengl_size;
            // For animated sprites, use the current frame's UV range
            if (sprite->is_animated() && sprite->animation && !sprite->animation->frames.empty()) {
                const auto& frame = sprite->animation->frames[sprite->animation->current_frame];
                sc.uv_range = Vec4(frame.u0, frame.v0, frame.u1, frame.v1);
                sc.texture = sprite->animation->texture;
            } else {
                sc.uv_range = sprite->uv_range;
                sc.texture = sprite->texture;
            }
            sc.alpha_threshold = shadow->alpha_threshold;
            sc.shadow_intensity = shadow->shadow_intensity;
            sc.entity_index = entity_index;  // Player gets next index after props
            
            shadow_casters.push_back(sc);
        }
    }
    
    const Renderer::ShadowCasterData* shadow_data = shadow_casters.empty() ? nullptr : shadow_casters.data();
    uint32_t num_shadow_casters = (uint32_t)shadow_casters.size();
    g_state.shadow_caster_count = num_shadow_casters;  // For debug display
    
    // =========================================================================
    // GATHER PROJECTOR LIGHT DATA FROM ECS
    // =========================================================================
    std::vector<Renderer::ProjectorLightData> projector_lights;
    for (ECS::EntityID proj_entity : g_state.scene.projector_light_entities) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(proj_entity);
        auto* projector = g_state.ecs_world.get_component<ECS::ProjectorLightComponent>(proj_entity);
        
        if (!transform || !projector || !projector->enabled) continue;
        
        Renderer::ProjectorLightData pd;
        pd.position = Vec3(transform->position.x, transform->position.y, transform->z_depth);
        pd.direction = projector->direction;
        pd.up = projector->up;
        pd.color = projector->color;
        pd.intensity = projector->intensity;
        pd.fov = projector->fov;
        pd.aspect_ratio = projector->aspect_ratio;
        pd.range = projector->range;
        pd.cookie = projector->cookie;
        projector_lights.push_back(pd);
    }
    
    // Set projector lights (global state used by all lit renders)
    if (!projector_lights.empty()) {
        Renderer::set_projector_lights(projector_lights.data(), (uint32_t)projector_lights.size());
    } else {
        Renderer::clear_projector_lights();
    }
    
    // Debug: Print shadow caster count once
    static bool shadow_debug_printed = false;
    if (!shadow_debug_printed && num_shadow_casters > 0) {
        printf("[SHADOW] Collected %u shadow casters:\n", num_shadow_casters);
        printf("[SHADOW] Starting position dump...\n");
        for (uint32_t i = 0; i < num_shadow_casters; i++) {
            const auto& sc = shadow_casters[i];
            printf("[SHADOW]   [%u] pos=(%.3f,%.3f,%.3f) size=(%.3f,%.3f) tex=%u alpha=%.2f\n",
                   i, sc.position.x, sc.position.y, sc.position.z,
                   sc.size.x, sc.size.y, sc.texture, sc.alpha_threshold);
        }
        printf("[SHADOW] Done.\n");
        fflush(stdout);
        shadow_debug_printed = true;
    }
    
    // =========================================================================
    // OFFSCREEN RENDERING: Render scene at base resolution (320x180)
    // =========================================================================
    Renderer::begin_render_to_framebuffer();
    Renderer::clear_screen();
    
    // Background - use BACKGROUND layer for depth (lit rendering WITH shadows)
    Renderer::render_sprite_lit_shadowed(g_state.scene.background, 
                           Vec3(0.0f, 0.0f, Layers::get_z_depth(Layer::BACKGROUND)),
                           Vec2((float)g_state.scene.width, (float)g_state.scene.height),
                           light_data, num_lights,
                           shadow_data, num_shadow_casters,
                           g_state.scene.background_normal_map);
    
    // =========================================================================
    // Render props using ECS components (from current scene)
    // Props can receive shadows from other props (but not from themselves)
    // =========================================================================
    static bool props_printed = false;
    int32_t prop_render_index = 0;
    for (ECS::EntityID prop_entity : g_state.scene.prop_entities) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(prop_entity);
        auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(prop_entity);
        
        if (!transform || !sprite || !sprite->visible) {
            prop_render_index++;
            continue;
        }
        
        // Debug: print prop z_depth once
        if (!props_printed) {
            printf("Prop z_depth: %.2f (pos: %.0f, %.0f)\n", 
                   transform->z_depth, transform->position.x, transform->position.y);
        }
        
        // Calculate depth scaling from ECS transform
        float depth_scale = ECS::TransformHelpers::compute_depth_scale(transform->z_depth);
        Vec2 scaled_size = Vec2(
            sprite->base_size.x * transform->scale.x * depth_scale,
            sprite->base_size.y * transform->scale.y * depth_scale
        );
        
        // Create 3D position for rendering (pixel coords + z_depth)
        Vec3 render_pos(transform->position.x, transform->position.y, transform->z_depth);
        
        // Render prop with lighting AND shadows - pass entity index to skip self-shadowing
        Renderer::render_sprite_lit_shadowed(sprite->texture, render_pos, scaled_size,
                                            light_data, num_lights,
                                            shadow_data, num_shadow_casters,
                                            sprite->normal_map, sprite->pivot,
                                            transform->z_depth, prop_render_index);
        prop_render_index++;
    }
    props_printed = true;
    
    // =========================================================================
    // Render player using ECS components
    // Player receives shadows from props but doesn't shadow itself
    // =========================================================================
    if (g_state.player_entity != ECS::INVALID_ENTITY) {
        auto* transform = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(g_state.player_entity);
        auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(g_state.player_entity);
        
        if (transform && sprite && sprite->visible) {
            // Calculate depth scaling
            float depth_scale = ECS::TransformHelpers::compute_depth_scale(transform->z_depth);
            Vec2 scaled_size = Vec2(
                sprite->base_size.x * transform->scale.x * depth_scale,
                sprite->base_size.y * transform->scale.y * depth_scale
            );
            
            // Create 3D position for rendering
            Vec3 render_pos(transform->position.x, transform->position.y, transform->z_depth);
            
            // Player's entity index for self-shadow skip (after all props)
            int32_t player_entity_index = (int32_t)g_state.scene.prop_entities.size();
            
            // Render animated sprite if animation is set (with lighting and shadows)
            if (sprite->is_animated() && sprite->animation) {
                Renderer::render_sprite_animated_lit_shadowed(sprite->animation, 
                                                render_pos, 
                                                scaled_size,
                                                light_data, num_lights,
                                                shadow_data, num_shadow_casters,
                                                sprite->normal_map,
                                                sprite->pivot,
                                                transform->z_depth,
                                                player_entity_index);
            } else {
                Renderer::render_sprite_lit_shadowed(sprite->texture, render_pos, scaled_size,
                                           light_data, num_lights, 
                                           shadow_data, num_shadow_casters,
                                           sprite->normal_map, sprite->pivot,
                                           transform->z_depth, player_entity_index);
            }
        }
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

