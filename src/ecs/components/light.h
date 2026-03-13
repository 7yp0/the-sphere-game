#pragma once

#include "types.h"
#include "renderer/texture_loader.h"

namespace ECS {

// LightComponent: Properties for point light illumination
// Used with Transform3DComponent for 3D positioning in OpenGL space
//
// COORDINATE SYSTEM (ALL IN OPENGL SPACE -1 to +1):
//   X = horizontal screen position (-1=left, +1=right)
//   Y = vertical screen position (-1=bottom, +1=top)
//   Z = depth into screen (-1=near camera, +1=far/background)
//
// Shadow behavior:
//   Shadows are cast by scene objects (quads) via ray intersection tests.
//   Shadow direction depends on relative positions of light and caster.
struct LightComponent {
    Vec3 color = Vec3(1.0f, 1.0f, 1.0f);   // RGB color (0-1 range)
    float intensity = 1.0f;                  // Brightness multiplier
    float radius = 1.0f;                     // Attenuation radius in OpenGL units (0-4 typical)
    bool casts_shadows = true;               // Whether this light casts shadows
    bool enabled = true;                     // Whether this light is active
};

// ProjectorLightComponent: Directional light that projects a cookie texture
// Simulates light through windows, stained glass, etc.
// Position is set via Transform3DComponent (where the window is)
//
// The light projects from position in the given direction, creating a
// light pattern on surfaces based on the cookie texture.
//
// EDITOR USAGE:
//   1. Set position (Transform3DComponent) = where the window/light source is
//   2. Cmd+Click (Mac) / Ctrl+Click (Win) somewhere in 2.5D space
//   3. Direction is auto-calculated: clicked point → OpenGL coords → normalize(target - position)
//
// PROPERTIES:
//   - direction: Normalized vector where light points (auto-set via editor click)
//   - fov: How wide the light cone spreads (like camera lens)
//         Small (30°) = narrow spotlight, Large (90°) = wide floodlight
//   - up: Which direction is "up" for the cookie texture rotation
//         Default (0,1,0) = cookie's top edge points screen-up
//   - aspect_ratio: Stretches the cookie pattern (width/height)
//         1.0 = square, 2.0 = twice as wide as tall
struct ProjectorLightComponent {
    // === DIRECTION (Set via Editor Cmd/Ctrl+Click) ===
    Vec3 direction = Vec3(0.0f, 0.0f, -1.0f);  // Normalized direction vector
    float target_distance = 0.5f;               // Distance to clicked target (for editor visualization)
    
    // === APPEARANCE ===
    Vec3 color = Vec3(1.0f, 0.95f, 0.8f);      // Light color (warm sunlight default)
    float intensity = 1.5f;                     // Brightness multiplier
    float range = 2.0f;                         // How far light reaches (OpenGL units, 0-4 typical)
    
    // === PROJECTION SHAPE ===
    float fov = 60.0f;                          // Cone width in degrees (30=narrow, 90=wide)
    float aspect_ratio = 1.0f;                  // Cookie stretch (1.0=square, 2.0=wide)
    Vec3 up = Vec3(0.0f, 1.0f, 0.0f);          // Cookie "up" direction (usually leave default)
    
    // === RESOURCES ===
    Renderer::TextureID cookie = 0;             // Cookie texture (light pattern/mask)
    bool enabled = true;
};

} // namespace ECS
