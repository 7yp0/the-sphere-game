#pragma once

#include <cstdio>

#ifndef NDEBUG
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
    
    #define DEBUG_LOG(fmt, ...) do { \
        fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__); \
        fflush(stderr); \
    } while(0)
    
    #define DEBUG_ERROR(fmt, ...) do { \
        fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__); \
        fflush(stderr); \
    } while(0)
    
    #pragma GCC diagnostic pop
#else
    #define DEBUG_LOG(fmt, ...) do {} while(0)
    #define DEBUG_ERROR(fmt, ...) do {} while(0)
#endif
