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

# Filter platform-specific files based on OS
if(APPLE)
    # On macOS: include only Mac platform files, exclude Windows files
    list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*windows/.*")
elseif(WIN32)
    # On Windows: exclude Mac platform files (*.mm files)
    list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*platform/mac/.*")
    list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*\\.mm$")
elseif(UNIX)
    # On Linux: exclude both Mac and Windows platform files
    list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*platform/(mac|windows)/.*")
    list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*\\.mm$")
endif()

add_executable(the_sphere_game ${ENGINE_SOURCES})

# Copy assets to build directory alongside the executable
add_custom_command(TARGET the_sphere_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:the_sphere_game>/assets
    COMMENT "Copying assets to build directory"
)

# Collect all shader files
file(GLOB SHADER_FILES 
    ${CMAKE_SOURCE_DIR}/src/renderer/shaders/*.vert
    ${CMAKE_SOURCE_DIR}/src/renderer/shaders/*.frag
)

# Copy shaders on every build (POST_BUILD ensures they're always fresh)
add_custom_command(TARGET the_sphere_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:the_sphere_game>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/renderer/shaders
        $<TARGET_FILE_DIR:the_sphere_game>/shaders
    COMMENT "Copying shaders to build directory"
)

# Note: On macOS, build.sh creates the .app bundle and copies assets/shaders
# to Contents/Resources/. The CMake post-build commands above copy to
# $<TARGET_FILE_DIR>, which build.sh then uses as source for the bundle.

# Find ZLIB (used for PNG decompression)
find_package(ZLIB REQUIRED)

# Find GLEW only on Windows/Linux (not needed on macOS with native OpenGL)
if(NOT APPLE)
    find_package(GLEW CONFIG REQUIRED)
endif()

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
    target_link_libraries(the_sphere_game
        opengl32
        user32
        gdi32
        ZLIB::ZLIB
        GLEW::GLEW
    )
elseif(UNIX)
    # Linux: ZLIB from system package manager
    target_link_libraries(the_sphere_game
        ZLIB::ZLIB
    )
endif()

target_include_directories(the_sphere_game PRIVATE src)
