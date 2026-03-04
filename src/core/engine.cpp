#include "core/engine.h"
#include "platform/mac/mac_window.h"
#include <OpenGL/gl3.h>

int engine_run(int argc, char** argv)
{
    Platform::WindowConfig cfg;
    cfg.width = 1024;
    cfg.height = 768;
    cfg.title = "The Sphere Game";

    if (!Platform::init_window(cfg))
        return -1;

    bool running = true;
    while (running) {
        // Clear Screen
        Platform::clear_screen();

        // TODO: später Renderer/Scene Update hier

        Platform::swap_buffers();
        running = !Platform::window_should_close();
    }

    Platform::shutdown();
    return 0;
}
