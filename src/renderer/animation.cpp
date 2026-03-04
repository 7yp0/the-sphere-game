#include "animation.h"
#include "renderer.h"
#include <cstdio>

namespace Renderer {

SpriteAnimation create_animation(const TextureID frames[], uint32_t count, float frame_duration) {
    if (count == 0 || count > 16) {
        printf("ERROR: Animation frame count must be 1-16, got %u\n", count);
        SpriteAnimation empty = {};
        return empty;
    }
    
    if (frame_duration <= 0.0f) {
        printf("ERROR: Animation frame_duration must be > 0, got %f\n", frame_duration);
        SpriteAnimation empty = {};
        return empty;
    }
    
    SpriteAnimation anim = {};
    anim.frame_count = count;
    anim.frame_duration = frame_duration;
    anim.elapsed_time = 0.0f;
    anim.current_frame = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        anim.frames[i] = frames[i];
    }
    
    return anim;
}

bool animate(SpriteAnimation* anim, float delta_time) {
    if (!anim || anim->frame_count == 0) {
        return false;
    }
    
    anim->elapsed_time += delta_time;
    
    if (anim->elapsed_time >= anim->frame_duration) {
        anim->elapsed_time = 0.0f;
        anim->current_frame = (anim->current_frame + 1) % anim->frame_count;
        return true;
    }
    
    return false;
}

void render_sprite_animated(const SpriteAnimation* anim, Vec2 pos, Vec2 size) {
    if (!anim || anim->frame_count == 0) {
        printf("ERROR: Invalid animation for rendering\n");
        return;
    }
    
    TextureID current_tex = anim->frames[anim->current_frame];
    render_sprite(current_tex, pos, size);
}

void animation_reset(SpriteAnimation* anim) {
    if (anim) {
        anim->elapsed_time = 0.0f;
        anim->current_frame = 0;
    }
}

}

