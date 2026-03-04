#pragma once
#include <cstdint>

namespace Renderer {

using TextureID = uint32_t;

// Loads a texture from file (PNG/JPG) - PNG support coming soon
TextureID load_texture(const char* path);

// Creates a simple test texture (red square)
TextureID create_test_texture();

// Frees a texture
void free_texture(TextureID tex);

}
