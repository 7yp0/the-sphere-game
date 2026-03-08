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
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstdio>

namespace Game {

GameState g_state;

static void init_player() {
    player_init(g_state.player, g_state.viewport_width, g_state.viewport_height, 
                &g_state.playerAnimations);
}

void init() {
    Scene::init_scene_test();
    init_player();
}

void set_viewport(uint32_t width, uint32_t height) {
    g_state.viewport_width = width;
    g_state.viewport_height = height;
    
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
    player_update(g_state.player, g_state.viewport_width, g_state.viewport_height, delta_time);
}

void render() {
    // Background fills entire scene
    Renderer::render_sprite(g_state.scene.background, 
                           Vec2(0.0f, 0.0f),
                           Vec2((float)g_state.scene.width, (float)g_state.scene.height),
                           Layers::get_z_depth(Layer::BACKGROUND));
    
    // Helper: Calculate visual base Y position (bottom of sprite for sorting)
    auto get_sort_y = [](Vec2 pos, Vec2 size, PivotPoint pivot) -> float {
        switch (pivot) {
            case PivotPoint::TOP_LEFT:
            case PivotPoint::TOP_CENTER:
            case PivotPoint::TOP_RIGHT:
                return pos.y + size.y;
            case PivotPoint::CENTER_LEFT:
            case PivotPoint::CENTER:
            case PivotPoint::CENTER_RIGHT:
                return pos.y + size.y * 0.5f;
            case PivotPoint::BOTTOM_LEFT:
            case PivotPoint::BOTTOM_CENTER:
            case PivotPoint::BOTTOM_RIGHT:
                return pos.y;
            default:
                return pos.y;
        }
    };
    
    // Collect all renderable entities for Y-sorting
    struct Renderable {
        float sort_y;          // Sort key (visual base of sprite)
        bool is_player;        // true = player, false = prop
        size_t prop_index;     // Only valid if !is_player
        
        Renderable(float y, bool player, size_t idx) 
            : sort_y(y), is_player(player), prop_index(idx) {}
    };
    std::vector<Renderable> renderables;
    
    // Add all props with their visual base Y
    for (size_t i = 0; i < g_state.scene.props.size(); ++i) {
        const Scene::Prop& prop = g_state.scene.props[i];
        float sort_y = get_sort_y(prop.position, prop.size, prop.pivot);
        renderables.push_back(Renderable(sort_y, false, i));
    }
    
    // Add player with their visual base Y
    float player_sort_y = get_sort_y(g_state.player.position, g_state.player.size, g_state.player.pivot);
    renderables.push_back(Renderable(player_sort_y, true, 0));
    
    // Sort by visual base Y position (descending: larger base means closer to viewer)
    for (size_t i = 0; i < renderables.size(); ++i) {
        for (size_t j = i + 1; j < renderables.size(); ++j) {
            if (renderables[j].sort_y > renderables[i].sort_y) {  // If j is larger (closer)
                // Swap
                Renderable temp = renderables[i];
                renderables[i] = renderables[j];
                renderables[j] = temp;
            }
        }
    }
    
    // Render in sorted order with proper z-depth
    for (size_t i = 0; i < renderables.size(); ++i) {
        const Renderable& entity = renderables[i];
        
        // Calculate z_depth based on sort order (earlier = further back)
        // Map sort index to z-depth range (-1 to 1)
        float z_depth = -1.0f + (2.0f * i / (renderables.size() - 1.0f + 0.001f));
        
        if (entity.is_player) {
            // Player - render with depth-based scaling for 2.5D effect
            const char* anim_name = player_get_animation_name(g_state.player);
            Renderer::SpriteAnimation* player_anim = g_state.playerAnimations.get(anim_name);
            if (player_anim) {
                // Calculate depth scaling (uses height map if available, falls back to horizon lines)
                float depth_scale = Scene::get_depth_scaling(g_state.scene, g_state.player.position.y);
                Vec2 scaled_size = Vec2(
                    g_state.player.size.x * depth_scale,
                    g_state.player.size.y * depth_scale
                );
                
                Renderer::render_sprite_animated(player_anim, 
                                                g_state.player.position, 
                                                scaled_size,
                                                z_depth,
                                                g_state.player.pivot);
            }
        } else {
            // Prop
            const Scene::Prop& prop = g_state.scene.props[entity.prop_index];
            
            // Calculate depth scaling for prop
            float depth_scale = Scene::get_depth_scaling(g_state.scene, prop.position.y);
            Vec2 scaled_size = Vec2(
                prop.size.x * depth_scale,
                prop.size.y * depth_scale
            );
            
            Renderer::render_sprite(prop.texture, prop.position, scaled_size,
                                   z_depth, prop.pivot);
        }
    }
    
    // Debug overlay
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

