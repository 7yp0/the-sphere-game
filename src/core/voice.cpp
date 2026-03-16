#include "voice.h"
#include <string>

namespace Voice {

std::string get_voice_path(const std::string& key, const std::string& lang) {
    // Replace '.' in key with '/' for subfolders if needed, or keep as is
    // Here: keep as flat structure
    return "voice/" + lang + "/" + key + ".ogg";
}

} // namespace Voice
