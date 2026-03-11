#include "debug.h"
#include "renderer/text.h"
#include "renderer/renderer.h"
#include "types.h"
#include "config.h"
#include "game/game.h"
#include "platform.h"
#include "scene/scene.h"
#include "collision/polygon_utils.h"
#include "core/timing.h"
#include "ecs/ecs.h"
#include <cstdio>
#include <cstring>

namespace Debug {

bool overlay_enabled = false;

void toggle_overlay() {
  overlay_enabled = !overlay_enabled;
}

void handle_debug_keys() {
#ifndef NDEBUG
    // D key: macOS = 2, Windows = 0x44 (68)
    #ifdef __APPLE__
        const int KEY_D = 2;  // macOS virtual key code for D
    #else
        const int KEY_D = 0x44;  // Windows/Linux virtual key code for D
    #endif
    
    bool key_d = Platform::key_pressed(KEY_D);
    static bool prev_d = false;
    
    if (key_d && !prev_d) {
        toggle_overlay();
    }
    prev_d = key_d;
#endif
}

static void render_lights_debug() {
    // Scale factor from base (320x180) to viewport (1280x720)
    float scale_x = (float)Config::VIEWPORT_WIDTH / (float)Config::BASE_WIDTH;
    float scale_y = (float)Config::VIEWPORT_HEIGHT / (float)Config::BASE_HEIGHT;
    float ui_z = Layers::get_z_depth(Layer::UI);
    
    // Yellow color for point lights
    Vec4 light_color = Vec4(1.0f, 1.0f, 0.0f, 0.8f);
    
    // Draw point lights (Transform3D - OpenGL coords)
    for (ECS::EntityID light_entity : Game::g_state.scene.light_entities) {
        auto* transform = Game::g_state.ecs_world.get_component<ECS::Transform3DComponent>(light_entity);
        auto* light = Game::g_state.ecs_world.get_component<ECS::LightComponent>(light_entity);
        
        if (!transform || !light) continue;
        
        // Convert OpenGL coords (-1 to +1) to viewport pixels
        // OpenGL: X=-1 is left, X=+1 is right, Y=-1 is bottom, Y=+1 is top
        float pixel_x = (transform->position.x + 1.0f) * 0.5f * Config::VIEWPORT_WIDTH;
        float pixel_y = (1.0f - transform->position.y) * 0.5f * Config::VIEWPORT_HEIGHT;
        
        // Draw cross at light position
        Vec3 pos = Vec3(pixel_x, pixel_y, ui_z);
        float size = 12.0f;
        Renderer::render_rect(pos, Vec2(size, 4.0f), light_color);  // Horizontal
        Renderer::render_rect(Vec3(pixel_x - size/2 + 2, pixel_y - size/2 + 2, ui_z), Vec2(4.0f, size), light_color);  // Vertical
    }
    
    // Orange color for projector lights
    Vec4 proj_color = Vec4(1.0f, 0.6f, 0.0f, 0.8f);
    
    // Draw projector lights (Transform2_5D - pixel coords)
    for (ECS::EntityID proj_entity : Game::g_state.scene.projector_light_entities) {
        auto* transform = Game::g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(proj_entity);
        auto* projector = Game::g_state.ecs_world.get_component<ECS::ProjectorLightComponent>(proj_entity);
        
        if (!transform || !projector) continue;
        
        // Scale from base to viewport coordinates
        float pixel_x = transform->position.x * scale_x;
        float pixel_y = transform->position.y * scale_y;
        
        // Draw square at projector position
        Vec3 pos = Vec3(pixel_x, pixel_y, ui_z);
        Renderer::render_rect(pos, Vec2(10.0f, 10.0f), proj_color);
        
        // Draw direction line (30 pixels long)
        float line_len = 40.0f;
        Vec3 dir_end = Vec3(
            pixel_x + projector->direction.x * line_len,
            pixel_y - projector->direction.y * line_len,  // Y is inverted in screen coords
            ui_z
        );
        Renderer::render_line(pos, dir_end, proj_color, 2.0f);
    }
}

static void render_geometry_debug() {
    const Scene::Scene& scene = Game::g_state.scene;
    
    // Scale factor from base (320x180) to viewport (1280x720)
    float scale_x = (float)Config::VIEWPORT_WIDTH / (float)Config::BASE_WIDTH;
    float scale_y = (float)Config::VIEWPORT_HEIGHT / (float)Config::BASE_HEIGHT;
    
    // Draw walkable areas (green)
    Vec4 walkable_color = Vec4(0.0f, 1.0f, 0.0f, 0.7f);
    float ui_z = Layers::get_z_depth(Layer::UI);
    for (const auto& walkable : scene.geometry.walkable_areas) {
        size_t n = walkable.points.size();
        for (size_t i = 0; i < n; i++) {
            // Scale from base to viewport coordinates
            Vec3 p1 = Vec3(walkable.points[i].x * scale_x, walkable.points[i].y * scale_y, ui_z);
            Vec3 p2 = Vec3(walkable.points[(i + 1) % n].x * scale_x, walkable.points[(i + 1) % n].y * scale_y, ui_z);
            
            // Draw edge as a line
            Renderer::render_line(p1, p2, walkable_color, 2.0f);
            
            // Draw vertex as a small circle
            Renderer::render_rect(p1, Vec2(6.0f, 6.0f), walkable_color);
        }
    }
    
    // Draw hotspots (red)
    Vec4 hotspot_color = Vec4(1.0f, 0.0f, 0.0f, 0.7f);
    for (const auto& hotspot : scene.geometry.hotspots) {
        size_t n = hotspot.bounds.points.size();
        for (size_t i = 0; i < n; i++) {
            // Scale from base to viewport coordinates
            Vec3 p1 = Vec3(hotspot.bounds.points[i].x * scale_x, hotspot.bounds.points[i].y * scale_y, ui_z);
            Vec3 p2 = Vec3(hotspot.bounds.points[(i + 1) % n].x * scale_x, hotspot.bounds.points[(i + 1) % n].y * scale_y, ui_z);
            
            Renderer::render_line(p1, p2, hotspot_color, 2.0f);
            Renderer::render_rect(p1, Vec2(6.0f, 6.0f), hotspot_color);
        }
    }
}

void render_overlay(Vec2 mouse_pixel) {
    if (overlay_enabled) {
        // Draw debug visualizations (behind text)
        render_geometry_debug();
        render_lights_debug();
        
        // Calculate FPS from delta time
        float dt = Core::get_delta_time();
        float fps = (dt > 0.0001f) ? (1.0f / dt) : 0.0f;
        
        // Get entity counts
        size_t prop_count = Game::g_state.scene.prop_entities.size();
        size_t light_count = Game::g_state.scene.light_entities.size();
        size_t proj_count = Game::g_state.scene.projector_light_entities.size();
        uint32_t sc_count = Game::g_state.shadow_caster_count;
        
        // Build debug text
        char text_buffer[512];
        snprintf(text_buffer, sizeof(text_buffer), 
                 "Mouse: (%.0f, %.0f)  FPS: %.0f\n"
                 "Props: %zu  L: %zu/%u  P: %zu/%u  SC: %u/%u",
                 mouse_pixel.x, mouse_pixel.y, fps,
                 prop_count, 
                 light_count, Renderer::MAX_LIGHTS,
                 proj_count, Renderer::MAX_PROJECTOR_LIGHTS,
                 sc_count, Renderer::MAX_SHADOW_CASTERS);
        
        // Black semi-transparent background - pixel coordinates, top-left
        Vec3 bg_pos = Vec3(0.0f, 0.0f, Layers::get_z_depth(Layer::UI));
        Vec2 bg_size = Vec2(500.0f, 50.0f);
        Vec4 bg_color = Vec4(0.0f, 0.0f, 0.0f, 0.7f);
        Renderer::render_rect(bg_pos, bg_size, bg_color);
        
        // Text on top - pixel coordinates
        Vec2 text_pos = Vec2(10.0f, 10.0f);
        Renderer::render_text(text_buffer, text_pos, 1.0f);
    }
}
}
