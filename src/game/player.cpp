#include "player.h"
#include "game.h"
#include "platform.h"
#include "types.h"
#include "config.h"
#include "renderer/texture_loader.h"
#include "renderer/spritesheet_utils.h"
#include "collision/polygon_utils.h"
#include "scene/scene.h"
#include "debug/debug.h"
#include "ecs/ecs.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace Game {

// Helper: Get Vec3 position from Transform (position.xy + z_depth)
static Vec3 get_position_3d(const ECS::Transform2_5DComponent& transform) {
    return Vec3(transform.position.x, transform.position.y, transform.z_depth);
}

// ============================================================================
// Player Entity Creation
// ============================================================================

ECS::EntityID player_create_entity(Player& player, ECS::World& world,
                                   uint32_t base_width, uint32_t base_height) {
    printf("\n[ECS] Creating player entity...\n");
    
    // Create entity
    ECS::EntityID entity = world.create_entity();
    
    // Add Transform2_5D component
    auto& transform = world.add_component<ECS::Transform2_5DComponent>(entity);
    
    // Initialize player - this sets up animations AND initializes transform position
    player_init(player, base_width, base_height, transform);
    
    // Add SpriteComponent
    auto& sprite = world.add_component<ECS::SpriteComponent>(entity);
    sprite.base_size = player.size;
    sprite.pivot = player.pivot;
    sprite.visible = true;
    
    // Add ShadowCasterComponent (player casts shadows)
    auto& shadow = world.add_component<ECS::ShadowCasterComponent>(entity);
    shadow.enabled = true;
    shadow.alpha_threshold = 0.3f;
    shadow.shadow_intensity = 0.7f;
    
    printf("  Player: Entity=%u, pos=(%.0f,%.0f), z=%.2f, size=(%.0f,%.0f), shadow_caster=YES\n",
           entity,
           transform.position.x, transform.position.y, transform.z_depth,
           sprite.base_size.x, sprite.base_size.y);
    
    return entity;
}

// Helper: Set position from Vec3 to Transform
static void set_position_3d(ECS::Transform2_5DComponent& transform, const Vec3& pos) {
    transform.position = Vec2(pos.x, pos.y);
    transform.z_depth = pos.z;
}

// Helper function: Determine walk direction from movement vector
static WalkDirection determine_walk_direction(Vec2 direction) {
    // Normalize to determine dominant direction
    float abs_x = std::abs(direction.x);
    float abs_y = std::abs(direction.y);
    
    // If vertical movement dominates
    if (abs_y > abs_x) {
        return (direction.y > 0) ? WalkDirection::Down : WalkDirection::Up;
    }
    // If horizontal movement dominates (or equal)
    else {
        return (direction.x > 0) ? WalkDirection::Right : WalkDirection::Left;
    }
}

// Helper function: Set player animation state and reset animation if changed
static void set_player_animation_state(Player& player, AnimationState new_state, WalkDirection new_direction = WalkDirection::Down) {
    if (new_state == player.animation_state && new_direction == player.walk_direction) {
        return;  // No change needed
    }
    
    player.animation_state = new_state;
    player.walk_direction = new_direction;
    
    const char* anim_name = nullptr;
    if (new_state == AnimationState::Idle) {
        // Choose idle animation based on direction
        switch (new_direction) {
            case WalkDirection::Down:  anim_name = "idle_down";  break;
            case WalkDirection::Up:    anim_name = "idle_up";    break;
            case WalkDirection::Right: anim_name = "idle_right"; break;
            case WalkDirection::Left:  anim_name = "idle_left";  break;
        }
    } else {
        // Walking - choose walk animation based on direction
        switch (new_direction) {
            case WalkDirection::Down:  anim_name = "walk_down";  break;
            case WalkDirection::Up:    anim_name = "walk_up";    break;
            case WalkDirection::Right: anim_name = "walk_right"; break;
            case WalkDirection::Left:  anim_name = "walk_left";  break;
        }
    }
    
    Renderer::SpriteAnimation* anim = player.animations.get(anim_name);
    if (anim) {
        Renderer::animation_reset(anim);
    }
}

// Helper function: Check if player is within interaction range of hotspot (squared distance to avoid sqrt)
static bool is_player_in_hotspot_range(const Vec2& player_pos, const Player& player, const Scene::Hotspot& hotspot) {
    Vec2 closest = Collision::closest_point_on_polygon(player_pos, hotspot.bounds);
    Vec2 delta = Vec2(closest.x - player_pos.x, closest.y - player_pos.y);
    float dist_sq = delta.x * delta.x + delta.y * delta.y;
    float threshold = hotspot.interaction_distance + player.hotspot_proximity_tolerance;
    return dist_sq <= (threshold * threshold);  // Compare squared to avoid sqrt()
}

// Helper function: Handle hotspot click detection and approach
static void handle_hotspot_click(Player& player, const Vec2& player_pos, Vec2 mouse_pos) {
    player.active_hotspot_index = -1;
    player.hotspot_state = HotspotInteractionState::None;
    
    for (size_t i = 0; i < g_state.scene.geometry.hotspots.size(); ++i) {
        const Scene::Hotspot& hotspot = g_state.scene.geometry.hotspots[i];
        if (!hotspot.enabled) continue;
        
        // Check if click is inside hotspot bounds
        if (Collision::point_in_polygon(mouse_pos, hotspot.bounds)) {
            // Found clicked hotspot
            player.active_hotspot_index = (int)i;
            
            // Check if player is already in interaction distance
            if (is_player_in_hotspot_range(player_pos, player, hotspot)) {
                // Already in range - trigger callback immediately
                if (hotspot.callback) {
                    hotspot.callback();
                }
                float current_z = Scene::get_z_from_depth_map(g_state.scene, player_pos.x, player_pos.y);
                player.target_position = Vec3(player_pos.x, player_pos.y, current_z);
                player.hotspot_state = HotspotInteractionState::InRange;
                return;
            }
            
            // Calculate approach point: move from hotspot outward to interaction distance
            Vec2 closest_on_hotspot = Collision::closest_point_on_polygon(player_pos, hotspot.bounds);
            Vec2 to_player = Vec2(
                player_pos.x - closest_on_hotspot.x,
                player_pos.y - closest_on_hotspot.y
            );
            float dist_to_player_sq = to_player.x * to_player.x + to_player.y * to_player.y;
            float direction_threshold_sq = player.direction_normalization_threshold * player.direction_normalization_threshold;
            
            // Normalize direction vector (using squared comparison to avoid sqrt when possible)
            if (dist_to_player_sq > direction_threshold_sq) {
                float dist_to_player = std::sqrt(dist_to_player_sq);
                to_player.x /= dist_to_player;
                to_player.y /= dist_to_player;
            } else {
                to_player = Vec2(0.0f, 1.0f);  // Default: downward
            }
            
            // Set target at interaction_distance away from hotspot boundary
            Vec2 approach_point_2d = Vec2(
                closest_on_hotspot.x + to_player.x * hotspot.interaction_distance,
                closest_on_hotspot.y + to_player.y * hotspot.interaction_distance
            );
            
            // Sample Z from depth map based on world position
            float target_z = Scene::get_z_from_depth_map(g_state.scene, approach_point_2d.x, approach_point_2d.y);
            player.target_position = Vec3(approach_point_2d.x, approach_point_2d.y, target_z);
            player.hotspot_state = HotspotInteractionState::Approaching;
            return;
        }
    }
}

// Helper function: Handle regular movement click
static void handle_movement_click(Player& player, Vec2 mouse_pos) {
    // Sample Z from depth map based on world position of click
    float target_z = Scene::get_z_from_depth_map(g_state.scene, mouse_pos.x, mouse_pos.y);
    player.target_position = Vec3(mouse_pos.x, mouse_pos.y, target_z);
    player.active_hotspot_index = -1;
    player.hotspot_state = HotspotInteractionState::None;
}

void player_init(Player& player, uint32_t viewport_width, uint32_t viewport_height, 
                 ECS::Transform2_5DComponent& transform) {
    // Load testa spritesheet
    // Frame dimensions: 104x130 pixels
    // Row 0 (y=0): Idle (1 frame)
    // Row 1 (y=130): Walk Left (8 frames) - mirror for Walk Right
    // Row 2 (y=260): Walk Up (6 frames)
    // Row 3 (y=390): Walk Down (6 frames)
    // Sheet size: 832x520 (8 frames wide, 4 rows)
    Renderer::TextureID testa_sprite = Renderer::load_texture("player/testa_spritesheet.png");
    
    const float frame_w = 104.0f;
    const float frame_h = 130.0f;
    const float sheet_w = 832.0f;
    const float sheet_h = 520.0f;
    
    // Helper to create UV frame (normal orientation)
    auto create_uv = [&](float x, float y) -> Renderer::SpriteFrame {
        return { 
            x / sheet_w, 
            y / sheet_h, 
            (x + frame_w) / sheet_w, 
            (y + frame_h) / sheet_h
        };
    };
    
    // Helper to create mirrored UV frame (flip horizontally)
    auto create_uv_mirrored = [&](float x, float y) -> Renderer::SpriteFrame {
        return { 
            (x + frame_w) / sheet_w,  // u0 = right edge
            y / sheet_h, 
            x / sheet_w,              // u1 = left edge
            (y + frame_h) / sheet_h
        };
    };
    
    // Idle animation - Row 0, single frame (used for all idle directions)
    Renderer::SpriteAnimation idle_anim;
    idle_anim.texture = testa_sprite;
    idle_anim.frames.push_back(create_uv(0.0f, 0.0f));
    idle_anim.frame_duration = 1.0f;
    idle_anim.elapsed_time = 0.0f;
    idle_anim.current_frame = 0;
    
    // Walk Left: Row 1 (y=130) - 8 frames
    Renderer::SpriteAnimation walk_left_anim;
    walk_left_anim.texture = testa_sprite;
    for (int i = 0; i < 8; i++) {
        walk_left_anim.frames.push_back(create_uv(i * frame_w, 130.0f));
    }
    walk_left_anim.frame_duration = 0.1f;
    walk_left_anim.elapsed_time = 0.0f;
    walk_left_anim.current_frame = 0;
    
    // Walk Right: Row 1 mirrored (same frames as walk_left but flipped)
    Renderer::SpriteAnimation walk_right_anim;
    walk_right_anim.texture = testa_sprite;
    for (int i = 0; i < 8; i++) {
        walk_right_anim.frames.push_back(create_uv_mirrored(i * frame_w, 130.0f));
    }
    walk_right_anim.frame_duration = 0.1f;
    walk_right_anim.elapsed_time = 0.0f;
    walk_right_anim.current_frame = 0;
    
    // Walk Up: Row 2 (y=260) - 6 frames
    Renderer::SpriteAnimation walk_up_anim;
    walk_up_anim.texture = testa_sprite;
    for (int i = 0; i < 6; i++) {
        walk_up_anim.frames.push_back(create_uv(i * frame_w, 260.0f));
    }
    walk_up_anim.frame_duration = 0.1f;
    walk_up_anim.elapsed_time = 0.0f;
    walk_up_anim.current_frame = 0;
    
    // Walk Down: Row 3 (y=390) - 6 frames
    Renderer::SpriteAnimation walk_down_anim;
    walk_down_anim.texture = testa_sprite;
    for (int i = 0; i < 6; i++) {
        walk_down_anim.frames.push_back(create_uv(i * frame_w, 390.0f));
    }
    walk_down_anim.frame_duration = 0.1f;
    walk_down_anim.elapsed_time = 0.0f;
    walk_down_anim.current_frame = 0;
    
    // Add all animations to the bank
    // Use same idle for all directions (single idle frame)
    player.animations.add("idle_down", idle_anim);
    player.animations.add("idle_right", idle_anim);
    player.animations.add("idle_up", idle_anim);
    player.animations.add("idle_left", idle_anim);
    player.animations.add("walk_down", walk_down_anim);
    player.animations.add("walk_up", walk_up_anim);
    player.animations.add("walk_right", walk_right_anim);
    player.animations.add("walk_left", walk_left_anim);
    
    // Also add generic "idle" that uses current direction
    player.animations.add("idle", idle_anim);
    
    // Update player size to match new sprite dimensions (scaled down for base resolution)
    player.size = Vec2(52.0f, 65.0f);  // Half of 104x130
    
    // Initialize player position via ECS Transform
    transform.position = Vec2(viewport_width * 0.5f, viewport_height * 0.5f);
    // Calculate Z from depth map based on initial XY coordinates
    transform.z_depth = Scene::get_z_from_depth_map(g_state.scene, transform.position.x, transform.position.y);
    
    // Initialize target to current position
    player.target_position = Vec3(transform.position.x, transform.position.y, transform.z_depth);
    // player.size comes from struct default
    player.animation_state = AnimationState::Idle;
    player.walk_direction = WalkDirection::Down;
}

void player_handle_input(Player& player, ECS::Transform2_5DComponent& transform) {
    // Track previous mouse state for release detection
    static bool s_was_mouse_down = false;
    bool is_mouse_down = Platform::mouse_down();
    
    // Handle right-click (delete vertex in editor)
    if (Platform::mouse_right_clicked()) {
        Vec2 mouse_viewport = Platform::get_mouse_pos();
        if (Debug::handle_mouse_right_click(mouse_viewport)) {
            // Right-click consumed by editor
        }
    }
    
    if (Platform::mouse_clicked()) {
        Vec2 mouse_viewport = Platform::get_mouse_pos();
        
        // Check if debug overlay wants to consume the click
        if (Debug::handle_mouse_click(mouse_viewport)) {
            s_was_mouse_down = is_mouse_down;
            return;  // Click was consumed by geometry editor
        }
        
        // Scale mouse position from viewport (1280x720) to base resolution (320x180)
        Vec2 mouse_pos = Vec2(
            mouse_viewport.x * (float)Config::BASE_WIDTH / (float)Config::VIEWPORT_WIDTH,
            mouse_viewport.y * (float)Config::BASE_HEIGHT / (float)Config::VIEWPORT_HEIGHT
        );
        handle_hotspot_click(player, transform.position, mouse_pos);
        
        // If no hotspot was clicked, handle regular movement
        if (player.active_hotspot_index == -1) {
            handle_movement_click(player, mouse_pos);
        }
    }
    
    // Handle mouse release for geometry editor (detect transition from down to up)
    if (s_was_mouse_down && !is_mouse_down) {
        Debug::handle_mouse_release();
    }
    s_was_mouse_down = is_mouse_down;
}

static void clamp_position(Vec2& position, const Player& player, uint32_t viewport_width, uint32_t viewport_height) {
    const float left = player.boundary_margin;
    const float right = viewport_width - player.boundary_margin;
    const float top = player.boundary_margin;
    const float bottom = viewport_height - player.boundary_margin;
    
    position.x = std::max(left, std::min(right, position.x));
    position.y = std::max(top, std::min(bottom, position.y));
}

static void apply_collision_response(Vec2& position, float& z_depth, Vec3 old_position, const std::vector<Collision::Polygon>& walkable_areas, const Scene::Scene& scene) {
    if (walkable_areas.empty()) return;
    
    // If player is inside walkable areas (and not in a hole), no collision
    if (Collision::point_in_walkable_area(position, walkable_areas)) {
        return;
    }
    
    // Collision detected - try wall sliding
    // Try X-axis only movement
    Vec2 x_only = Vec2(position.x, old_position.y);
    if (Collision::point_in_walkable_area(x_only, walkable_areas)) {
        position = x_only;
        z_depth = Scene::get_z_from_depth_map(scene, x_only.x, x_only.y);
        return;
    }
    
    // Try Y-axis only movement
    Vec2 y_only = Vec2(old_position.x, position.y);
    if (Collision::point_in_walkable_area(y_only, walkable_areas)) {
        position = y_only;
        z_depth = Scene::get_z_from_depth_map(scene, y_only.x, y_only.y);
        return;
    }
    
    // Both blocked - revert to old position (full stop)
    position = Vec2(old_position.x, old_position.y);
    z_depth = old_position.z;
}

void player_update(Player& player, ECS::Transform2_5DComponent& transform,
                   uint32_t viewport_width, uint32_t viewport_height, float delta_time) {
    // Save old position for collision response (as Vec3: xy + z)
    Vec3 old_position = get_position_3d(transform);
    
    Vec2 direction = Vec2(
        player.target_position.x - transform.position.x,
        player.target_position.y - transform.position.y
    );
    
    float distance_sq = direction.x * direction.x + direction.y * direction.y;
    float distance_threshold_sq = player.distance_threshold * player.distance_threshold;
    bool is_moving = distance_sq > distance_threshold_sq;  // Compare squared to avoid sqrt
    
    // Determine animation state and direction
    AnimationState new_state = is_moving ? AnimationState::Walking : AnimationState::Idle;
    WalkDirection new_direction = is_moving ? determine_walk_direction(direction) : player.walk_direction;
    set_player_animation_state(player, new_state, new_direction);
    
    // Get current animation based on state and direction
    const char* anim_name = nullptr;
    if (player.animation_state == AnimationState::Idle) {
        switch (player.walk_direction) {
            case WalkDirection::Down:  anim_name = "idle_down";  break;
            case WalkDirection::Up:    anim_name = "idle_up";    break;
            case WalkDirection::Right: anim_name = "idle_right"; break;
            case WalkDirection::Left:  anim_name = "idle_left";  break;
        }
    } else {
        switch (player.walk_direction) {
            case WalkDirection::Down:  anim_name = "walk_down";  break;
            case WalkDirection::Up:    anim_name = "walk_up";    break;
            case WalkDirection::Right: anim_name = "walk_right"; break;
            case WalkDirection::Left:  anim_name = "walk_left";  break;
        }
    }
    
    Renderer::SpriteAnimation* current_anim = player.animations.get(anim_name);
    if (current_anim) {
        Renderer::animate(current_anim, delta_time);
    }
    
    if (is_moving) {
        float distance = std::sqrt(distance_sq);
        direction.x /= distance;
        direction.y /= distance;
        
        Vec2 movement = Vec2(
            direction.x * player.speed * delta_time,
            direction.y * player.speed * delta_time
        );
        
        // Check if movement would overshoot the target (using squared comparison)
        float move_distance_sq = movement.x * movement.x + movement.y * movement.y;
        if (move_distance_sq >= distance_sq) {
            // Snap to target to avoid oscillation
            set_position_3d(transform, player.target_position);
        } else {
            transform.position.x += movement.x;
            transform.position.y += movement.y;
            // Update Z continuously based on position during movement
            transform.z_depth = Scene::get_z_from_depth_map(g_state.scene, transform.position.x, transform.position.y);
        }
        
        clamp_position(transform.position, player, viewport_width, viewport_height);
        apply_collision_response(transform.position, transform.z_depth, old_position, 
                                g_state.scene.geometry.walkable_areas, g_state.scene);
        
        // Check if player actually moved (not stuck against walls) - using squared comparison
        Vec2 position_delta = Vec2(
            transform.position.x - old_position.x,
            transform.position.y - old_position.y
        );
        float actual_movement_sq = position_delta.x * position_delta.x + position_delta.y * position_delta.y;
        float stuck_threshold_sq = player.stuck_movement_threshold * player.stuck_movement_threshold;
        
        // If stuck (moved less than threshold this frame), stop movement
        if (actual_movement_sq < stuck_threshold_sq) {
            // Stop trying to move - give up on this target
            // Animation will switch to Idle in the next frame when distance becomes 0
            player.target_position = get_position_3d(transform);
        }
    }
    
    // Check if player is close enough to active hotspot to trigger callback
    if (player.active_hotspot_index >= 0 && 
        player.active_hotspot_index < (int)g_state.scene.geometry.hotspots.size()) {
        Scene::Hotspot& hotspot = g_state.scene.geometry.hotspots[player.active_hotspot_index];
        
        // Only trigger callback when transitioning from Approaching to InRange (not if already InRange)
        if (player.hotspot_state == HotspotInteractionState::Approaching &&
            is_player_in_hotspot_range(transform.position, player, hotspot)) {
            // Player reached the hotspot - trigger callback
            if (hotspot.callback) {
                hotspot.callback();
            }
            // Stop all movement
            player.target_position = get_position_3d(transform);
            player.active_hotspot_index = -1;
            player.hotspot_state = HotspotInteractionState::InRange;
        }
    }
}

const char* player_get_animation_name(const Player& player) {
    if (player.animation_state == AnimationState::Idle) {
        switch (player.walk_direction) {
            case WalkDirection::Down:  return "idle_down";
            case WalkDirection::Up:    return "idle_up";
            case WalkDirection::Right: return "idle_right";
            case WalkDirection::Left:  return "idle_left";
        }
    } else {  // Walking
        switch (player.walk_direction) {
            case WalkDirection::Down:  return "walk_down";
            case WalkDirection::Up:    return "walk_up";
            case WalkDirection::Right: return "walk_right";
            case WalkDirection::Left:  return "walk_left";
        }
    }
    return "idle_down";  // Fallback
}

}
