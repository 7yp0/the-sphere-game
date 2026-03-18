#include "act_registry.h"
#include "scene_registry.h"
#include "../game/game.h"
#include "../inventory/inventory.h"
#include "../debug/debug_log.h"
#include <functional>
#include <string>
#include <unordered_map>

// Forward-declare each act's init function (defined in their own .cpp files)
namespace Scene {
    void init_act_1();
    // void init_act_2();
}

namespace Scene {

struct ActDef {
    std::string start_scene;
    std::string start_spawn;
    std::function<void()> init;
};

static std::unordered_map<int, ActDef> g_acts;

static void register_act(int number, const std::string& start_scene,
                         const std::string& start_spawn, std::function<void()> init) {
    g_acts[number] = { start_scene, start_spawn, std::move(init) };
}

void register_all_acts() {
    register_act(1, "test", "default", init_act_1);
    // register_act(2, "corridor", "default", init_act_2);
}

void load_act(int number) {
    using namespace Game;

    auto it = g_acts.find(number);
    if (it == g_acts.end()) {
        DEBUG_ERROR("[ActRegistry] Act %d not registered", number);
        return;
    }

    // Reset all persistent game state
    g_state.flags.clear();
    g_state.values.clear();
    g_state.strings.clear();
    g_state.scene_states.clear();
    g_state.scene.name.clear();  // prevent load_scene from snapshotting stale live state

    // Clear inventory (items, not definitions)
    Inventory::clear();

    // Run act initialisation (sets initial flags, gives starting items, etc.)
    it->second.init();

    // Load the act's starting scene
    load_scene(it->second.start_scene, it->second.start_spawn);

    DEBUG_INFO("[ActRegistry] Loaded Act %d", number);
}

} // namespace Scene
