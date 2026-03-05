#include "shader_loader.h"
#include "asset_manager.h"
#include <fstream>
#include <sstream>
#include <cstdio>

namespace Renderer {

std::string load_shader_source(const char* path) {
    // Try to load shader relative to asset base path
    std::string full_path = get_shader_path(path);
    
    std::ifstream file(full_path);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    printf("ERROR: Could not open shader file: %s (tried: %s)\n", path, full_path.c_str());
    return "";
}

}
