# Implementation Plan: ECS Architecture

This document outlines the step-by-step implementation of the entity-component architecture described in `architecture.md`.

## Current State Analysis

**Existing:**

- `Prop` struct (position, size, texture, normal_map, pivot)
- `PointLight` struct (position, color, intensity, radius)
- `Scene` struct (background, depth_map, props, lights)
- Basic lighting shader (`basic_lit.frag`)
- Depth map sampling for Z-depth
- Normal map support for lighting

**Missing:**

- Entity-Component System (ECS)
- Transform separation (2.5D vs 3D)
- Shadow casting in shaders
- Light/object shadow categories
- Visibility determination
- Self-shadowing prevention

---

## Phase 1: ECS Core Infrastructure

### 1.1 Entity System Foundation

**File:** `src/ecs/entity.h`

- [ ] Define `EntityID` type (uint32_t)
- [ ] Create `Entity` struct with ID and component bitfield
- [ ] Create `EntityManager` class
  - `create_entity()` → EntityID
  - `destroy_entity(EntityID)`
  - `is_valid(EntityID)` → bool

### 1.2 Component Registry

**File:** `src/ecs/component.h`

- [ ] Define component type enum
- [ ] Create component storage (contiguous arrays per component type)
- [ ] Implement `add_component<T>(EntityID)` → T&
- [ ] Implement `get_component<T>(EntityID)` → T*
- [ ] Implement `has_component<T>(EntityID)` → bool
- [ ] Implement `remove_component<T>(EntityID)`

---

## Phase 2: Transform Components

### 2.1 Transform2_5DComponent

**File:** `src/ecs/components/transform.h`

- [ ] Define `Transform2_5DComponent` struct:

  ```cpp
  struct Transform2_5DComponent {
      Vec2 position;      // Pixel coordinates in scene
      float z_depth;      // Computed from depth map or manual
      float rotation;     // Rotation in radians
      Vec2 scale;         // Scale factors
  };
  ```

- [ ] Add helper: `get_z_from_depth_map(pos)`
- [ ] Add helper: `compute_depth_scale(z_depth)`

### 2.2 Transform3DComponent

**File:** `src/ecs/components/transform.h`

- [ ] Define `Transform3DComponent` struct:

  ```cpp
  struct Transform3DComponent {
      Vec3 position;      // OpenGL coordinates (-1 to +1)
      Vec2 size;          // OpenGL size
      float rotation;     // Rotation in radians
  };
  ```

- [ ] Implement `derive_3d_from_2_5d()` conversion function
- [ ] Handle pivot point transformation

---

## Phase 3: Rendering Components

### 3.1 SpriteComponent

**File:** `src/ecs/components/sprite.h`

- [ ] Define `SpriteComponent` struct:

  ```cpp
  struct SpriteComponent {
      TextureID texture;
      TextureID normal_map;       // Optional (0 = none)
      Vec4 uv_range;              // For spritesheets
      PivotPoint pivot;
      bool visible;
  };
  ```

- [ ] Integrate with existing animation system

### 3.2 EmissiveComponent

**File:** `src/ecs/components/emissive.h`

- [ ] Define `EmissiveComponent` struct:

  ```cpp
  struct EmissiveComponent {
      Vec3 emissive_color;  // RGB, values > 1.0 for HDR glow
  };
  ```

- [ ] Update shader to handle emissive surfaces

**Note:** Emissive color encodes both hue and brightness. Use values > 1.0 for bright emissive surfaces (e.g., `Vec3(2.0, 1.5, 0.5)` for bright orange glow). This is distinct from `LightComponent.intensity` which controls how strongly the light illuminates *other* objects.

---

## Phase 4: Lighting Components

### 4.1 Remove Existing Lighting Code

- [ ] Delete `basic_lit.frag` shader completely
- [ ] Delete `basic_lit.vert` shader completely
- [ ] Remove all `render_sprite_lit()` functions from `renderer.h` and `renderer.cpp`
- [ ] Remove light uniform setup code from renderer
- [ ] Remove `PointLight` struct from `scene.h`
- [ ] Remove `lights` vector from `Scene` struct
- [ ] Clean up any remaining light-related code paths

### 4.2 LightComponent

**File:** `src/ecs/components/light.h`

- [ ] Define `LightComponent` struct:

  ```cpp
  struct LightComponent {
      Vec3 color;             // RGB (0-1)
      float intensity;
      float radius;
      bool casts_shadows;     // Shadow-casting flag
      bool enabled;
  };
  ```

---

## Phase 5: Visibility Determination and Surface Shading

**Note:** This phase replaces the existing lighting shader with the new architecture.
Visibility must be determined before lighting and shadows can be evaluated.

### 5.1 Camera Ray Casting

**File:** `src/renderer/shaders/basic_lit.frag`

- [ ] For each screen pixel, determine surface ownership:
  - Cast ray from camera into scene
  - Test intersection against object quads
  - If object hit first → pixel belongs to object
  - Otherwise → pixel belongs to background

### 5.2 Surface Data Extraction

- [ ] **Background pixels:**
  - Position reconstructed from depth map
  - Normal sampled from background normal map

- [ ] **Object pixels:**
  - Hit position from ray/quad intersection
  - World position: `P_hit = P_quad + offset_x * quad_right + offset_y * quad_up`
  - Normal sampled from object's normal map

### 5.3 Lighting Accumulation

- [ ] For each light:
  - Compute vector to light
  - Compute distance, skip if outside radius
  - Compute diffuse contribution from surface normal
  - Accumulate additively

---

## Phase 6: Shadow System

**Note:** Shadows are evaluated only for Shadow-Casting Lights.

### 6.1 ShadowCasterComponent

**File:** `src/ecs/components/shadow_caster.h`

- [ ] Define `ShadowCasterComponent` struct:

  ```cpp
  struct ShadowCasterComponent {
      bool enabled;
      float alpha_threshold;  // For alpha testing (default 0.3)
  };
  ```

### 6.2 Shadow Shader Implementation

**File:** `src/renderer/shaders/basic_lit.frag`

- [ ] Add shadow caster uniforms:

  ```glsl
  uniform int numShadowCasters;
  uniform vec3 shadowCasterPositions[MAX_SHADOW_CASTERS];
  uniform vec2 shadowCasterSizes[MAX_SHADOW_CASTERS];
  uniform float shadowCasterZDepths[MAX_SHADOW_CASTERS];
  uniform sampler2D shadowCasterTextures[MAX_SHADOW_CASTERS];
  uniform vec4 shadowCasterUVRanges[MAX_SHADOW_CASTERS];
  ```

- [ ] Implement `rayQuadIntersect()` function:

  ```glsl
  bool rayQuadIntersect(vec3 rayOrigin, vec3 rayDir, 
                        vec3 quadCenter, vec2 quadSize, float quadZ,
                        out vec2 hitUV);
  ```

- [ ] Implement `calculateShadow()` function:

  ```glsl
  float calculateShadow(vec3 fragPos, vec3 lightPos, int excludeIndex);
  ```

- [ ] Add alpha testing in shadow evaluation

### 6.3 Self-Shadowing Prevention

- [ ] Pass current fragment's entity index to shader
- [ ] Skip self-intersection in shadow loops
- [ ] Add configurable normal bias

---

## Phase 7: Integration and Migration

### 7.1 Migrate Existing Code

- [ ] Convert `Prop` to Entity with components
- [ ] Convert `PointLight` to Entity with components
- [ ] Update `Scene` to use entity lists
- [ ] Update `init_scene_test()` to create entities

### 7.2 Entity Factory Functions

**File:** `src/ecs/entity_factory.h`

- [ ] `create_static_prop(pos, texture, normal_map)` → EntityID
- [ ] `create_shadow_casting_prop(pos, texture, normal_map)` → EntityID
- [ ] `create_light(pos, color, radius, casts_shadows)` → EntityID
- [ ] `create_lamp(pos, sprite, light_params)` → EntityID (composite)
- [ ] `create_emissive_object(pos, texture, emissive_color)` → EntityID

### 7.3 Render Pipeline Integration

- [ ] Collect entities by component type each frame
- [ ] Sort renderables by z-depth
- [ ] Upload shadow caster data to GPU
- [ ] Upload light data (separated by shadow category)
- [ ] Execute render passes

---

## Phase 8: Testing and Validation

### 8.1 Unit Tests

- [ ] Entity creation/destruction
- [ ] Component add/remove/query
- [ ] Transform conversion (2.5D → 3D)

### 8.2 Visual Tests

- [ ] Shadow casting from props onto background
- [ ] Alpha-tested shadows (tree silhouettes)
- [ ] Self-shadowing prevention
- [ ] Mixed shadow-casting and non-shadow-casting lights
- [ ] Emissive objects

### 8.3 Performance Validation

- [ ] Measure frame time with varying entity counts
- [ ] Profile shadow evaluation overhead
- [ ] Optimize hot paths if needed

---

## File Structure (Proposed)

```
src/
├── ecs/
│   ├── entity.h
│   ├── entity.cpp
│   ├── component.h
│   ├── component.cpp
│   ├── entity_factory.h
│   ├── entity_factory.cpp
│   └── components/
│       ├── transform.h
│       ├── sprite.h
│       ├── light.h
│       ├── shadow_caster.h
│       └── emissive.h
├── renderer/
│   └── shaders/
│       └── basic_lit.frag  (rewritten)
└── scene/
    └── scene.h             (updated to use ECS)
```

---

## Implementation Order (Recommended)

1. **Phase 1** - ECS core (entity + component storage)
2. **Phase 2** - Transform components
3. **Phase 3** - Sprite + Emissive components
4. **Phase 4** - Light component with categories
5. **Phase 7.1** - Migrate existing Prop/PointLight
6. **Phase 5** - Visibility + Lighting (replaces existing shader)
7. **Phase 6** - Shadow system
8. **Phase 7.2-7.3** - Factories and pipeline integration
9. **Phase 8** - Testing

Total estimated effort: 3-5 development sessions depending on complexity.

---

## Notes

- Phase 5 replaces the existing `basic_lit.frag` shader completely
- Test incrementally after each phase
- Start with simple shadow casters before alpha testing
- Consider data-oriented design for cache efficiency
