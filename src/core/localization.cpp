#include "localization.h"
#include "renderer/asset_manager.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Localization {

static std::unordered_map<std::string, std::string> strings;
static std::string current_language = "en";

void load(const std::string& lang) {
    current_language = lang;
    strings.clear();
    std::string path = Renderer::get_asset_path(("localization/" + lang + ".json").c_str());
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Localization file not found: " + path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // Simple key-value JSON parser (flat, no nesting, no escapes)
    size_t pos = 0;
    while (true) {
        // Find key
        size_t key_start = content.find('"', pos);
        if (key_start == std::string::npos) break;
        size_t key_end = content.find('"', key_start + 1);
        if (key_end == std::string::npos) break;
        std::string key = content.substr(key_start + 1, key_end - key_start - 1);

        // Find value
        size_t colon = content.find(':', key_end);
        if (colon == std::string::npos) break;
        size_t value_start = content.find('"', colon);
        if (value_start == std::string::npos) break;
        size_t value_end = content.find('"', value_start + 1);
        if (value_end == std::string::npos) break;
        std::string value = content.substr(value_start + 1, value_end - value_start - 1);

        strings[key] = value;
        pos = value_end + 1;
    }
}

const std::string& get(const std::string& key) {
    static std::string missing = "[MISSING]";
    auto it = strings.find(key);
    if (it != strings.end()) return it->second;
    return missing;
}

void set_language(const std::string& lang) {
    load(lang);
}

const std::string& get_language() {
    return current_language;
}

} // namespace Localization
