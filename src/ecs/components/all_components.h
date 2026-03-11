#pragma once

// All ECS component structures
// Each component type is defined in its own header for clarity

#include "../../types.h"
#include "../../renderer/renderer.h"
#include "transform.h"      // Transform2_5DComponent, Transform3DComponent
#include "sprite.h"         // SpriteComponent, EmissiveComponent
#include "light.h"          // LightComponent
#include "shadow_caster.h"  // ShadowCasterComponent

namespace ECS {

// Component types are defined in their respective headers:
// - Transform2_5DComponent (transform.h)
// - Transform3DComponent (transform.h)
// - SpriteComponent (sprite.h)
// - EmissiveComponent (sprite.h)
// - LightComponent (light.h)
// - ShadowCasterComponent (shadow_caster.h)

} // namespace ECS
