#pragma once

#include "types.h"

namespace ECS {

// ShadowCasterComponent: Marks an entity as a shadow caster
// Used with Transform2_5DComponent and SpriteComponent
//
// When a light casts shadows, rays are traced from each surface point
// towards the light. If a ray intersects a shadow caster's quad,
// the shadow caster's texture is sampled at the intersection point.
// If the alpha value is above alpha_threshold, the surface is shadowed.
//
// NOTES:
// - Only entities with ShadowCasterComponent cast shadows on OTHER objects
// - The background itself does NOT cast shadows (it only receives them)
// - Self-shadowing is prevented by excluding the current entity from tests
struct ShadowCasterComponent {
    bool enabled = true;                   // Whether shadow casting is active
    float alpha_threshold = 0.3f;          // Alpha above this casts shadow
    float shadow_intensity = 0.7f;         // How dark shadows are (0=black, 1=no shadow effect)
};

} // namespace ECS
