#include "texture_loader.h"
#include "png_loader.h"
#include <OpenGL/gl3.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>
#include <map>
#include <string>

namespace Renderer {

// Texture cache: filename -> TextureID
static std::map<std::string, TextureID> g_texture_cache;

TextureID load_texture(const char* path) {
    std::string path_str(path);
    
    // Check cache first
    auto it = g_texture_cache.find(path_str);
    if (it != g_texture_cache.end()) {
        return it->second;  // Return cached texture
    }
    
    // Try PNG first
    PNGImage img = png_load(path);
    if (img.pixels != nullptr) {
        // Create OpenGL texture
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, img.pixels);
        
        glBindTexture(GL_TEXTURE_2D, 0);
        png_free(img);
        
        // Cache it
        g_texture_cache[path_str] = tex;
        return tex;
    }
    
    return create_test_texture();
}

TextureID create_test_texture() {
    // Create a simple 64x64 red test texture
    const int size = 64;
    unsigned char* data = new unsigned char[size * size * 4];
    
    for (int i = 0; i < size * size * 4; i += 4) {
        data[i]     = 255;  // R
        data[i + 1] = 0;    // G
        data[i + 2] = 0;    // B
        data[i + 3] = 255;  // A
    }
    
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    delete[] data;
    return tex;
}

void free_texture(TextureID tex) {
    glDeleteTextures(1, &tex);
}

void clear_texture_cache() {
    // Free all cached textures
    for (auto& pair : g_texture_cache) {
        glDeleteTextures(1, &pair.second);
    }
    g_texture_cache.clear();
}

} // namespace Renderer
