#pragma once

#include <string>
#include <functional>

namespace Scene {

// Signature for scene init functions.
// Sets up textures, ECS entities, hotspot callbacks — but does NOT create the player.
using SceneInitFn = std::function<void()>;

// Register a scene init function by name. Called once at startup.
void register_scene(const std::string& name, SceneInitFn fn);

// Register all known scenes. Call once during game init.
void register_all_scenes();

// Tear down the current scene, run the named scene's init function,
// then spawn the player at spawn_point_name.
// Falls back to scene center if spawn_point_name is empty or not found.
void load_scene(const std::string& scene_name,
                const std::string& spawn_point_name = "default");

} // namespace Scene
