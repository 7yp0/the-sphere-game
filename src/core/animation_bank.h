#pragma once

#include "renderer/animation.h"
#include <map>
#include <string>

namespace Core {

struct AnimationBank {
    std::map<std::string, Renderer::SpriteAnimation> animations;
    
    void add(const std::string& name, const Renderer::SpriteAnimation& anim) {
        animations[name] = anim;
    }
    
    Renderer::SpriteAnimation* get(const std::string& name) {
        auto it = animations.find(name);
        if (it != animations.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    bool has(const std::string& name) const {
        return animations.find(name) != animations.end();
    }
    
    void clear() {
        animations.clear();
    }
};

}
