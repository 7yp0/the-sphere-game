#include "scene.h"
#include "scene_registry.h"
#include "game/game.h"
#include "debug/geometry_editor.h"
#include "config.h"

using Game::g_state;

namespace Scene {

void init_scene_test2() {
    g_state.scene.name = "test2";
    g_state.scene.width = Config::BASE_WIDTH;
    g_state.scene.height = Config::BASE_HEIGHT;

    // Load geometry from JSON (walkable areas + hotspots)
    GeometryEditor::load_geometry(g_state.scene.name.c_str());

    // Exit back to test scene
    register_hotspot_callback("exit_to_test", []() {
        load_scene("test", "from_test2");
    });
}

} // namespace Scene
