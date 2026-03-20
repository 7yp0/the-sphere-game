#include "scene.h"
#include "scene_registry.h"
#include "game/game.h"
#include "renderer/renderer.h"
#include "debug/geometry_editor.h"
#include "config.h"
#include "ecs/ecs.h"
#include "ecs/entity_factory.h"

using Game::g_state;

namespace Scene {

void init_scene_basement() {
    Scene scene;
    scene.name   = "basement";
    scene.width  = Config::BASE_WIDTH;
    scene.height = Config::BASE_HEIGHT;

    scene.background = Renderer::load_texture("scenes/basement/backgrounds/background.png");
    scene.foreground = Renderer::load_texture("scenes/basement/backgrounds/foreground.png");

    g_state.scene = scene;

    GeometryEditor::load_geometry(scene.name.c_str());

    // =========================================================================
    // Props
    // =========================================================================
    Renderer::TextureID sphere_tex = Renderer::load_texture("scenes/basement/props/sphere.png");
    ECS::EntityID sphere = ECS::create_static_prop(
        Vec2(160.0f, 130.0f),
        Vec2(83.0f, 65.0f),
        sphere_tex,
        0,
        PivotPoint::BOTTOM_CENTER
    );
    g_state.scene.prop_entities.push_back(sphere);
    register_entity("sphere", sphere);

    g_state.scene.light_entities.clear();

    // Warm center light (SHADOW CASTING - main light)
    ECS::EntityID top_light = ECS::create_point_light(
        Vec3(0.0f, 0.0f, -0.5f),     // Center screen, slightly in front (OpenGL coords)
        Vec3(1.0f, 0.9f, 0.7f),      // Warm white color
        1.5f,                         // Intensity
        2.0f,                         // Radius (OpenGL units)
        true                          // Casts shadows
    );
    g_state.scene.light_entities.push_back(top_light);
    register_entity("top_light", top_light);

    g_state.scene.projector_light_entities.clear();

    // Load the window cookie texture
    Renderer::TextureID window_cookie = Renderer::load_texture("lights/window_cookie.png");

    // Create window light on the left wall
    // Position higher up, projecting diagonally right and down onto the floor
    ECS::EntityID window_light = ECS::create_projector_light(
        Vec2(20.0f, 35.0f),          // Left side, higher up (pixels)
        0.0f,                        // Z depth (near camera, like a window on the wall)
        Vec3(1.0f, -0.9f, 0.0f),    // Direction: pointing right (+X) and into room (+Z)
        Vec3(0.0f, 1.0f, -0.25f),      // Up: pointing up (+Y)
        Vec3(1.0f, 0.95f, 0.85f),    // Warm sunlight color
        2.5f,                         // Intensity
        20.0f,                        // FOV (narrower for cleaner pattern)
        1.0f,                         // Aspect ratio (square window)
        3.0f,                         // Range (in Z units, covers room depth)
        window_cookie
    );
    g_state.scene.projector_light_entities.push_back(window_light);
    register_entity("window_light", window_light);

    // =========================================================================
    // Hotspot callbacks
    // =========================================================================
    // register_hotspot_callback("...", []() { ... });

    GeometryEditor::load_entities(scene.name.c_str());
}

} // namespace Scene
