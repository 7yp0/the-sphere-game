# Rendering Architecture

This document describes the rendering, lighting, and shadow architecture of the engine.

## Rendering Overview

The engine renders a 2.5D scene consisting of:

- A pre-rendered 2D background image
- Sprite-based objects placed in 3D / 2.5D space
- Dynamic point lights affecting both background and objects

The engine follows a component-based architecture where scene entities may contain different components such as rendering, lighting, and shadow behavior.

## Entity Component Architecture

Scene elements are represented as entities composed of components.

Typical components include:

- `Transform2_5DComponent`
- `Transform3DComponent`
- `SpriteComponent`
- `LightComponent`
- `ShadowCasterComponent`
- `EmissiveComponent`

Entities may contain any combination of these components.

### Entity Examples

**Lamp entity:**

- Transform2_5DComponent
- Transform3DComponent
- SpriteComponent
- LightComponent
- ShadowCasterComponent (optional)

**Window light:**

- Transform2_5DComponent
- LightComponent

**Decorative object:**

- Transform2_5DComponent
- Transform3DComponent
- SpriteComponent

**Emissive object:**

- Transform2_5DComponent
- Transform3DComponent
- SpriteComponent
- EmissiveComponent

## Transform Systems

The engine distinguishes between two different transform systems.

### Transform2_5DComponent

This transform describes the entity's position within the game scene.

It represents the logical scene position used for:

- Gameplay positioning
- Depth ordering
- Lighting calculations
- Ray intersection tests
- Shadow evaluation

Coordinates are expressed in the scene's 2.5D space where objects are positioned relative to the background depth layout.

### Transform3DComponent

This transform describes the object's position in the actual OpenGL rendering space.

It is used for:

- GPU rendering
- Vertex transformation
- Camera projection
- Billboard alignment

In most cases the Transform3DComponent is derived from the Transform2_5DComponent during rendering.

Separating these two coordinate systems allows the engine to keep scene logic independent from the final OpenGL projection.

## Background Representation

The background is a 2D image accompanied by two auxiliary textures:

### Depth Map

- Encodes the approximate Z-depth of each background pixel
- White pixels (255) = near camera (Z=-1)
- Black pixels (0) = far from camera (Z=+1)
- Used to reconstruct the 3D position of background pixels in view space

The reconstructed position is used for:

- Computing direction vectors toward light sources
- Computing light attenuation based on distance

The depth map is NOT used as shadow-casting geometry.

### Normal Map

- Encodes the surface normal direction at each background pixel
- Used for diffuse lighting calculations (N · L)
- Allows the flat background to respond to dynamic lights as if it had actual surface geometry

### Background Shadow Behavior

The background receives lighting and can receive shadows from objects, but it does NOT cast shadows itself.

No ray marching is performed against the background depth map.

## Object Types

Renderable objects are typically provided by a SpriteComponent and rendered as quads placed in the scene (two triangles per quad).

Objects must be classified into two shadow categories:

1. **Shadow-Casting Objects** - Cast shadows onto other surfaces
2. **Non-Shadow-Casting Objects** - Do not cast shadows; behave like the background with respect to shadow casting

## Light Types

Lights in the scene must be classified into two categories:

1. **Shadow-Casting Lights** - Perform shadow evaluation against shadow-casting objects
2. **Non-Shadow-Casting Lights** - Contribute lighting without shadow calculations

## Visible Light Sources

Visible light sources are represented using composition.

A visible light source consists of:

- A LightComponent responsible for illumination
- An optional SpriteComponent or EmissiveComponent representing the visible source

The emissive sprite represents the visible glowing source of the light but does not participate in lighting calculations.

This allows:

- Lights without visible sources (e.g., moonlight, window light)
- Visible emissive objects without affecting scene lighting
- Objects that both emit light and are visible sources

## Light Radius and Influence

Each light has a radius defining the maximum distance at which the light affects surfaces.

Before evaluating lighting for a pixel:

1. Compute distance between surface point and light
2. If distance exceeds the radius, skip the light

## Visibility Determination

Before lighting is evaluated, the renderer determines which surface a screen pixel belongs to.

For each screen pixel:

1. Cast a ray from the camera into the scene
2. Test intersection against object quads
3. If an object is hit first → pixel belongs to the object
4. Otherwise → reconstruct background position from depth map

## Surface Data for Shading

### Background Pixels

- Position reconstructed from depth map
- Normal sampled from background normal map

### Object Pixels

- Hit position determined using ray/quad intersection
- Intersection computed in quad local coordinates
- World position reconstructed as:

```
P_hit = P_quad + offset_x * quad_right + offset_y * quad_up
```

- Surface normal sampled from the object's normal map

## Lighting Model

Lighting is accumulated from all relevant lights affecting the surface.

For each light:

1. Compute vector to light
2. Compute distance
3. Skip if outside radius
4. Compute diffuse lighting from surface normal

Each light contributes additively.

## Shadow Evaluation

Only Shadow-Casting Lights perform shadow evaluation.

For these lights:

1. Cast shadow ray from surface point toward light
2. Test intersections against Shadow-Casting Objects
3. If intersection occurs before reaching the light → surface is shadowed

### Self-Shadowing Rule

Objects must never cast shadows onto themselves.

When evaluating shadow rays:

- Ignore the object that produced the visible surface point
- Skip intersections with that object
- Apply a small normal bias to avoid numerical artifacts

## Object Intersection and Alpha Testing

When a shadow ray hits an object quad:

1. Compute hit position
2. Convert to UV coordinates
3. Sample object alpha
4. Only pixels above alpha threshold block light

This allows sprites with irregular silhouettes (e.g., trees, characters) to cast shadows that match their visible shape rather than their bounding quad.

## Limitations and Trade-offs

### Advantages

- **Performance**: No expensive ray marching through volumetric data
- **Simplicity**: Shadow casters are simple quad geometry
- **Accuracy**: Alpha testing produces pixel-accurate shadow silhouettes
- **Flexibility**: Background art can have complex painted depth and normals

### Limitations

- Background geometry cannot cast shadows onto itself or objects
- Self-shadowing within the background must be baked into the art
- Shadow casters are limited to quad geometry (no complex meshes)
- Soft shadows require additional techniques (not implemented)

This architecture is suited for 2.5D adventure games where the background is pre-rendered artwork and dynamic objects need to cast shadows onto that artwork.
