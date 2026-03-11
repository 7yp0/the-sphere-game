#pragma once

// All ECS component structures
// Each component type is defined in its own header for clarity

#include "../../types.h"
#include "../../renderer/renderer.h"
#include "transform.h"  // Transform2_5DComponent, Transform3DComponent
#include "sprite.h"     // SpriteComponent, EmissiveComponent

namespace ECS {

// LightComponent - Point light illumination
// Expanded in Phase 4
struct LightComponent {
    Vec3 color;             // RGB (0-1)
    float intensity;
    float radius;
    bool casts_shadows;     // Shadow-casting flag
    bool enabled;
    
    LightComponent() 
        : color(1, 1, 1), intensity(1), radius(1), 
          casts_shadows(false), enabled(true) {}
};

// ShadowCasterComponent - Shadow casting behavior
// Expanded in Phase 6
struct ShadowCasterComponent {
    bool enabled;
    float alpha_threshold;  // For alpha testing (default 0.3)
    
    ShadowCasterComponent() 
        : enabled(true), alpha_threshold(0.3f) {}
};

} // namespace ECS
