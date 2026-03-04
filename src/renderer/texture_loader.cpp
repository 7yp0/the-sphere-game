#include "texture_loader.h"
#include <OpenGL/gl3.h>
#include <cstdio>
#include <cstring>

namespace Renderer {

TextureID load_texture(const char* path) {
    // TODO: Implement PNG/JPG loading with proper image library
    printf("WARNING: load_texture(%s) not implemented yet. Use create_test_texture() instead.\n", path);
    return 0;
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

} // namespace Renderer
