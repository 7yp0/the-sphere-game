#include "asset_manager.h"
#include <cstring>
#include <cstdlib>

namespace Renderer {

static std::string g_asset_base_path;

void init_asset_manager(const char* executable_path)
{
    // Extract directory from executable path
    // e.g., "/path/to/build/the_sphere_game" -> "/path/to/build"
    
    std::string path_str(executable_path);
    size_t last_slash = path_str.find_last_of("/\\");
    
    if (last_slash != std::string::npos) {
        g_asset_base_path = path_str.substr(0, last_slash);
    } else {
        g_asset_base_path = ".";
    }
}

std::string get_asset_path(const char* filename)
{
    // Returns: {executable_dir}/assets/{filename}
    return g_asset_base_path + "/assets/" + filename;
}

}
