#pragma once

// All ECS component structures
// Each component type is defined in its own header for clarity

#include "../../types.h"
#include "../../renderer/renderer.h"
#include "transform.h"  // Transform2_5DComponent, Transform3DComponent

namespace ECS {

// SpriteComponent - Visual representation
// Expanded in Phase 3
struct SpriteComponent {
    Renderer::TextureID texture;
    Renderer::TextureID normal_map;   // Optional (0 = none)
    Vec2 base_size;                   // Base sprite size in pixels
    Vec4 uv_range;                    // For spritesheets (minU, minV, maxU, maxV)
    PivotPoint pivot;
    bool visible;
    
    SpriteComponent() 
        : texture(0), normal_map(0), base_size(32, 32), uv_range(0, 0, 1, 1), 
          pivot(PivotPoint::CENTER), visible(true) {}
};

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

// EmissiveComponent - Self-illumination
// Expanded in Phase 3
struct EmissiveComponent {
    Vec3 emissive_color;    // RGB, values > 1.0 for HDR glow
    
    EmissiveComponent() 
        : emissive_color(0, 0, 0) {}
};

} // namespace ECS
