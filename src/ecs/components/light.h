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
struct ProjectorLightComponent {
    Vec3 direction = Vec3(0.0f, 0.0f, -1.0f);  // Direction light travels (normalized)
    Vec3 up = Vec3(0.0f, 1.0f, 0.0f);          // Up vector for cookie orientation
    Vec3 color = Vec3(1.0f, 0.95f, 0.8f);      // Warm sunlight color
    float intensity = 1.5f;                     // Brightness multiplier
    float fov = 60.0f;                          // Field of view in degrees
    float aspect_ratio = 1.0f;                  // Cookie width/height
    float range = 2.0f;                         // How far light reaches (OpenGL units)
    Renderer::TextureID cookie = 0;             // Cookie texture (grayscale mask)
    bool enabled = true;
};

} // namespace ECS
