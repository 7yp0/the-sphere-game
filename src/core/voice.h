#pragma once
#include <string>

namespace Voice {

// Returns the expected path to the voice file for a given key and language
std::string get_voice_path(const std::string& key, const std::string& lang);

} // namespace Voice
