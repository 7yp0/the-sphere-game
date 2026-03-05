#include "animation_bank.h"
#include "renderer/texture_loader.h"

namespace Core {

void animation_bank_load_player(AnimationBank& bank) {
    Renderer::TextureID idle_tex = Renderer::load_texture("player_idle.png");
    Renderer::TextureID idle_frames[] = { idle_tex };
    Renderer::SpriteAnimation idle_anim = Renderer::create_animation(idle_frames, 1, 1.0f);
    bank.add("idle", idle_anim);
    
    Renderer::TextureID walk0 = Renderer::load_texture("player_walk_0.png");
    Renderer::TextureID walk1 = Renderer::load_texture("player_walk_1.png");
    Renderer::TextureID walk2 = Renderer::load_texture("player_walk_2.png");
    Renderer::TextureID walk_frames[] = { walk0, walk1, walk2 };
    Renderer::SpriteAnimation walk_anim = Renderer::create_animation(walk_frames, 3, 0.2f);
    bank.add("walk", walk_anim);
}

}
