#include "core/engine.h"
#include "platform/mac/mac_window.h"
#include <OpenGL/gl3.h>

int engine_run(int argc, char** argv)
{
    Platform::WindowConfig config;
    config.width = 1024;
    config.height = 768;
    config.title = "The Sphere Game";

    if (!Platform::init_window(config))
        return -1;

    Platform::init_renderer();

    bool running = true;
    while (running) {
        Platform::clear_screen();
        Platform::render_quad();
        Platform::swap_buffers();

        running = !Platform::window_should_close();
    }

    Platform::shutdown();
    return 0;
}
