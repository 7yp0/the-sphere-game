#pragma once
#include <cstdint>

namespace Renderer {

using TextureID = uint32_t;

// Loads a PNG texture from file with caching
// Assets location: build/assets/{filename}
// Textures are cached - loading same file twice returns cached TextureID (no re-load)
TextureID load_texture(const char* path);

// Creates a simple test texture (64x64 red square)
// Used as fallback if PNG loading fails
TextureID create_test_texture();

// Frees a texture by OpenGL handle
void free_texture(TextureID tex);

// Free all cached textures (call before shutdown)
void clear_texture_cache();

}
