#include "shader_loader.h"
#include "asset_manager.h"
#include <fstream>
#include <sstream>
#include <cstdio>

namespace Renderer {

std::string load_shader_source(const char* path) {
    // Shaders are stored relative to source, need to find them relative to executable
    // For now, assume path like "src/renderer/shaders/basic.vert"
    // We'll construct: {executable_dir}/../{path}
    
    // Try the direct path first (if called from project root)
    std::ifstream file(path);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    // Try with ../ (in case executable is in build/)
    std::string alt_path = std::string("../") + path;
    file.clear();
    file.open(alt_path);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    printf("ERROR: Could not open shader file: %s (also tried %s)\n", path, alt_path.c_str());
    return "";
}

}
