#include "texture_loader.h"
#include "png_loader.h"
#include "../debug/debug_log.h"
#include "opengl_compat.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>
#include <map>
#include <string>

namespace Renderer {

static std::map<std::string, TextureID> g_texture_cache;

TextureID load_texture(const char* path) {
    if (!path) {
        DEBUG_ERROR("load_texture called with nullptr path");
        return create_test_texture();
    }
    
    std::string path_str(path);
    
    auto it = g_texture_cache.find(path_str);
    if (it != g_texture_cache.end()) {
        return it->second;
    }
    PNGImage img = png_load(path);
    if (img.pixels != nullptr) {
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
        
        g_texture_cache[path_str] = tex;
        return tex;
    }
    
    DEBUG_INFO("Could not load texture '%s', using fallback", path);
    return create_test_texture();
}

TextureID create_test_texture() {
    const int size = 64;
    unsigned char* data = new unsigned char[size * size * 4];
    
    for (int i = 0; i < size * size * 4; i += 4) {
        data[i]     = 255;
        data[i + 1] = 0;
        data[i + 2] = 0;
        data[i + 3] = 255;
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

DepthMapData load_depth_map(const char* path) {
    DepthMapData result;
    result.texture_id = 0;
    result.width = 0;
    result.height = 0;
    
    if (!path) {
        DEBUG_ERROR("load_depth_map called with nullptr path");
        return result;
    }
    
    // Load PNG
    PNGImage img = png_load(path);
    if (img.pixels == nullptr) {
        DEBUG_ERROR("Failed to load height map: %s", path);
        return result;
    }
    
    // Store dimensions
    result.width = img.width;
    result.height = img.height;
    
    // Convert RGBA to grayscale and store
    result.pixels.resize(img.width * img.height);
    for (uint32_t i = 0; i < img.width * img.height; ++i) {
        uint8_t r = img.pixels[i * 4 + 0];
        uint8_t g = img.pixels[i * 4 + 1];
        uint8_t b = img.pixels[i * 4 + 2];
        // Standard grayscale conversion
        result.pixels[i] = (uint8_t)((r * 0.299f + g * 0.587f + b * 0.114f));
    }
    
    // Create OpenGL texture
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, img.width, img.height, 0,
                 GL_RED, GL_UNSIGNED_BYTE, result.pixels.data());
    
    glBindTexture(GL_TEXTURE_2D, 0);
    png_free(img);
    
    result.texture_id = tex;
    DEBUG_LOG("Depth map loaded: %s (%ux%u)", path, result.width, result.height);
    return result;
}

void clear_texture_cache() {
    for (auto& pair : g_texture_cache) {
        glDeleteTextures(1, &pair.second);
    }
    g_texture_cache.clear();
}

}
