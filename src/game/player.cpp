#include "player.h"
#include "game.h"
#include "platform.h"
#include "types.h"
#include "config.h"
#include "renderer/texture_loader.h"
#include "renderer/spritesheet_utils.h"
#include "collision/polygon_utils.h"
#include "scene/scene.h"
#include <cmath>
#include <algorithm>

namespace Game {

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
    if (!player.animations) return;
    
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
    
    Renderer::SpriteAnimation* anim = player.animations->get(anim_name);
    if (anim) {
        Renderer::animation_reset(anim);
    }
}

// Helper function: Check if player is within interaction range of hotspot (squared distance to avoid sqrt)
static bool is_player_in_hotspot_range(const Player& player, const Scene::Hotspot& hotspot) {
    Vec2 closest = Collision::closest_point_on_polygon(player.position, hotspot.bounds);
    Vec2 delta = Vec2(closest.x - player.position.x, closest.y - player.position.y);
    float dist_sq = delta.x * delta.x + delta.y * delta.y;
    float threshold = hotspot.interaction_distance + player.hotspot_proximity_tolerance;
    return dist_sq <= (threshold * threshold);  // Compare squared to avoid sqrt()
}

// Helper function: Handle hotspot click detection and approach
static void handle_hotspot_click(Player& player, Vec2 mouse_pos) {
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
            if (is_player_in_hotspot_range(player, hotspot)) {
                // Already in range - trigger callback immediately
                if (hotspot.callback) {
                    hotspot.callback();
                }
                player.target_position = player.position;
                player.hotspot_state = HotspotInteractionState::InRange;
                return;
            }
            
            // Calculate approach point: move from hotspot outward to interaction distance
            Vec2 closest_on_hotspot = Collision::closest_point_on_polygon(player.position, hotspot.bounds);
            Vec2 to_player = Vec2(
                player.position.x - closest_on_hotspot.x,
                player.position.y - closest_on_hotspot.y
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
                 Core::AnimationBank* animations) {
    // Load Lenore spritesheet (288x920)
    // Frame dimensions: 36x46 pixels
    // Row 0 (y=0): Idle sprites (Down, Right, Up, Left) - 4 sprites
    // Row 1 (y=46): Walk Down (8 frames)
    // Row 2 (y=92): Walk Up (8 frames)
    // Row 3 (y=138): Walk Right (8 frames)
    // Row 4 (y=184): Walk Left (8 frames)
    Renderer::TextureID lenore_sprite = Renderer::load_texture("player/Lenore-Sheet.png");
    
    // Helper to create UV frame with small offset to prevent texture bleeding
    auto create_uv = [](float x, float y, float w, float h, float sheet_w, float sheet_h) -> Renderer::SpriteFrame {
        return { 
            x / sheet_w, 
            y / sheet_h, 
            (x + w) / sheet_w, 
            (y + h) / sheet_h
        };
    };
    
    // Idle animations - one frame each for each direction
    // Idle Down (frame 0 of row 0)
    Renderer::SpriteAnimation idle_down_anim;
    idle_down_anim.texture = lenore_sprite;
    idle_down_anim.frames.push_back(create_uv(0.0f, 0.0f, 36.0f, 46.0f, 288.0f, 920.0f));
    idle_down_anim.frame_duration = 1.0f;
    idle_down_anim.elapsed_time = 0.0f;
    idle_down_anim.current_frame = 0;
    
    // Idle Right (frame 1 of row 0)
    Renderer::SpriteAnimation idle_right_anim;
    idle_right_anim.texture = lenore_sprite;
    idle_right_anim.frames.push_back(create_uv(36.0f, 0.0f, 36.0f, 46.0f, 288.0f, 920.0f));
    idle_right_anim.frame_duration = 1.0f;
    idle_right_anim.elapsed_time = 0.0f;
    idle_right_anim.current_frame = 0;
    
    // Idle Up (frame 2 of row 0)
    Renderer::SpriteAnimation idle_up_anim;
    idle_up_anim.texture = lenore_sprite;
    idle_up_anim.frames.push_back(create_uv(72.0f, 0.0f, 36.0f, 46.0f, 288.0f, 920.0f));
    idle_up_anim.frame_duration = 1.0f;
    idle_up_anim.elapsed_time = 0.0f;
    idle_up_anim.current_frame = 0;
    
    // Idle Left (frame 3 of row 0)
    Renderer::SpriteAnimation idle_left_anim;
    idle_left_anim.texture = lenore_sprite;
    idle_left_anim.frames.push_back(create_uv(108.0f, 0.0f, 36.0f, 46.0f, 288.0f, 920.0f));
    idle_left_anim.frame_duration = 1.0f;
    idle_left_anim.elapsed_time = 0.0f;
    idle_left_anim.current_frame = 0;
    
    // Walk Down: Row 1 (y=46) - frames 0-7
    Renderer::SpriteAnimation walk_down_anim;
    walk_down_anim.texture = lenore_sprite;
    for (int i = 0; i < 8; i++) {
        walk_down_anim.frames.push_back(create_uv(i * 36.0f, 46.0f, 36.0f, 46.0f, 288.0f, 920.0f));
    }
    walk_down_anim.frame_duration = 0.12f;
    walk_down_anim.elapsed_time = 0.0f;
    walk_down_anim.current_frame = 0;
    
    // Walk Up: Row 2 (y=92) - frames 0-7
    Renderer::SpriteAnimation walk_up_anim;
    walk_up_anim.texture = lenore_sprite;
    for (int i = 0; i < 8; i++) {
        walk_up_anim.frames.push_back(create_uv(i * 36.0f, 92.0f, 36.0f, 46.0f, 288.0f, 920.0f));
    }
    walk_up_anim.frame_duration = 0.12f;
    walk_up_anim.elapsed_time = 0.0f;
    walk_up_anim.current_frame = 0;
    
    // Walk Right: Row 3 (y=138) - frames 0-7
    Renderer::SpriteAnimation walk_right_anim;
    walk_right_anim.texture = lenore_sprite;
    for (int i = 0; i < 8; i++) {
        walk_right_anim.frames.push_back(create_uv(i * 36.0f, 138.0f, 36.0f, 46.0f, 288.0f, 920.0f));
    }
    walk_right_anim.frame_duration = 0.12f;
    walk_right_anim.elapsed_time = 0.0f;
    walk_right_anim.current_frame = 0;
    
    // Walk Left: Row 4 (y=184) - frames 0-7
    Renderer::SpriteAnimation walk_left_anim;
    walk_left_anim.texture = lenore_sprite;
    for (int i = 0; i < 8; i++) {
        walk_left_anim.frames.push_back(create_uv(i * 36.0f, 184.0f, 36.0f, 46.0f, 288.0f, 920.0f));
    }
    walk_left_anim.frame_duration = 0.12f;
    walk_left_anim.elapsed_time = 0.0f;
    walk_left_anim.current_frame = 0;
    
    // Add all animations to the bank
    animations->add("idle_down", idle_down_anim);
    animations->add("idle_right", idle_right_anim);
    animations->add("idle_up", idle_up_anim);
    animations->add("idle_left", idle_left_anim);
    animations->add("walk_down", walk_down_anim);
    animations->add("walk_up", walk_up_anim);
    animations->add("walk_right", walk_right_anim);
    animations->add("walk_left", walk_left_anim);
    
    // Also add generic "idle" that uses current direction
    animations->add("idle", idle_down_anim);
    
    // Initialize player entity
    player.position = Vec3(viewport_width * 0.5f, viewport_height * 0.5f, 0.0f);
    player.target_position = Vec3(viewport_width * 0.5f, viewport_height * 0.5f, 0.0f);
    // Calculate player Z position from depth map based on initial XY coordinates
    player.position.z = Scene::get_z_from_depth_map(g_state.scene, player.position.x, player.position.y);
    player.target_position.z = player.position.z;
    // player.size comes from struct default (100x150)
    player.animation_state = AnimationState::Idle;
    player.walk_direction = WalkDirection::Down;
    player.animations = animations;
}

void player_handle_input(Player& player) {
    if (Platform::mouse_clicked()) {
        Vec2 mouse_viewport = Platform::get_mouse_pos();
        // Scale mouse position from viewport (1280x720) to base resolution (320x180)
        Vec2 mouse_pos = Vec2(
            mouse_viewport.x * (float)Config::BASE_WIDTH / (float)Config::VIEWPORT_WIDTH,
            mouse_viewport.y * (float)Config::BASE_HEIGHT / (float)Config::VIEWPORT_HEIGHT
        );
        handle_hotspot_click(player, mouse_pos);
        
        // If no hotspot was clicked, handle regular movement
        if (player.active_hotspot_index == -1) {
            handle_movement_click(player, mouse_pos);
        }
    }
}

static void clamp_player_position(Player& player, uint32_t viewport_width, uint32_t viewport_height) {
    const float left = player.boundary_margin;
    const float right = viewport_width - player.boundary_margin;
    const float top = player.boundary_margin;
    const float bottom = viewport_height - player.boundary_margin;
    
    player.position.x = std::max(left, std::min(right, player.position.x));
    player.position.y = std::max(top, std::min(bottom, player.position.y));
}

static void apply_collision_response(Player& player, Vec3 old_position, const std::vector<Collision::Polygon>& walkable_areas, const Scene::Scene& scene) {
    if (walkable_areas.empty()) return;
    
    // If player is inside walkable areas, no collision
    if (Collision::point_in_any_polygon(player.position, walkable_areas)) {
        return;
    }
    
    // Collision detected - try wall sliding
    // Try X-axis only movement
    Vec2 x_only = Vec2(player.position.x, old_position.y);
    if (Collision::point_in_any_polygon(x_only, walkable_areas)) {
        player.position = Vec3(x_only.x, x_only.y, Scene::get_z_from_depth_map(scene, x_only.x, x_only.y));
        return;
    }
    
    // Try Y-axis only movement
    Vec2 y_only = Vec2(old_position.x, player.position.y);
    if (Collision::point_in_any_polygon(y_only, walkable_areas)) {
        player.position = Vec3(y_only.x, y_only.y, Scene::get_z_from_depth_map(scene, y_only.x, y_only.y));
        return;
    }
    
    // Both blocked - revert to old position (full stop)
    player.position = old_position;
}

void player_update(Player& player, uint32_t viewport_width, uint32_t viewport_height, float delta_time) {
    Vec3 old_position = player.position;  // Save for collision response (includes z)
    
    Vec2 direction = Vec2(
        player.target_position.x - player.position.x,
        player.target_position.y - player.position.y
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
    
    Renderer::SpriteAnimation* current_anim = 
        (player.animations) ? player.animations->get(anim_name) : nullptr;
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
            player.position = player.target_position;
        } else {
            player.position.x += movement.x;
            player.position.y += movement.y;
            // Update Z continuously based on Y position during movement
            player.position.z = Scene::get_z_from_depth_map(g_state.scene, player.position.x, player.position.y);
        }
        
        clamp_player_position(player, viewport_width, viewport_height);
        apply_collision_response(player, old_position, g_state.scene.geometry.walkable_areas, g_state.scene);
        
        // Check if player actually moved (not stuck against walls) - using squared comparison
        Vec2 position_delta = Vec2(
            player.position.x - old_position.x,
            player.position.y - old_position.y
        );
        float actual_movement_sq = position_delta.x * position_delta.x + position_delta.y * position_delta.y;
        float stuck_threshold_sq = player.stuck_movement_threshold * player.stuck_movement_threshold;
        
        // If stuck (moved less than threshold this frame), stop movement
        if (actual_movement_sq < stuck_threshold_sq) {
            // Stop trying to move - give up on this target
            // Animation will switch to Idle in the next frame when distance becomes 0
            player.target_position = player.position;
        }
    }
    
    // Check if player is close enough to active hotspot to trigger callback
    if (player.active_hotspot_index >= 0 && 
        player.active_hotspot_index < (int)g_state.scene.geometry.hotspots.size()) {
        Scene::Hotspot& hotspot = g_state.scene.geometry.hotspots[player.active_hotspot_index];
        
        // Only trigger callback when transitioning from Approaching to InRange (not if already InRange)
        if (player.hotspot_state == HotspotInteractionState::Approaching &&
            is_player_in_hotspot_range(player, hotspot)) {
            // Player reached the hotspot - trigger callback
            if (hotspot.callback) {
                hotspot.callback();
            }
            // Stop all movement
            player.target_position = player.position;
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
