#pragma once

#include <string>

namespace Renderer {

void init_asset_manager(const char* executable_path);
std::string get_asset_path(const char* filename);

}
