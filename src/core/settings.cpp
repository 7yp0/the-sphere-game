#include "settings.h"
#include "renderer/asset_manager.h"
#include "debug/debug_log.h"
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <direct.h>
#define MAKE_DIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MAKE_DIR(p) mkdir(p, 0755)
#endif

namespace Settings {

// ============================================================================
// State
// ============================================================================

static float       g_music_volume  = 1.0f;
static float       g_sfx_volume    = 1.0f;
static float       g_voice_volume  = 1.0f;
static std::string g_language      = "en";

// ============================================================================
// Path helpers (mirrors save_system.cpp logic)
// ============================================================================

static std::string get_settings_dir() {
    std::string exe = Renderer::get_exe_dir();
#ifdef __APPLE__
    const std::string marker = "/Contents/MacOS";
    if (exe.size() > marker.size() &&
        exe.compare(exe.size() - marker.size(), marker.size(), marker) == 0) {
        std::string bundle_parent = exe.substr(0, exe.size() - marker.size());
        size_t slash = bundle_parent.find_last_of('/');
        if (slash != std::string::npos)
            bundle_parent = bundle_parent.substr(0, slash);
        return bundle_parent;
    }
#endif
    return exe;
}

static std::string get_settings_path() {
    return get_settings_dir() + "/settings.json";
}

// ============================================================================
// JSON helpers
// ============================================================================

static std::string escape_json(const std::string& s) {
    std::string out;
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else                out += c;
    }
    return out;
}

static float clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }

// ============================================================================
// Accessors
// ============================================================================

float              get_music_volume() { return g_music_volume; }
float              get_sfx_volume()   { return g_sfx_volume; }
float              get_voice_volume() { return g_voice_volume; }
const std::string& get_language()     { return g_language; }

void set_music_volume(float v)             { g_music_volume = clamp01(v); }
void set_sfx_volume(float v)              { g_sfx_volume   = clamp01(v); }
void set_voice_volume(float v)            { g_voice_volume = clamp01(v); }
void set_language(const std::string& lang) { g_language = lang; }

// ============================================================================
// Load
// ============================================================================

void init() {
    std::ifstream f(get_settings_path());
    if (!f) {
        DEBUG_LOG("[Settings] No settings file found, using defaults");
        return;
    }
    std::stringstream buf;
    buf << f.rdbuf();
    std::string src = buf.str();

    // Minimal key-value parser for our flat settings JSON
    size_t pos = 0;
    while (true) {
        size_t ks = src.find('"', pos);
        if (ks == std::string::npos) break;
        size_t ke = src.find('"', ks + 1);
        if (ke == std::string::npos) break;
        std::string key = src.substr(ks + 1, ke - ks - 1);

        size_t colon = src.find(':', ke);
        if (colon == std::string::npos) break;
        pos = colon + 1;

        // Skip whitespace
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\n' || src[pos] == '\r'))
            ++pos;
        if (pos >= src.size()) break;

        if (src[pos] == '"') {
            // String value
            ++pos;
            size_t ve = src.find('"', pos);
            if (ve == std::string::npos) break;
            std::string val = src.substr(pos, ve - pos);
            if (key == "language") g_language = val;
            pos = ve + 1;
        } else {
            // Numeric value
            size_t end = src.find_first_of(",}\n\r", pos);
            std::string val = src.substr(pos, end - pos);
            // trim trailing whitespace
            size_t last = val.find_last_not_of(" \t");
            if (last != std::string::npos) val = val.substr(0, last + 1);
            try {
                float v = std::stof(val);
                if (key == "music_volume")  g_music_volume = clamp01(v);
                if (key == "sfx_volume")    g_sfx_volume   = clamp01(v);
                if (key == "voice_volume")  g_voice_volume = clamp01(v);
            } catch (...) {}
            pos = (end == std::string::npos) ? src.size() : end;
        }
    }

    DEBUG_INFO("[Settings] Loaded from %s", get_settings_path().c_str());
}

// ============================================================================
// Save
// ============================================================================

void save() {
    std::ofstream f(get_settings_path());
    if (!f) {
        DEBUG_ERROR("[Settings] Cannot write settings file: %s", get_settings_path().c_str());
        return;
    }
    f << "{\n";
    f << "    \"music_volume\": "  << g_music_volume  << ",\n";
    f << "    \"sfx_volume\": "    << g_sfx_volume    << ",\n";
    f << "    \"voice_volume\": "  << g_voice_volume  << ",\n";
    f << "    \"language\": \""    << escape_json(g_language) << "\"\n";
    f << "}\n";
    DEBUG_INFO("[Settings] Saved to %s", get_settings_path().c_str());
}

} // namespace Settings
