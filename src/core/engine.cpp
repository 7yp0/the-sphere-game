#include "core/engine.h"
#include "platform/mac/mac_window.h"
#include "renderer/renderer.h"
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

    bool running = true;
    while (running) {
        Renderer::clear_screen();
        Renderer::render_quad();
        Platform::swap_buffers();

        running = !Platform::window_should_close();
    }

    Renderer::shutdown();
    return 0;
}
