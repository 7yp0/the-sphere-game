#pragma once
#include <string>
#include <unordered_map>

namespace Localization {

void load(const std::string& lang);
const std::string& get(const std::string& key);
void set_language(const std::string& lang);
const std::string& get_language();

} // namespace Localization
