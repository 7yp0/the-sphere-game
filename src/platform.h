#pragma once

// Platform abstraction header - includes the correct platform implementation

#ifdef __APPLE__
    #include "platform/mac/mac_window.h"
#elif defined(_WIN32) || defined(_WIN64)
    #include "platform/windows/windows_window.h"
#elif defined(__linux__)
    // Linux implementation would go here
    #error "Linux platform not yet implemented"
#else
    #error "Unknown platform"
#endif
