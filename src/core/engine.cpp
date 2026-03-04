#include "engine.h"
#include <iostream>

int engine_run(int argc, char** argv)
{
    std::cout << "Engine is booting...\n";

    // TODO
    // - Platform Layer
    // - Renderer
    // - Scene
    // - GameState

    // TODO Main Loop placeholder
    bool running = true;
    while(running)
    {
        // TODO: Input polling
        // TODO: Update GameState
        // TODO: Render

        running = false;
    }

    std::cout << "Engine shutting down...\n";

    return 0;
}
