#include "asset_manager.h"
#include <cstring>
#include <cstdlib>
#ifdef __APPLE__
#include <sys/stat.h>
#endif

namespace Renderer {

static std::string g_asset_base_path;

void init_asset_manager(const char* executable_path)
{
    std::string path_str(executable_path);
    size_t last_slash = path_str.find_last_of("/\\");
    
    std::string exe_dir;
    if (last_slash != std::string::npos) {
        exe_dir = path_str.substr(0, last_slash);
    } else {
        exe_dir = ".";
    }
    
#ifdef __APPLE__
    // macOS: Check if we're running from a .app bundle
    // Bundle structure: MyApp.app/Contents/MacOS/executable
    // Assets are at: MyApp.app/Contents/Resources/
    std::string bundle_resources = exe_dir + "/../Resources";
    struct stat st;
    if (stat(bundle_resources.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        g_asset_base_path = bundle_resources;
    } else {
        g_asset_base_path = exe_dir;
    }
#else
    // Windows/Linux: Assets are in build/ directory relative to executable
    g_asset_base_path = exe_dir;
#endif
}

std::string get_asset_path(const char* filename)
{
    return g_asset_base_path + "/assets/" + filename;
}

std::string get_shader_path(const char* filename)
{
    // Shaders are in build/shaders/ directory (copied there by CMake)
    return g_asset_base_path + "/shaders/" + filename;
}

}
