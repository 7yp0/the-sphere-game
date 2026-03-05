#pragma once

#include "types.h"
#include "core/animation_bank.h"
#include <cstdint>
#include <vector>

namespace Scene {

struct Prop {
};

struct Scene {
    std::string name;
    uint32_t width;
    uint32_t height;
};

struct SceneManager {
    Scene current_scene;
};

void init_scene_test();

}
