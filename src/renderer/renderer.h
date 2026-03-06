#pragma once

#include <cstdint>
#include "types.h"
#include "animation.h"

namespace Renderer {

using GLuint = uint32_t;
using TextureID = uint32_t;

void init_renderer(uint32_t width, uint32_t height);
void clear_screen();
void render_sprite(TextureID tex, Vec2 pos, Vec2 size, float z_depth = 0.0f, PivotPoint pivot = PivotPoint::TOP_LEFT);
void render_sprite(TextureID tex, Vec2 pos, Vec2 size, Vec4 tex_coord_range, float z_depth = 0.0f, PivotPoint pivot = PivotPoint::TOP_LEFT);
void render_sprite_animated(const SpriteAnimation* anim, Vec2 pos, Vec2 size, float z_depth = 0.0f, PivotPoint pivot = PivotPoint::TOP_LEFT);
void render_rect(Vec2 pos, Vec2 size, Vec4 color, float z_depth = 0.0f, PivotPoint pivot = PivotPoint::TOP_LEFT);

// 2.5D Depth Scaling Functions
// Calculate sprite scale based on Y position relative to horizon
float calculate_depth_scale(float sprite_y, float horizon_y, float scale_gradient = 0.003f, bool inverted = false);

void render_sprite_with_depth(TextureID tex, Vec2 pos, Vec2 base_size, float sprite_y, 
                              float horizon_y, float scale_gradient = 0.003f, bool inverted = false,
                              float z_depth = 0.0f, PivotPoint pivot = PivotPoint::TOP_LEFT);

void render_sprite_with_depth(TextureID tex, Vec2 pos, Vec2 base_size, Vec4 tex_coord_range,
                              float sprite_y, float horizon_y, float scale_gradient = 0.003f, bool inverted = false,
                              float z_depth = 0.0f, PivotPoint pivot = PivotPoint::TOP_LEFT);

void render_sprite_animated_with_depth(const SpriteAnimation* anim, Vec2 pos, Vec2 base_size,
                                       float sprite_y, float horizon_y, float scale_gradient = 0.003f, bool inverted = false,
                                       float z_depth = 0.0f, PivotPoint pivot = PivotPoint::TOP_LEFT);

void shutdown();

}
