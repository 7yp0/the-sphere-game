#pragma once

// All ECS component structures
// Each component type is defined in its own header for clarity

#include "../../types.h"
#include "../../renderer/renderer.h"
#include "transform.h"  // Transform2_5DComponent, Transform3DComponent
#include "sprite.h"     // SpriteComponent, EmissiveComponent
#include "light.h"      // LightComponent

namespace ECS {

// ShadowCasterComponent - Shadow casting behavior
// Expanded in Phase 6
struct ShadowCasterComponent {
    bool enabled;
    float alpha_threshold;  // For alpha testing (default 0.3)
    
    ShadowCasterComponent() 
        : enabled(true), alpha_threshold(0.3f) {}
};

} // namespace ECS
