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

// Check if player has reached the end of the edge they're following
// Returns the endpoint if reached, or invalid Vec2 if not
static Vec2 get_reached_edge_endpoint(Vec2 position, const Player& player, float epsilon = 2.0f) {
    // Check distance to both endpoints
    Vec2 to_start = Vec2(player.edge_start.x - position.x, player.edge_start.y - position.y);
    Vec2 to_end = Vec2(player.edge_end.x - position.x, player.edge_end.y - position.y);
    
    float dist_start_sq = to_start.x * to_start.x + to_start.y * to_start.y;
    float dist_end_sq = to_end.x * to_end.x + to_end.y * to_end.y;
    
    float eps_sq = epsilon * epsilon;
    
    // Determine which endpoint we're heading toward based on edge_direction
    Vec2 edge_vec = Vec2(player.edge_end.x - player.edge_start.x, 
                         player.edge_end.y - player.edge_start.y);
    float dot = player.edge_direction.x * edge_vec.x + player.edge_direction.y * edge_vec.y;
    
    // If heading toward end (positive dot)
    if (dot >= 0.0f) {
        if (dist_end_sq <= eps_sq) {
            return player.edge_end;
        }
    } else {
        // Heading toward start
        if (dist_start_sq <= eps_sq) {
            return player.edge_start;
        }
    }
    
    return Vec2(-99999.0f, -99999.0f); // Invalid marker
}

// Check if direct path to target is clear (no collision)
static bool is_direct_path_clear(Vec2 from, Vec2 to, const std::vector<Collision::Polygon>& walkable_areas) {
    if (walkable_areas.empty()) return true;
    
    // FIRST check if starting position is inside walkable area
    // If we're on an edge (outside polygon), path is NOT clear
    if (!Collision::point_in_walkable_area(from, walkable_areas)) {
        return false;
    }
    
    // Check several points along the path
    const int steps = 5;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / (float)steps;
        Vec2 test_pos = Vec2(from.x + t * (to.x - from.x), from.y + t * (to.y - from.y));
        if (!Collision::point_in_walkable_area(test_pos, walkable_areas)) {
            return false;
        }
    }
    return true;
}

// Check if a point is within tolerance of a line segment (edge)
static bool is_point_near_edge(Vec2 point, Vec2 edge_start, Vec2 edge_end, float tolerance = 2.0f) {
    Vec2 edge = Vec2(edge_end.x - edge_start.x, edge_end.y - edge_start.y);
    Vec2 to_point = Vec2(point.x - edge_start.x, point.y - edge_start.y);
    
    float edge_len_sq = edge.x * edge.x + edge.y * edge.y;
    if (edge_len_sq < 0.0001f) return false;
    
    // Project point onto edge line
    float t = (to_point.x * edge.x + to_point.y * edge.y) / edge_len_sq;
    
    // Check if projection is within edge bounds (with some slack)
    if (t < -0.1f || t > 1.1f) return false;
    
    // Calculate distance from point to edge line
    float edge_len = std::sqrt(edge_len_sq);
    Vec2 edge_norm = Vec2(edge.x / edge_len, edge.y / edge_len);
    Vec2 perp = Vec2(-edge_norm.y, edge_norm.x);
    float dist = std::abs(to_point.x * perp.x + to_point.y * perp.y);
    
    return dist <= tolerance;
}

// Simple slide along collision boundary
// Returns the new position after sliding
static Vec2 slide_along_boundary(Vec2 old_pos, Vec2 desired_pos, 
                                  const std::vector<Collision::Polygon>& walkable_areas,
                                  Player& player) {
    // If desired position is valid, just use it and stop edge following
    if (Collision::point_in_walkable_area(desired_pos, walkable_areas)) {
        player.is_edge_following = false;
        return desired_pos;
    }
    
    // If we're already edge-following, try to continue along the current edge
    if (player.is_edge_following) {
        Vec2 movement = Vec2(desired_pos.x - old_pos.x, desired_pos.y - old_pos.y);
        float move_dist = std::sqrt(movement.x * movement.x + movement.y * movement.y);
        float proj = movement.x * player.edge_direction.x + movement.y * player.edge_direction.y;
        
        // Only move if projection is positive (we're heading in the right direction along edge)
        if (proj > 0.01f && move_dist > 0.001f) {
            // Move at full speed along the edge direction
            Vec2 slide_movement = Vec2(player.edge_direction.x * move_dist, 
                                       player.edge_direction.y * move_dist);
            Vec2 new_pos = Vec2(old_pos.x + slide_movement.x, old_pos.y + slide_movement.y);
            
            bool in_walkable = Collision::point_in_walkable_area(new_pos, walkable_areas);
            bool near_edge = is_point_near_edge(new_pos, player.edge_start, player.edge_end, 1.5f);
            
            // Check if new position is either in walkable area OR near the current edge
            if (in_walkable || near_edge) {
                return new_pos;
            }
            
            // Try smaller steps
            for (float scale = 0.75f; scale > 0.1f; scale -= 0.25f) {
                Vec2 test_pos = Vec2(old_pos.x + slide_movement.x * scale, 
                                     old_pos.y + slide_movement.y * scale);
                if (Collision::point_in_walkable_area(test_pos, walkable_areas) ||
                    is_point_near_edge(test_pos, player.edge_start, player.edge_end, 1.5f)) {
                    return test_pos;
                }
            }
        }
        
        // proj is too low or movement failed - behavior depends on edge type
        // For hole edges: keep moving toward vertex to get around the obstacle
        // For outer boundary: stay put, stuck timer will handle timeout
        
        if (!player.is_on_hole_edge) {
            // Outer boundary - just stay in place
            return old_pos;
        }
        
        // Hole edge - keep moving toward end of edge to get around obstacle
        const float vertex_snap_dist = 3.0f;
        Vec2 to_start = Vec2(player.edge_start.x - old_pos.x, player.edge_start.y - old_pos.y);
        Vec2 to_end = Vec2(player.edge_end.x - old_pos.x, player.edge_end.y - old_pos.y);
        float dist_start_sq = to_start.x * to_start.x + to_start.y * to_start.y;
        float dist_end_sq = to_end.x * to_end.x + to_end.y * to_end.y;
        
        // If near vertex, snap to it
        if (dist_start_sq < vertex_snap_dist * vertex_snap_dist) {
            return player.edge_start;
        }
        if (dist_end_sq < vertex_snap_dist * vertex_snap_dist) {
            return player.edge_end;
        }
        
        // Not near vertex - continue moving along edge toward the endpoint we were heading to
        // Use edge_direction which was set when we started following this edge
        if (move_dist > 0.001f) {
            Vec2 continue_movement = Vec2(player.edge_direction.x * move_dist,
                                          player.edge_direction.y * move_dist);
            Vec2 continue_pos = Vec2(old_pos.x + continue_movement.x, old_pos.y + continue_movement.y);
            
            if (Collision::point_in_walkable_area(continue_pos, walkable_areas) ||
                is_point_near_edge(continue_pos, player.edge_start, player.edge_end, 1.5f)) {
                return continue_pos;
            }
            
            // Try smaller steps
            for (float scale = 0.5f; scale > 0.1f; scale -= 0.2f) {
                Vec2 test_pos = Vec2(old_pos.x + continue_movement.x * scale,
                                     old_pos.y + continue_movement.y * scale);
                if (Collision::point_in_walkable_area(test_pos, walkable_areas) ||
                    is_point_near_edge(test_pos, player.edge_start, player.edge_end, 1.5f)) {
                    return test_pos;
                }
            }
        }
        
        return old_pos;
    }
    
    // Not edge-following - find closest edge and start following
    Collision::EdgeInfo edge = Collision::find_closest_edge(desired_pos, walkable_areas);
    if (!edge.valid) {
        return old_pos;
    }
    
    player.is_edge_following = true;
    player.is_on_hole_edge = edge.is_hole;
    player.edge_start = edge.start;
    player.edge_end = edge.end;
    
    // Choose edge direction based on movement direction (not target)
    // This ensures we slide in the direction closest to where we want to go
    Vec2 movement = Vec2(desired_pos.x - old_pos.x, desired_pos.y - old_pos.y);
    float move_dist = std::sqrt(movement.x * movement.x + movement.y * movement.y);
    
    // Calculate edge direction
    Vec2 edge_vec = Vec2(edge.end.x - edge.start.x, edge.end.y - edge.start.y);
    float edge_len = std::sqrt(edge_vec.x * edge_vec.x + edge_vec.y * edge_vec.y);
    if (edge_len < 0.001f) return old_pos;
    
    Vec2 edge_dir = Vec2(edge_vec.x / edge_len, edge_vec.y / edge_len);
    float proj = movement.x * edge_dir.x + movement.y * edge_dir.y;
    
    // Choose direction that aligns with movement
    if (proj >= 0.0f) {
        player.edge_direction = edge_dir;
    } else {
        player.edge_direction = Vec2(-edge_dir.x, -edge_dir.y);
        proj = -proj;
    }
    
    // Now try to slide along this new edge at full speed
    if (proj > 0.0f && move_dist > 0.001f) {
        Vec2 slide_movement = Vec2(player.edge_direction.x * move_dist, 
                                   player.edge_direction.y * move_dist);
        Vec2 new_pos = Vec2(old_pos.x + slide_movement.x, old_pos.y + slide_movement.y);
        
        if (Collision::point_in_walkable_area(new_pos, walkable_areas) ||
            is_point_near_edge(new_pos, player.edge_start, player.edge_end, 1.5f)) {
            return new_pos;
        }
        
        for (float scale = 0.75f; scale > 0.1f; scale -= 0.25f) {
            Vec2 test_pos = Vec2(old_pos.x + slide_movement.x * scale, 
                                 old_pos.y + slide_movement.y * scale);
            if (Collision::point_in_walkable_area(test_pos, walkable_areas) ||
                is_point_near_edge(test_pos, player.edge_start, player.edge_end, 1.5f)) {
                return test_pos;
            }
        }
    }
    
    return old_pos;
}

void player_update(Player& player, ECS::Transform2_5DComponent& transform,
                   uint32_t viewport_width, uint32_t viewport_height, float delta_time) {
    Vec2 to_target = Vec2(
        player.target_position.x - transform.position.x,
        player.target_position.y - transform.position.y
    );
    
    float distance_sq = to_target.x * to_target.x + to_target.y * to_target.y;
    float distance_threshold_sq = player.distance_threshold * player.distance_threshold;
    bool want_to_move = distance_sq > distance_threshold_sq;
    
    // If reached target, stop everything
    if (!want_to_move) {
        player.is_edge_following = false;
    }
    
    // Check if we can stop edge-following (direct path is now clear)
    if (player.is_edge_following && want_to_move) {
        if (is_direct_path_clear(transform.position, 
                                  Vec2(player.target_position.x, player.target_position.y),
                                  g_state.scene.geometry.walkable_areas)) {
            player.is_edge_following = false;
        }
    }
    
    // Animation uses the direction toward target
    AnimationState new_state = want_to_move ? AnimationState::Walking : AnimationState::Idle;
    WalkDirection new_direction = want_to_move ? determine_walk_direction(to_target) : player.walk_direction;
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
    
    if (want_to_move) {
        Vec2 old_pos = transform.position;
        
        // Normalize direction to target
        float dist = std::sqrt(distance_sq);
        Vec2 dir_normalized = Vec2(to_target.x / dist, to_target.y / dist);
        
        // Calculate desired position this frame
        Vec2 movement = Vec2(
            dir_normalized.x * player.speed * delta_time,
            dir_normalized.y * player.speed * delta_time
        );
        
        // Check for overshoot
        float move_dist_sq = movement.x * movement.x + movement.y * movement.y;
        Vec2 desired_pos;
        if (move_dist_sq >= distance_sq) {
            desired_pos = Vec2(player.target_position.x, player.target_position.y);
        } else {
            desired_pos = Vec2(old_pos.x + movement.x, old_pos.y + movement.y);
        }
        
        // Try to move to desired position, with sliding if needed
        Vec2 new_pos = slide_along_boundary(old_pos, desired_pos, 
                                             g_state.scene.geometry.walkable_areas, player);
        
        // Check if we reached edge endpoint while edge-following
        if (player.is_edge_following) {
            Vec2 reached_vertex = get_reached_edge_endpoint(new_pos, player);
            if (reached_vertex.x > -99990.0f) {
                // At edge endpoint - first check if direct path to target is now clear
                Vec2 target_2d = Vec2(player.target_position.x, player.target_position.y);
                if (is_direct_path_clear(reached_vertex, target_2d, g_state.scene.geometry.walkable_areas)) {
                    // Direct path is clear - stop edge following and continue normally
                    player.is_edge_following = false;
                    new_pos = reached_vertex;
                } else {
                    // Find next edge at this vertex
                    Collision::EdgeInfo current_edge;
                    current_edge.start = player.edge_start;
                    current_edge.end = player.edge_end;
                    current_edge.valid = true;
                    
                    Collision::EdgeInfo next_edge = Collision::find_next_edge_at_vertex(
                        current_edge, reached_vertex, target_2d,
                        g_state.scene.geometry.walkable_areas);
                    
                    if (next_edge.valid) {
                        // SNAP player to the vertex position before continuing
                        new_pos = reached_vertex;
                        
                        // Switch to next edge
                        player.edge_start = next_edge.start;
                        player.edge_end = next_edge.end;
                        player.is_on_hole_edge = next_edge.is_hole;
                        
                        // Calculate direction based on which end connects to our vertex
                        Vec2 to_target = Vec2(target_2d.x - reached_vertex.x, target_2d.y - reached_vertex.y);
                        Vec2 edge_vec = Vec2(next_edge.end.x - next_edge.start.x, 
                                             next_edge.end.y - next_edge.start.y);
                        float edge_len = std::sqrt(edge_vec.x * edge_vec.x + edge_vec.y * edge_vec.y);
                        if (edge_len > 0.001f) {
                            Vec2 edge_dir = Vec2(edge_vec.x / edge_len, edge_vec.y / edge_len);
                            // Choose direction that moves toward target
                            float dot = edge_dir.x * to_target.x + edge_dir.y * to_target.y;
                            if (dot >= 0) {
                                player.edge_direction = edge_dir;
                            } else {
                                player.edge_direction = Vec2(-edge_dir.x, -edge_dir.y);
                            }
                        }
                        
                        // Continue moving along the new edge from the vertex
                        float total_move_dist = std::sqrt(move_dist_sq);
                        float remaining = total_move_dist * 0.5f;
                        
                        if (remaining > 0.1f) {
                            Vec2 edge_move = Vec2(
                                player.edge_direction.x * remaining,
                                player.edge_direction.y * remaining
                            );
                            Vec2 continued_pos = Vec2(new_pos.x + edge_move.x, new_pos.y + edge_move.y);
                            
                            bool in_walkable = Collision::point_in_walkable_area(continued_pos, g_state.scene.geometry.walkable_areas);
                            bool near_edge = is_point_near_edge(continued_pos, player.edge_start, player.edge_end, 2.0f);
                            
                            if (in_walkable || near_edge) {
                                new_pos = continued_pos;
                            }
                        }
                    } else {
                        // No next edge found - stop edge following
                        player.is_edge_following = false;
                    }
                }
            }
        }
        
        // Apply new position
        transform.position = new_pos;
        transform.z_depth = Scene::get_z_from_depth_map(g_state.scene, new_pos.x, new_pos.y);
        
        clamp_position(transform.position, player, viewport_width, viewport_height);
        
        // Time-based stuck detection
        Vec2 position_delta = Vec2(
            transform.position.x - player.last_significant_position.x,
            transform.position.y - player.last_significant_position.y
        );
        float movement_from_last_sq = position_delta.x * position_delta.x + position_delta.y * position_delta.y;
        float stuck_threshold_sq = player.stuck_movement_threshold * player.stuck_movement_threshold;
        
        if (movement_from_last_sq >= stuck_threshold_sq) {
            player.stuck_timer = 0.0f;
            player.last_significant_position = transform.position;
        } else {
            player.stuck_timer += delta_time;
            
            if (player.stuck_timer >= player.stuck_timeout) {
                // Stuck too long - cancel movement
                player.target_position = get_position_3d(transform);
                player.is_edge_following = false;
                player.stuck_timer = 0.0f;
            }
        }
    } else {
        player.stuck_timer = 0.0f;
        player.last_significant_position = transform.position;
    }
    
    // Check if player is close enough to active hotspot to trigger callback
    if (player.active_hotspot_index >= 0 && 
        player.active_hotspot_index < (int)g_state.scene.geometry.hotspots.size()) {
        Scene::Hotspot& hotspot = g_state.scene.geometry.hotspots[player.active_hotspot_index];
        
        if (player.hotspot_state == HotspotInteractionState::Approaching &&
            is_player_in_hotspot_range(transform.position, player, hotspot)) {
            if (hotspot.callback) {
                hotspot.callback();
            }
            player.target_position = get_position_3d(transform);
            player.is_edge_following = false;
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
