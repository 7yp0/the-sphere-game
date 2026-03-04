#pragma once

#include <string>

namespace Renderer {

// Initialize asset manager with executable path
// Should be called once at startup before loading any assets
void init_asset_manager(const char* executable_path);

// Get full path for an asset (relative to executable directory)
std::string get_asset_path(const char* filename);

}
