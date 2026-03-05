#pragma once

#include <cstdint>
#include <vector>
#include "texture_loader.h"
#include "types.h"

namespace Renderer {

// UV coordinates for a single frame within a sprite map
struct SpriteFrame {
    float u0, v0;  // top-left UV
    float u1, v1;  // bottom-right UV
};

struct SpriteAnimation {
    TextureID texture;                  // shared sprite map texture
    std::vector<SpriteFrame> frames;    // UV coordinates for each frame
    float frame_duration;
    float elapsed_time;
    uint32_t current_frame;
};

// Create animation from sprite map texture and frame UV coordinates
SpriteAnimation create_animation(TextureID texture, const SpriteFrame frames[], uint32_t count, float frame_duration);
bool animate(SpriteAnimation* anim, float delta_time);
// render_sprite_animated declaration moved to renderer.h
void animation_reset(SpriteAnimation* anim);

}

