#pragma once

#include <cstdint>
#include <vector>
#include "texture_loader.h"
#include "types.h"

namespace Renderer {

struct SpriteAnimation {
    std::vector<TextureID> frames;
    float frame_duration;
    float elapsed_time;
    uint32_t current_frame;
};

SpriteAnimation create_animation(const TextureID frames[], uint32_t count, float frame_duration);
bool animate(SpriteAnimation* anim, float delta_time);
// render_sprite_animated declaration moved to renderer.h
void animation_reset(SpriteAnimation* anim);

}

