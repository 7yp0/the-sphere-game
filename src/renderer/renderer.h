#pragma once

#include <cstdint>
#include <vector>
#include "types.h"
#include "animation.h"

namespace Scene {
    struct PointLight;  // Forward declaration
}

namespace Renderer {

using GLuint = uint32_t;
using TextureID = uint32_t;

void init_renderer(uint32_t width, uint32_t height);
void clear_screen();
void set_depth_map_data(TextureID depthMapTexture, uint32_t sceneWidth, uint32_t sceneHeight);
void render_sprite(TextureID tex, Vec3 pos, Vec2 size, PivotPoint pivot = PivotPoint::TOP_LEFT);
void render_sprite(TextureID tex, Vec3 pos, Vec2 size, Vec4 tex_coord_range, PivotPoint pivot = PivotPoint::TOP_LEFT);
void render_sprite_animated(const SpriteAnimation* anim, Vec3 pos, Vec2 size, PivotPoint pivot = PivotPoint::TOP_LEFT);
void render_rect(Vec3 pos, Vec2 size, Vec4 color, PivotPoint pivot = PivotPoint::TOP_LEFT);
void render_line(Vec3 start, Vec3 end, Vec4 color, float thickness = 1.0f);

// 2.5D Depth Scaling Functions
// Calculate sprite scale based on Y position relative to horizon
float calculate_depth_scale(float sprite_y, float horizon_y, float scale_gradient = 0.003f, bool inverted = false);

void render_sprite_with_depth(TextureID tex, Vec3 pos, Vec2 base_size, float sprite_y, 
                              float horizon_y, float scale_gradient = 0.003f, bool inverted = false,
                              PivotPoint pivot = PivotPoint::TOP_LEFT);

void render_sprite_with_depth(TextureID tex, Vec3 pos, Vec2 base_size, Vec4 tex_coord_range,
                              float sprite_y, float horizon_y, float scale_gradient = 0.003f, bool inverted = false,
                              PivotPoint pivot = PivotPoint::TOP_LEFT);

void render_sprite_animated_with_depth(const SpriteAnimation* anim, Vec3 pos, Vec2 base_size,
                                       float sprite_y, float horizon_y, float scale_gradient = 0.003f, bool inverted = false,
                                       PivotPoint pivot = PivotPoint::TOP_LEFT);

// Point light rendering - lights affect sprite illumination based on position and depth
void render_sprite_lit(TextureID tex, Vec3 pos, Vec2 size, const std::vector<Scene::PointLight>& lights, 
                       TextureID normal_map = 0, PivotPoint pivot = PivotPoint::TOP_LEFT);

void render_sprite_lit(TextureID tex, Vec3 pos, Vec2 size, Vec4 tex_coord_range, const std::vector<Scene::PointLight>& lights,
                       TextureID normal_map = 0, PivotPoint pivot = PivotPoint::TOP_LEFT);

void render_sprite_animated_lit(const SpriteAnimation* anim, Vec3 pos, Vec2 size, const std::vector<Scene::PointLight>& lights,
                                TextureID normal_map = 0, PivotPoint pivot = PivotPoint::TOP_LEFT);

// Framebuffer Object (FBO) for offscreen rendering at base resolution
// All scene rendering goes to FBO at 320x180, then upscaled to viewport
void init_framebuffer(uint32_t base_width, uint32_t base_height);
void begin_render_to_framebuffer();   // Bind FBO, set viewport to base resolution
void end_render_to_framebuffer();     // Unbind FBO
void render_framebuffer_to_screen();  // Blit FBO texture to screen with upscaling
void shutdown_framebuffer();

// Get current rendering dimensions (FBO base or viewport)
uint32_t get_render_width();
uint32_t get_render_height();

void shutdown();

}
