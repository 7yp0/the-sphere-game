#include "scene.h"
#include "game/game.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "renderer/animation.h"

using Game::g_state;

namespace Scene {

void init_scene_test() {
    g_state.blueTex = Renderer::load_texture("test.png");
    g_state.redTex = Renderer::load_texture("test_red.png");
    g_state.greenTex = Renderer::load_texture("test_green.png");

    Renderer::TextureID animFrames[] = { g_state.redTex, g_state.greenTex, g_state.blueTex };
    g_state.anim = Renderer::create_animation(animFrames, 3, 0.5f);

    Renderer::TextureID reverseFrames[] = { g_state.blueTex, g_state.greenTex, g_state.redTex };
    g_state.reverseAnim = Renderer::create_animation(reverseFrames, 3, 0.5f);
}

}
