#include "shader_loader.h"
#include <fstream>
#include <sstream>
#include <cstdio>

namespace Renderer {

std::string load_shader_source(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        printf("ERROR: Could not open shader file: %s\n", path);
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

}
