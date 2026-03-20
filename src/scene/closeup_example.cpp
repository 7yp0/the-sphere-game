#include "scene.h"
#include "scene_registry.h"
#include "../game/game.h"
#include "../renderer/renderer.h"
#include "../debug/geometry_editor.h"
#include "../config.h"

using Game::g_state;

namespace Scene {

// Sets up the close-up scene visuals and geometry.
// Puzzle logic is configured by whoever calls enter_close_up() (e.g. scene_test.cpp).
void init_closeup_example() {
    Scene scene;
    scene.name   = "closeup_example";
    scene.width  = Config::BASE_WIDTH;
    scene.height = Config::BASE_HEIGHT;

    // TODO: replace with actual close-up background
    scene.background = Renderer::load_texture("scenes/test/backgrounds/bg_room.png");

    g_state.scene = scene;

    GeometryEditor::load_geometry(scene.name.c_str());
}

} // namespace Scene
