#pragma once

#include "types.h"

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

} // namespace ECS
