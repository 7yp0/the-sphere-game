#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include <cstdint>
#include <vector>
#include <string>

namespace Scene {

struct Prop {
    Vec2 position;
    Vec2 size;
    Renderer::TextureID texture;
    std::string name;
    PivotPoint pivot = PivotPoint::TOP_LEFT;
};

struct Scene {
    std::string name;
    uint32_t width;
    uint32_t height;
    Renderer::TextureID background;
    std::vector<Prop> props;
};

struct SceneManager {
    Scene current_scene;
};

void init_scene_test();

}
