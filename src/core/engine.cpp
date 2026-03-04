#include "core/engine.h"
#include "platform/mac/mac_window.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "types.h"
#include <OpenGL/gl3.h>

int engine_run(int argc, char** argv)
{
    Platform::WindowConfig config;
    config.width = 1024;
    config.height = 768;
    config.title = "The Sphere Game";

    if (!Platform::init_window(config))
        return -1;

    Renderer::init_renderer(config.width, config.height);

    // Load test textures (different colors)
    Renderer::TextureID blueTex = Renderer::load_texture("test.png");
    Renderer::TextureID redTex = Renderer::load_texture("test_red.png");
    Renderer::TextureID greenTex = Renderer::load_texture("test_green.png");

    bool running = true;
    while (running) {
        Renderer::clear_screen();
        
        // Blue sprite at center
        Renderer::render_sprite(blueTex, Vec2(0.0f, 0.0f), Vec2(0.2f, 0.2f));
        
        // Red sprite at top-left
        Renderer::render_sprite(redTex, Vec2(-0.5f, 0.5f), Vec2(0.15f, 0.15f));
        
        // Green sprite at top-right
        Renderer::render_sprite(greenTex, Vec2(0.5f, 0.5f), Vec2(0.1f, 0.1f));
        
        // Red sprite at bottom-left (different size)
        Renderer::render_sprite(redTex, Vec2(-0.5f, -0.5f), Vec2(0.25f, 0.25f));
        
        // Green sprite at bottom-right
        Renderer::render_sprite(greenTex, Vec2(0.5f, -0.5f), Vec2(0.18f, 0.18f));
        
        Platform::swap_buffers();
        running = !Platform::window_should_close();
    }

    // Clear texture cache (frees all cached textures)
    Renderer::clear_texture_cache();
    Renderer::shutdown();
    return 0;
}
