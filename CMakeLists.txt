cmake_minimum_required(VERSION 3.20)
project(the_sphere_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Type Default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

file(GLOB_RECURSE ENGINE_SOURCES
    src/*.cpp
    src/*.mm
)

add_executable(the_sphere_game ${ENGINE_SOURCES})

# Find ZLIB (used for PNG decompression)
find_package(ZLIB REQUIRED)

if(APPLE)
    target_link_libraries(the_sphere_game
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
        "-framework CoreVideo"
        ZLIB::ZLIB
    )
    target_compile_options(the_sphere_game PRIVATE
        "-Wno-deprecated-declarations"
    )
elseif(WIN32)
    # Windows: ZLIB should be available via vcpkg or system
    target_link_libraries(the_sphere_game
        ZLIB::ZLIB
    )
elseif(UNIX)
    # Linux: ZLIB from system package manager
    target_link_libraries(the_sphere_game
        ZLIB::ZLIB
    )
endif()

target_include_directories(the_sphere_game PRIVATE src)
