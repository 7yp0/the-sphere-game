#include "core/engine.h"
#include "platform/mac/mac_window.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "game/game.h"
#include <chrono>

int engine_run(int argc, char** argv)
{
    Platform::WindowConfig config;
    config.width = 1024;
    config.height = 768;
    config.title = "The Sphere Game";

    if (!Platform::init_window(config))
        return -1;

    Renderer::init_renderer(config.width, config.height);
    Game::init();

    // Delta time tracking
    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    bool running = true;
    while (running) {
        // Calculate delta time
        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
        lastFrameTime = now;
        
        // Update game state and logic
        Game::update(deltaTime);
        
        // Update visual effects
        Game::visual_update(deltaTime);

        // Render
        Renderer::clear_screen();
        Game::render();
        Platform::swap_buffers();

        running = !Platform::window_should_close();
    }

    // Cleanup
    Game::shutdown();
    Renderer::clear_texture_cache();
    Renderer::shutdown();
    return 0;
}
