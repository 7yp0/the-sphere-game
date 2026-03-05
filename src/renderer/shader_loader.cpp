#include "shader_loader.h"
#include "asset_manager.h"
#include "../debug/debug_log.h"
#include <fstream>
#include <sstream>

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
    
    DEBUG_ERROR("Could not open shader file: %s (tried: %s)", path, full_path.c_str());
    return "";
}

}
