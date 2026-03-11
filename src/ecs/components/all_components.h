#pragma once

// All ECS component structures
// These are defined here for Phase 1, will be expanded in Phases 2-4

#include "../../types.h"
#include "../../renderer/renderer.h"

namespace ECS {

// Transform2_5DComponent - Position in game scene (2.5D space)
// Expanded in Phase 2
struct Transform2_5DComponent {
    Vec2 position;      // Pixel coordinates in scene
    float z_depth;      // Computed from depth map or manual
    float rotation;     // Rotation in radians
    Vec2 scale;         // Scale factors
    
    Transform2_5DComponent() 
        : position(0, 0), z_depth(0), rotation(0), scale(1, 1) {}
};

// Transform3DComponent - Position in OpenGL render space
// Expanded in Phase 2
struct Transform3DComponent {
    Vec3 position;      // OpenGL coordinates (-1 to +1)
    Vec2 size;          // OpenGL size
    float rotation;     // Rotation in radians
    
    Transform3DComponent() 
        : position(0, 0, 0), size(1, 1), rotation(0) {}
};

// SpriteComponent - Visual representation
// Expanded in Phase 3
struct SpriteComponent {
    Renderer::TextureID texture;
    Renderer::TextureID normal_map;   // Optional (0 = none)
    Vec4 uv_range;                    // For spritesheets (minU, minV, maxU, maxV)
    PivotPoint pivot;
    bool visible;
    
    SpriteComponent() 
        : texture(0), normal_map(0), uv_range(0, 0, 1, 1), 
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
