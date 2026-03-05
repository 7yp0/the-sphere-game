#include "core/engine.h"
#include "core/timing.h"
#include "platform/mac/mac_window.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "renderer/asset_manager.h"
#include "game/game.h"
#include <chrono>

int engine_run(int argc, char** argv)
{
    // Initialize asset manager with executable path for resource loading
    if (argc > 0) {
        Renderer::init_asset_manager(argv[0]);
    } else {
        printf("WARNING: No argv provided, asset loading may fail.\n");
    }
    
    Platform::WindowConfig config;
    config.width = 1024;
    config.height = 768;
    config.title = "The Sphere Game";

    if (!Platform::init_window(config))
        return -1;

    Renderer::init_renderer(config.width, config.height);
    Game::init();
    Game::set_viewport(config.width, config.height);

    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    bool running = true;
    while (running) {
        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
        lastFrameTime = now;
        
        Core::update_delta_time(deltaTime);
        Game::update(deltaTime);

        Renderer::clear_screen();
        Game::render();
        Platform::swap_buffers();

        running = !Platform::window_should_close();
    }

    Game::shutdown();
    Renderer::clear_texture_cache();
    Renderer::shutdown();
    return 0;
}
