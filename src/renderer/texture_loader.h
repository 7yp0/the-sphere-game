#pragma once
#include <cstdint>
#include <vector>

namespace Renderer {

using TextureID = uint32_t;

// Height map data structure - stores pixel data for sampling
struct HeightMapData {
    TextureID texture_id;
    uint32_t width;
    uint32_t height;
    std::vector<uint8_t> pixels;  // Grayscale pixel values (0-255)
    
    bool is_valid() const {
        return texture_id != 0 && width > 0 && height > 0 && !pixels.empty();
    }
};

// Loads a PNG texture from file with caching
// Assets location: build/assets/{filename}
// Textures are cached - loading same file twice returns cached TextureID (no re-load)
TextureID load_texture(const char* path);

// Loads a height map texture, keeping pixel data for runtime sampling
HeightMapData load_height_map(const char* path);

// Creates a simple test texture (64x64 red square)
// Used as fallback if PNG loading fails
TextureID create_test_texture();

// Frees a texture by OpenGL handle
void free_texture(TextureID tex);

// Free all cached textures (call before shutdown)
void clear_texture_cache();

}
