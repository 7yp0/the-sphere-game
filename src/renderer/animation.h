#pragma once

#include <cstdint>
#include "texture_loader.h"
#include "types.h"

namespace Renderer {

struct SpriteAnimation {
    TextureID frames[16];
    uint32_t frame_count;
    float frame_duration;
    float elapsed_time;
    uint32_t current_frame;
};

// Create animation from array of textures
// frames: array of TextureIDs
// count: number of frames
// frame_duration: seconds per frame
SpriteAnimation create_animation(const TextureID frames[], uint32_t count, float frame_duration);

// Update animation timing (advance frame if time elapsed)
// delta_time: time since last frame in seconds
// Returns: true if frame advanced, false otherwise
bool animate(SpriteAnimation* anim, float delta_time);

// Render sprite using current animation frame
void render_sprite_animated(const SpriteAnimation* anim, Vec2 pos, Vec2 size);

// Reset animation to first frame
void animation_reset(SpriteAnimation* anim);

}

