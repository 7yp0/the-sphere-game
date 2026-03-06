#pragma once

#include <cstdio>

#ifndef NDEBUG
    #ifdef __GNUC__
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
    #endif
    
    #define DEBUG_LOG(fmt, ...) do { \
        fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__); \
        fflush(stderr); \
    } while(0)
    
    #define DEBUG_ERROR(fmt, ...) do { \
        fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__); \
        fflush(stderr); \
    } while(0)
    
    #define DEBUG_INFO(fmt, ...) do { \
        fprintf(stderr, "[INFO] " fmt "\n", ##__VA_ARGS__); \
        fflush(stderr); \
    } while(0)
    
    #ifdef __GNUC__
        #pragma GCC diagnostic pop
    #endif
#else
    #define DEBUG_LOG(fmt, ...) do {} while(0)
    #define DEBUG_ERROR(fmt, ...) do {} while(0)
    #define DEBUG_INFO(fmt, ...) do {} while(0)
#endif
