#include "core/engine.h"
#include <cstdio>

#ifdef _WIN32
    #include <windows.h>
#endif

int main(int argc, char** argv)
{
    // On Windows debug builds, allocate a console for output
    #if defined(_WIN32) && !defined(NDEBUG)
        AllocConsole();
    #endif
    
    return engine_run(argc, argv);
}
