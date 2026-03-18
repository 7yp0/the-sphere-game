#include "asset_manager.h"
#include "../debug/debug_log.h"
#include <cstring>
#include <cstdlib>
#ifdef __APPLE__
#include <sys/stat.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

namespace Renderer {

static std::string g_asset_base_path;
static std::string g_exe_dir;

void init_asset_manager(const char* executable_path)
{
    std::string path_str(executable_path);
    
    // On Windows, argv[0] might be just the filename without path
    // Use GetModuleFileName to get the full path
#ifdef _WIN32
    char full_path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, full_path, MAX_PATH)) {
        path_str = full_path;
        // Normalize: convert backslashes to forward slashes for consistency
        for (auto& c : path_str) {
            if (c == '\\') c = '/';
        }
    } else {
        DEBUG_ERROR("GetModuleFileName failed, using argv[0]: %s", executable_path);
    }
#endif
    
    size_t last_slash = path_str.find_last_of("/");

    std::string exe_dir;
    if (last_slash != std::string::npos) {
        exe_dir = path_str.substr(0, last_slash);
    } else {
        exe_dir = ".";
    }
    g_exe_dir = exe_dir;
    
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
    // Windows: Executable is in build/Debug or build/Release
    // Assets are copied to the same directory via post-build in build.ps1
    // So just return the direct path
    std::string result = g_asset_base_path + "/assets/" + filename;
    DEBUG_INFO("get_asset_path('%s') -> '%s'", filename, result.c_str());
    return result;
}

std::string get_exe_dir()
{
    return g_exe_dir;
}

std::string get_shader_path(const char* filename)
{
    // Shaders are in build/shaders/ directory (copied there by CMake)
    return g_asset_base_path + "/shaders/" + filename;
}

}
