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

// Helper function: Set player animation state and reset animation if changed
static void set_player_animation_state(Player& player, AnimationState new_state) {
    if (new_state == player.animation_state) return;  // No change needed
    
    player.animation_state = new_state;
    if (!player.animations) return;
    
    const char* anim_name = (new_state == AnimationState::Idle) ? "idle" : "walk";
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
            Vec2 approach_point = Vec2(
                closest_on_hotspot.x + to_player.x * hotspot.interaction_distance,
                closest_on_hotspot.y + to_player.y * hotspot.interaction_distance
            );
            
            player.target_position = approach_point;
            player.hotspot_state = HotspotInteractionState::Approaching;
            return;
        }
    }
}

// Helper function: Handle regular movement click
static void handle_movement_click(Player& player, Vec2 mouse_pos) {
    player.target_position = mouse_pos;
    player.active_hotspot_index = -1;
    player.hotspot_state = HotspotInteractionState::None;
}

void player_init(Player& player, uint32_t viewport_width, uint32_t viewport_height, 
                 Core::AnimationBank* animations) {
    // Load player spritesheet
    Renderer::TextureID player_sprite_map = Renderer::load_texture("player/player_spritesheet.png");
    
    // Calculate UV coordinates for all 4 frames in a 4-column, 1-row grid
    auto uv_frames = Renderer::create_uv_grid(4, 1);
    
    // Idle animation - frame 0 only
    Renderer::SpriteAnimation idle_anim;
    idle_anim.texture = player_sprite_map;
    idle_anim.frames = { uv_frames[0] };
    idle_anim.frame_duration = 1.0f;
    idle_anim.elapsed_time = 0.0f;
    idle_anim.current_frame = 0;
    animations->add("idle", idle_anim);
    
    // Walk animation - frames 1, 2, 3
    Renderer::SpriteAnimation walk_anim;
    walk_anim.texture = player_sprite_map;
    walk_anim.frames = { uv_frames[1], uv_frames[2], uv_frames[3] };
    walk_anim.frame_duration = 0.2f;
    walk_anim.elapsed_time = 0.0f;
    walk_anim.current_frame = 0;
    animations->add("walk", walk_anim);
    
    // Initialize player entity
    player.position = Vec2(viewport_width * 0.5f, viewport_height * 0.5f);
    player.target_position = Vec2(viewport_width * 0.5f, viewport_height * 0.5f);
    player.animation_state = AnimationState::Idle;
    player.animations = animations;
}

void player_handle_input(Player& player) {
    if (Platform::mouse_clicked()) {
        Vec2 mouse_pos = Platform::get_mouse_pos();
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

static void apply_collision_response(Player& player, Vec2 old_position, const std::vector<Collision::Polygon>& walkable_areas) {
    if (walkable_areas.empty()) return;
    
    // If player is inside walkable areas, no collision
    if (Collision::point_in_any_polygon(player.position, walkable_areas)) {
        return;
    }
    
    // Collision detected - try wall sliding
    // Try X-axis only movement
    Vec2 x_only = Vec2(player.position.x, old_position.y);
    if (Collision::point_in_any_polygon(x_only, walkable_areas)) {
        player.position = x_only;
        return;
    }
    
    // Try Y-axis only movement
    Vec2 y_only = Vec2(old_position.x, player.position.y);
    if (Collision::point_in_any_polygon(y_only, walkable_areas)) {
        player.position = y_only;
        return;
    }
    
    // Both blocked - revert to old position (full stop)
    player.position = old_position;
}

void player_update(Player& player, uint32_t viewport_width, uint32_t viewport_height, float delta_time) {
    Vec2 old_position = player.position;  // Save for collision response
    
    Vec2 direction = Vec2(
        player.target_position.x - player.position.x,
        player.target_position.y - player.position.y
    );
    
    float distance_sq = direction.x * direction.x + direction.y * direction.y;
    float distance_threshold_sq = player.distance_threshold * player.distance_threshold;
    bool is_moving = distance_sq > distance_threshold_sq;  // Compare squared to avoid sqrt
    
    AnimationState new_state = is_moving ? AnimationState::Walking : AnimationState::Idle;
    set_player_animation_state(player, new_state);
    
    const char* anim_name = (player.animation_state == AnimationState::Idle) ? "idle" : "walk";
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
        }
        
        clamp_player_position(player, viewport_width, viewport_height);
        apply_collision_response(player, old_position, g_state.scene.geometry.walkable_areas);
        
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

}
