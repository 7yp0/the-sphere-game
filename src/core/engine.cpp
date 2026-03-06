#include "core/engine.h"
#include "core/timing.h"
#include "platform.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"
#include "renderer/asset_manager.h"
#include "game/game.h"
#include "config.h"
#include "debug/debug_log.h"
#include <chrono>

int engine_run(int argc, char** argv)
{
    // Initialize asset manager with executable path for resource loading
    if (argc > 0) {
        Renderer::init_asset_manager(argv[0]);
    } else {
        DEBUG_ERROR("No argv provided, asset loading may fail");
    }
    
    Platform::WindowConfig config;
    config.width = Config::VIEWPORT_WIDTH;
    config.height = Config::VIEWPORT_HEIGHT;
    config.title = Config::WINDOW_TITLE;

    if (!Platform::init_window(config))
        return -1;

    // Use actual client rect dimensions, not requested window size
    uint32_t actual_width = Platform::get_window_width();
    uint32_t actual_height = Platform::get_window_height();
    
    Renderer::init_renderer(actual_width, actual_height);
    Game::set_viewport(actual_width, actual_height);
    Game::init();

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
