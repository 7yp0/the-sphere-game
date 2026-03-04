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
)

add_executable(the_sphere_game ${ENGINE_SOURCES})

target_include_directories(the_sphere_game PRIVATE src)
