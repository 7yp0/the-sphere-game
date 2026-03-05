#include "animation_bank.h"
#include "renderer/texture_loader.h"

namespace Core {

void animation_bank_load_player(AnimationBank& bank) {
    // Load sprite map texture for player animations (4 frames: idle + 3 walk)
    // Layout: [idle | walk0 | walk1 | walk2]
    Renderer::TextureID player_sprite_map = Renderer::load_texture("player/player_spritesheet.png");
    
    // Idle animation - single frame (first 1/4 of sprite map)
    Renderer::SpriteFrame idle_frames[] = {
        { 0.0f, 0.0f, 0.25f, 1.0f }  // Frame 0: left 1/4
    };
    Renderer::SpriteAnimation idle_anim = Renderer::create_animation(
        player_sprite_map, idle_frames, 1, 1.0f
    );
    bank.add("idle", idle_anim);
    
    // Walk animation - 3 frames horizontally packed
    // Each frame is 1/4 of the texture width
    Renderer::SpriteFrame walk_frames[] = {
        { 0.25f,  0.0f, 0.5f,  1.0f },  // Frame 1: second 1/4
        { 0.5f,   0.0f, 0.75f, 1.0f },  // Frame 2: third 1/4
        { 0.75f,  0.0f, 1.0f,  1.0f }   // Frame 3: right 1/4
    };
    Renderer::SpriteAnimation walk_anim = Renderer::create_animation(
        player_sprite_map, walk_frames, 3, 0.2f
    );
    bank.add("walk", walk_anim);
}

}
