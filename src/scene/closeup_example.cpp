#include "scene.h"
#include "close_up.h"
#include "scene_registry.h"
#include "../game/game.h"
#include "../renderer/renderer.h"
#include "../renderer/texture_loader.h"
#include "../debug/geometry_editor.h"
#include "../config.h"

using Game::g_state;

namespace Scene {

void init_closeup_example() {
    Scene scene;
    scene.name  = "closeup_example";
    scene.width  = Config::BASE_WIDTH;
    scene.height = Config::BASE_HEIGHT;

    // TODO: replace with actual close-up background
    scene.background = Renderer::load_texture("scenes/test/backgrounds/bg_room.png");

    g_state.scene = scene;

    // Load geometry from JSON if it exists (editor-placed hotspots, etc.)
    GeometryEditor::load_geometry(scene.name.c_str());

    // -------------------------------------------------------------------------
    // "Zurück" — IMMEDIATE hotspot covering the entire viewport.
    // Clicking anywhere exits the close-up and returns to the previous scene.
    // -------------------------------------------------------------------------
    Hotspot back;
    back.name             = "back";
    back.tooltip_key      = "Zurück";
    back.interaction_type = InteractionType::IMMEDIATE;
    back.bounds = Collision::Polygon({
        Vec2(0.0f,                        0.0f),
        Vec2((float)Config::BASE_WIDTH,   0.0f),
        Vec2((float)Config::BASE_WIDTH,   (float)Config::BASE_HEIGHT),
        Vec2(0.0f,                        (float)Config::BASE_HEIGHT),
    });
    back.callback = []() {
        exit_close_up();
    };
    g_state.scene.geometry.hotspots.push_back(back);
}

} // namespace Scene
