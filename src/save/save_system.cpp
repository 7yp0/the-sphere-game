#include "save_system.h"
#include "game/game.h"
#include "inventory/inventory.h"
#include "scene/scene_registry.h"
#include "renderer/asset_manager.h"
#include "ecs/components/transform.h"
#include "ecs/components/sprite.h"
#include "debug/debug_log.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <stdexcept>

#ifdef _WIN32
#include <direct.h>
#define MAKE_DIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MAKE_DIR(p) mkdir(p, 0755)
#endif

namespace SaveSystem {

static constexpr int   SAVE_VERSION      = 1;
static constexpr float DEBOUNCE_INTERVAL = 1.0f;

static float g_pending_timer = -1.0f;
static bool  g_is_loading    = false;

// ============================================================================
// Path helpers
// ============================================================================

static std::string get_save_dir()  { return Renderer::get_exe_dir() + "/saves"; }
static std::string get_save_path() { return get_save_dir() + "/autosave.json"; }

static void ensure_save_dir() {
    MAKE_DIR(get_save_dir().c_str());
}

// ============================================================================
// JSON serialization helpers
// ============================================================================

static std::string escape_json(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else                out += c;
    }
    return out;
}

// ============================================================================
// Save
// ============================================================================

void save() {
    if (g_is_loading) return;

    using namespace Game;

    ensure_save_dir();

    // Capture current player position
    float player_x = -1.0f, player_y = -1.0f;
    if (g_state.player_entity != ECS::INVALID_ENTITY) {
        auto* t = g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(g_state.player_entity);
        if (t) { player_x = t->position.x; player_y = t->position.y; }
    }

    // Snapshot current scene state (hotspots + entity visibility)
    SceneState current_snap;
    for (const auto& hs : g_state.scene.geometry.hotspots) {
        current_snap.hotspot_enabled[hs.name] = hs.enabled;
    }
    for (const auto& [name, id] : g_state.scene.named_entities) {
        auto* sprite = g_state.ecs_world.get_component<ECS::SpriteComponent>(id);
        if (sprite) current_snap.entity_visible[name] = sprite->visible;
    }

    // Merge with stored scene states (current scene overwrites)
    auto all_states = g_state.scene_states;
    if (!g_state.scene.name.empty()) {
        all_states[g_state.scene.name] = current_snap;
    }

    // Build JSON
    std::ostringstream js;
    js << "{\n";
    js << "    \"version\": " << SAVE_VERSION << ",\n";
    js << "    \"scene\": \"" << escape_json(g_state.scene.name) << "\",\n";
    js << "    \"player_x\": " << player_x << ",\n";
    js << "    \"player_y\": " << player_y << ",\n";

    // Inventory
    js << "    \"inventory\": [";
    {
        bool first = true;
        for (const auto& slot : Inventory::get_slots()) {
            if (!slot.is_empty()) {
                if (!first) js << ", ";
                js << "\"" << escape_json(slot.item_id) << "\"";
                first = false;
            }
        }
    }
    js << "],\n";

    // Flags
    js << "    \"flags\": {";
    {
        bool first = true;
        for (const auto& [k, v] : g_state.flags) {
            if (!first) js << ", ";
            js << "\"" << escape_json(k) << "\": " << (v ? "true" : "false");
            first = false;
        }
    }
    js << "},\n";

    // Values
    js << "    \"values\": {";
    {
        bool first = true;
        for (const auto& [k, v] : g_state.values) {
            if (!first) js << ", ";
            js << "\"" << escape_json(k) << "\": " << v;
            first = false;
        }
    }
    js << "},\n";

    // Strings
    js << "    \"strings\": {";
    {
        bool first = true;
        for (const auto& [k, v] : g_state.strings) {
            if (!first) js << ", ";
            js << "\"" << escape_json(k) << "\": \"" << escape_json(v) << "\"";
            first = false;
        }
    }
    js << "},\n";

    // Scene states
    js << "    \"scene_states\": {\n";
    {
        bool first_scene = true;
        for (const auto& [scene_name, snap] : all_states) {
            if (!first_scene) js << ",\n";
            js << "        \"" << escape_json(scene_name) << "\": {\n";

            js << "            \"hotspots\": {";
            bool fh = true;
            for (const auto& [hs_name, enabled] : snap.hotspot_enabled) {
                if (!fh) js << ", ";
                js << "\"" << escape_json(hs_name) << "\": " << (enabled ? "true" : "false");
                fh = false;
            }
            js << "},\n";

            js << "            \"entities\": {";
            bool fe = true;
            for (const auto& [ent_name, visible] : snap.entity_visible) {
                if (!fe) js << ", ";
                js << "\"" << escape_json(ent_name) << "\": " << (visible ? "true" : "false");
                fe = false;
            }
            js << "}\n";

            js << "        }";
            first_scene = false;
        }
    }
    js << "\n    }\n}\n";

    std::ofstream f(get_save_path());
    if (!f) {
        DEBUG_ERROR("[SaveSystem] Cannot open save file: %s", get_save_path().c_str());
        return;
    }
    f << js.str();
    g_pending_timer = -1.0f;
    DEBUG_INFO("[SaveSystem] Saved to %s", get_save_path().c_str());
}

// ============================================================================
// JSON parser (minimal, handles our specific save format)
// ============================================================================

struct Parser {
    const std::string& src;
    size_t pos = 0;

    void skip_ws() {
        while (pos < src.size() &&
               (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\n' || src[pos] == '\r'))
            ++pos;
    }

    bool at_end() const { return pos >= src.size(); }

    bool peek(char c) { skip_ws(); return !at_end() && src[pos] == c; }

    bool consume(char c) {
        skip_ws();
        if (!at_end() && src[pos] == c) { ++pos; return true; }
        return false;
    }

    std::string parse_string() {
        skip_ws();
        if (at_end() || src[pos] != '"') return "";
        ++pos;
        std::string result;
        while (pos < src.size() && src[pos] != '"') {
            if (src[pos] == '\\' && pos + 1 < src.size()) {
                ++pos;
                if      (src[pos] == '"')  result += '"';
                else if (src[pos] == '\\') result += '\\';
                else if (src[pos] == 'n')  result += '\n';
                else                       result += src[pos];
            } else {
                result += src[pos];
            }
            ++pos;
        }
        if (!at_end()) ++pos;  // closing "
        return result;
    }

    // Parse a raw token (number or boolean keyword), stop before , } ]
    std::string parse_token() {
        skip_ws();
        std::string val;
        while (pos < src.size() &&
               src[pos] != ',' && src[pos] != '}' && src[pos] != ']' &&
               src[pos] != '\n' && src[pos] != '\r') {
            val += src[pos++];
        }
        // trim trailing whitespace
        size_t end = val.find_last_not_of(" \t");
        return (end == std::string::npos) ? "" : val.substr(0, end + 1);
    }

    void skip_object() {
        consume('{'); skip_ws();
        if (consume('}')) return;
        while (!at_end()) {
            skip_ws(); if (peek('}')) { consume('}'); return; }
            parse_string(); consume(':'); skip_value();
            consume(',');
        }
    }
    void skip_array() {
        consume('['); skip_ws();
        if (consume(']')) return;
        while (!at_end()) {
            skip_ws(); if (peek(']')) { consume(']'); return; }
            skip_value(); consume(',');
        }
    }
    void skip_value() {
        skip_ws();
        if (at_end()) return;
        if (src[pos] == '"') { parse_string(); return; }
        if (src[pos] == '{') { skip_object(); return; }
        if (src[pos] == '[') { skip_array();  return; }
        parse_token();
    }

    // Parse {key: bool, ...} into map
    void parse_bool_map(std::unordered_map<std::string, bool>& out) {
        consume('{'); skip_ws();
        while (!at_end() && !peek('}')) {
            std::string k = parse_string();
            consume(':');
            std::string v = parse_token();
            out[k] = (v == "true");
            consume(',');
            skip_ws();
        }
        consume('}');
    }

    // Parse {key: int, ...} into map
    void parse_int_map(std::unordered_map<std::string, int>& out) {
        consume('{'); skip_ws();
        while (!at_end() && !peek('}')) {
            std::string k = parse_string();
            consume(':');
            std::string v = parse_token();
            try { out[k] = std::stoi(v); } catch (...) {}
            consume(',');
            skip_ws();
        }
        consume('}');
    }

    // Parse {key: "value", ...} into map
    void parse_string_map(std::unordered_map<std::string, std::string>& out) {
        consume('{'); skip_ws();
        while (!at_end() && !peek('}')) {
            std::string k = parse_string();
            consume(':');
            std::string v = parse_string();
            out[k] = v;
            consume(',');
            skip_ws();
        }
        consume('}');
    }
};

// ============================================================================
// Load
// ============================================================================

bool load() {
    std::ifstream f(get_save_path());
    if (!f) {
        DEBUG_LOG("[SaveSystem] No save file found at %s", get_save_path().c_str());
        return false;
    }
    std::stringstream buf;
    buf << f.rdbuf();
    std::string src = buf.str();

    Parser p{src};

    std::string saved_scene;
    float player_x = -1.0f, player_y = -1.0f;
    std::vector<std::string> inventory_items;
    std::unordered_map<std::string, bool>        flags;
    std::unordered_map<std::string, int>         values;
    std::unordered_map<std::string, std::string> strings;
    std::unordered_map<std::string, Game::SceneState> scene_states;

    p.consume('{');
    while (!p.at_end() && !p.peek('}')) {
        p.skip_ws();
        std::string key = p.parse_string();
        p.consume(':');
        p.skip_ws();

        if (key == "scene") {
            saved_scene = p.parse_string();
        } else if (key == "player_x") {
            try { player_x = std::stof(p.parse_token()); } catch (...) {}
        } else if (key == "player_y") {
            try { player_y = std::stof(p.parse_token()); } catch (...) {}
        } else if (key == "inventory") {
            p.consume('['); p.skip_ws();
            while (!p.at_end() && !p.peek(']')) {
                p.skip_ws();
                if (!p.peek(']')) inventory_items.push_back(p.parse_string());
                p.consume(',');
                p.skip_ws();
            }
            p.consume(']');
        } else if (key == "flags") {
            p.parse_bool_map(flags);
        } else if (key == "values") {
            p.parse_int_map(values);
        } else if (key == "strings") {
            p.parse_string_map(strings);
        } else if (key == "scene_states") {
            p.consume('{'); p.skip_ws();
            while (!p.at_end() && !p.peek('}')) {
                std::string scene_name = p.parse_string();
                p.consume(':'); p.skip_ws();

                Game::SceneState snap;
                p.consume('{'); p.skip_ws();
                while (!p.at_end() && !p.peek('}')) {
                    std::string sub_key = p.parse_string();
                    p.consume(':'); p.skip_ws();
                    if (sub_key == "hotspots") {
                        p.parse_bool_map(snap.hotspot_enabled);
                    } else if (sub_key == "entities") {
                        p.parse_bool_map(snap.entity_visible);
                    } else {
                        p.skip_value();
                    }
                    p.consume(','); p.skip_ws();
                }
                p.consume('}');

                scene_states[scene_name] = snap;
                p.consume(','); p.skip_ws();
            }
            p.consume('}');
        } else {
            p.skip_value();
        }

        p.consume(',');
        p.skip_ws();
    }

    // Apply to game state (suppress save triggers during restore)
    g_is_loading = true;

    Game::g_state.flags        = std::move(flags);
    Game::g_state.values       = std::move(values);
    Game::g_state.strings      = std::move(strings);
    Game::g_state.scene_states = std::move(scene_states);

    Inventory::clear();
    for (const auto& item_id : inventory_items) {
        Inventory::add_item(item_id);
    }

    if (!saved_scene.empty()) {
        Scene::load_scene(saved_scene, "");

        // Override player position set by load_scene
        if (player_x >= 0.0f && Game::g_state.player_entity != ECS::INVALID_ENTITY) {
            auto* t = Game::g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(
                Game::g_state.player_entity);
            if (t) { t->position.x = player_x; t->position.y = player_y; }
        }
    }

    g_is_loading = false;
    DEBUG_INFO("[SaveSystem] Loaded from %s", get_save_path().c_str());
    return true;
}

// ============================================================================
// Public API
// ============================================================================

void init() {
    g_pending_timer = -1.0f;
    g_is_loading    = false;
}

void update(float delta_time) {
    if (g_pending_timer > 0.0f) {
        g_pending_timer -= delta_time;
        if (g_pending_timer <= 0.0f) {
            save();
        }
    }
}

bool has_save() {
    std::ifstream f(get_save_path());
    return f.good();
}

void clear_save() {
    std::remove(get_save_path().c_str());
    DEBUG_INFO("[SaveSystem] Save cleared");
}

void schedule_save() {
    if (g_is_loading) return;
    if (g_pending_timer < 0.0f) {
        g_pending_timer = DEBOUNCE_INTERVAL;
    }
}

}  // namespace SaveSystem
