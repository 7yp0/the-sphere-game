#pragma once

// Platform-agnostic OpenGL header selection with GL extension support

#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else
    // Windows - use GLEW for modern OpenGL support
    #include <windows.h>
    #include <GL/glew.h>
#endif
