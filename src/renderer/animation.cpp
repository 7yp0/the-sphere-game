#include "animation.h"
#include "renderer.h"
#include "../debug/debug_log.h"

namespace Renderer {

SpriteAnimation create_animation(const TextureID frames[], uint32_t count, float frame_duration) {
    if (count == 0) {
        DEBUG_ERROR("Animation frame count must be > 0");
        return SpriteAnimation();
    }
    if (frame_duration <= 0.0f) {
        DEBUG_ERROR("Animation frame_duration must be > 0");
        return SpriteAnimation();
    }
    SpriteAnimation anim;
    anim.frames.assign(frames, frames + count);
    anim.frame_duration = frame_duration;
    anim.elapsed_time = 0.0f;
    anim.current_frame = 0;
    return anim;
}

bool animate(SpriteAnimation* anim, float delta_time) {
    if (!anim || anim->frames.empty()) {
        return false;
    }
    anim->elapsed_time += delta_time;
    if (anim->elapsed_time >= anim->frame_duration) {
        anim->elapsed_time = 0.0f;
        anim->current_frame = (anim->current_frame + 1) % anim->frames.size();
        return true;
    }
    return false;
}

void animation_reset(SpriteAnimation* anim) {
    if (anim) {
        anim->elapsed_time = 0.0f;
        anim->current_frame = 0;
    }
}

}

