# The Sphere Game - TODO

## Phase 1: Sprite Loading & Rendering (COMPLETE)

- [x] **Texture & Asset System**
  - [x] PNG loader with Zlib decompression + all 5 filter types
  - [x] `Renderer::load_texture(const char* path)` → returns TextureID
  - [x] `Renderer::bind_texture(TextureID)` - optional utility
  - [x] Texture caching (don't load same texture twice)
  - [x] Handle PNG loading with build/assets/ layout

- [x] **Sprite Renderer**
  - [x] `Renderer::render_sprite(TextureID tex, Vec2 pos, Vec2 size)` - variable position/size
  - [x] OpenGL shaders in src/renderer/shaders/
  - [x] Shaders support texture sampling
  - [x] Test PNG rendering (blue quad)

- [x] **Animation System (Phase 1.5)**
  - [x] `SpriteAnimation` struct (frames, timing, current_frame)
  - [x] `Renderer::create_animation(TextureID[])` - from texture array
  - [x] `Renderer::animate(SpriteAnimation*, delta_time)` - update frame
  - [x] `Renderer::render_sprite_animated(SpriteAnimation*, Vec2 pos, Vec2 size)` - draw current frame
  - [x] Test animated sprite (cycle through red/green/blue frames)
  - [x] Layer system with z_depth and parallax factors
  - [x] Separate visual_update() from game logic update()
  - [x] Z-depth implementation with GL_DEPTH_TEST enabled
  - [x] `Layers::get_z_depth()` conversion from Layer enum to OpenGL range [-1, 1]

- [x] **Sprite Maps / Spritesheets (Phase 1.6)**
  - [x] UV mapping system for sprite frames within single texture
  - [x] `SpriteFrame` struct with UV coordinates (u0, v0, u1, v1)
  - [x] Updated `SpriteAnimation` to use sprite map texture + UV coords instead of separate TextureIDs
  - [x] Created test sprite map (player_spritesheet.png with 4 frames: idle + 3 walk)
  - [x] `Renderer::render_sprite_animated()` updated to use UV mapping from sprite map
  - [x] Updated `create_animation()` API to take texture + SpriteFrame array
  - [x] Reduces texture bindings (1 binde pro animation statt 1 binde pro frame), improves memory layout

- [x] **Test Assets**
  - [x] test.png (64x64 blue)
  - [x] Verify loading and rendering works

---

## Phase 2: Core Input & Movement (COMPLETE)

- [x] **Input System**
  - [x] Platform abstraction for mouse input - `Platform::get_mouse_pos()`, `Platform::mouse_clicked()`
  - [x] macOS implementation with NSTrackingArea for mouseMoved events
  - [x] Mouse click detection on mouseDown

- [x] **Player Entity**
  - [x] Create `Game::Player` struct (position, target_position, speed, texture)
  - [x] Point-and-click movement to target position
  - [x] Simple linear walking movement with distance check

- [x] **Basic Game Loop**
  - [x] Load test sprite for player (uses blue texture)
  - [x] Render player at Layer::PLAYER
  - [x] Test movement on click - player walks towards clicked position

**Known limitations (for Phase 6+):**

- Mouse coordinates are in window space (pixels), not normalized world space
- Need coordinate conversion: screen pixels → normalized [-1, 1] range for rendering
- Window dimensions must be tracked for proper conversion
- Will fix in Phase 6 when implementing camera system

---

## Phase 3: Scene & Game World (COMPLETE)

- [x] **Scene System**
  - [x] `Game::Scene` struct with Props array
  - [x] `Game::Prop` struct with pixel coordinates
  - [x] Render background texture
  - [x] Render all props in scene with correct z-depth

- [x] **Pixel Coordinate System**
  - [x] Central `Coords::pixel_to_opengl()` helper
  - [x] All Props use pixel coordinates (not normalized)
  - [x] Automatic conversion in render pipeline

- [x] **Debug Overlay**
  - [x] F1 toggle for debug display
  - [x] Show mouse position in pixel coordinates
  - [x] Platform::key_pressed() for input handling

- [x] **Test Scene**
  - [x] Load background (bg_field.png)
  - [x] Place test props (tree, stone, chest)
  - [x] Pixel-based placement system

---

## Phase 3.5: Pixel-Perfect Rendering & 2.5D Depth Scaling (COMPLETE)

- [x] **Pixel-Perfect Rendering & Scaling**
  - [x] Define base resolution (336x189) where game logic lives
  - [x] Support any window resolution (672x378 viewport)
  - [x] Float-based scaling: `scale_factor = viewport_height / BASE_HEIGHT` (GPU handles upscaling smoothly)
  - [x] No integer-scaling requirement: allows flexible window sizes
  - [x] Scene stores `HorizonLine` structs with y_position and scale_gradient per horizon

- [x] **Depth-Based Character Scaling** (2.5D effect)
  - [x] Calculate character scale based on Y position relative to horizon
  - [x] Formula: `scale = 1.0 + (sprite_y - horizon_y) * scale_gradient`
  - [x] Above horizon (smaller Y) → scale down (away from camera)
  - [x] Below horizon (larger Y) → scale up (closer to camera)
  - [x] Character scale range: `0.01` (minimum) to `2.0` (maximum when scaled up)
  - [x] Multi-horizon zones: Between horizons = no scaling (scale = 1.0)
  - [x] Configurable `scale_gradient` per horizon line for flexible scaling rates
  - [x] Inverted mode support for isometric/top-down perspectives
  - [x] Applied in render pipeline via `render_sprite_animated_with_depth()`

---

## Phase 4: Collision & Interaction

### 4.1 Polygon-Based Collision System (Foundation)

- [x] **Generic Polygon Utilities**
  - [x] `Polygon` struct: array of 2D points (Vec2), closed path
  - [x] `point_in_polygon()` - point-in-polygon test (used for: click within hotspot, point within walkable area)
  - [x] `closest_point_on_polygon()` - find nearest point on polygon boundary (used for: clamp target to walkable area edge)
  - [x] `line_intersects_polygon()` - path validation (used for: check if walk path crosses obstacle)
  - [x] `polygon_collision()` - polygon-vs-polygon (used for: obstacle detection)
  - [x] Location: `src/collision/polygon_utils.h/cpp` or `src/renderer/polygon_utils.h/cpp`

- [x] **Scene Geometry**
  - [x] `Game::SceneGeometry` struct contains:
    - `Polygon[] walkable_areas` - player can walk here (outer boundary + inner holes as separate polygons)
    - `Hotspot[] hotspots` - interactive regions (also polygons with callbacks)
  - [x] All stored in Scene
  - [x] Serialization: JSON format with polygon vertex arrays
  - [x] Render all polygons (walkable/hotspots) with different colors (D toggle)
  - [x] Display polygon vertices
  - [x] Show closest point on boundary when hovering/clicking

### 4.2 Walkable Areas & Pathfinding

- [x] **Player Movement with Collision**
  - [x] On click: validate target point with `point_in_walkable_areas(target, walkable_areas[])`
  - [x] If outside all walkable areas: use `closest_point_on_any_polygon(target, walkable_areas[])` as actual destination
  - [x] Use `line_intersects_walkable(path_start, path_end, walkable_areas[])` to validate walk path
  - [x] On collision: slide along polygon edge (smooth wall following)

- [x] **Pathfinding System** (Ron Gilbert approach)
  - [x] When target is outside walkable areas: find closest valid point on any area boundary
  - [x] When target is inside any walkable area: move directly to target
  - [x] Player slides along polygon edges when path clips boundary
  - [x] Implementation: project target→closest-point, walk there, continue
  - [x] Holes in walkable area = separate negative polygons in walkable_areas[] array

- [x] **No Separate Obstacle System**
  - [x] Holes in walkable area = separate negative polygons in walkable_areas[] array
  - [x] "Not walkable" = "not in any walkable_areas polygon" (simplifies logic)
  - [x] pathfinding validates all moves against all walkable_areas polygons

### 4.3 Hotspot System (Point-and-Click Interaction)

- [x] **Hotspot Definition**
  - [x] `Game::Hotspot` struct with: polygon (bounds), interaction_distance, enabled flag, callback
  - [x] Hotspots are polygons stored in SceneGeometry
  - [x] Each hotspot has a callback function pointer
  - [x] Interaction distance configurable per hotspot (0 = on boundary, N = range in pixels)

- [x] **Hotspot Click Detection**
  - [x] On mouse click: use `point_in_polygon(click_pos, hotspot.polygon)` for all hotspots
  - [x] For each hotspot hit: check if player can reach its interaction distance
  - [x] If reachable: player walks to closest walkable point within `interaction_distance` of hotspot, then triggers callback
  - [x] If unreachable: don't walk, trigger callback immediately to show feedback ("Can't reach")
  - [x] If player already within interaction distance: trigger action immediately (no walk needed)

- [x] **Interaction Flow**
  - [x] Player clicks hotspot → validate against walkable_areas[] to find reachable destination
  - [x] Player walks there (pathfinding against walkable_areas) → trigger hotspot callback
  - [x] Hotspot callback handles game logic (dialogue, item pickup, state change, etc.)
  - [x] Multiple hotspots per prop allowed (e.g., "look at tree", "climb tree")

### 4.4 Player State & Depth

- [x] **Y-Sorting for Z-Depth**
  - [x] Collect all entities (Player + Props) with visual base Y position
  - [x] Sort by visual base Y (account for Pivot Points: TOP_LEFT, BOTTOM_CENTER, etc.)
  - [x] Dynamic z-depth assignment based on sort order (-1 to +1 range)
  - [x] Correct rendering order: behind/in-front based on screen depth ✅
  - [x] Props don't interfere with correct layering
  - [x] Hotspots don't affect rendering (invisible, only interactive)

- [x] **Alpha-Test Depth Correction (Pixel-Perfect Layering)**
  - [x] Modified GLSL fragment shader with alpha discard
  - [x] If alpha < 0.5: discard pixel (transparent parts don't write depth)
  - [x] Enables proper depth testing with transparent pixels
  - [x] Player can now render behind semi-transparent prop areas
  - [x] No more visual glitches at transparent asset boundaries ✅

---

## Phase 4.5: Polygon Editor (Debug Geometry Tool) ✅

### 4.5.1 Debug Polygon Visualization & Editing

- [x] **D-Key Toggle Debug Mode**
  - [x] Show all walkable_areas polygons (green with alpha 0.7)
  - [x] Show all hotspots polygons (red with alpha 0.7)
  - [x] Show polygon vertices (small rectangles at each point, clickable)
  - [x] Show polygon edges (lines connecting vertices)
  - [x] Highlight selected polygon (brighter color + thicker lines)
  - [x] Highlight selected vertex (cyan rectangle, larger size)

- [x] **Polygon Creation (In Debug Mode)**
  - [x] Press 'W' to start creating new walkable_area
  - [x] Press 'H' to start creating new hotspot
  - [x] Click to add vertices to current polygon (LMB)
  - [x] Polygon auto-closes when clicking first vertex again (visual: green highlight)
  - [x] OR press 'F' to manually finish/close polygon
  - [x] Visual feedback: yellow preview line from last vertex to mouse cursor

- [x] **Polygon Editing (Drag & Select)**
  - [x] Click vertex to select it + polygon (highlighted)
  - [x] Drag selected vertex to move it (real-time update)
  - [x] Click on edge to insert new vertex at click position (immediately draggable)
  - [x] Right-click vertex to delete it (minimum 3 vertices maintained)
  - [x] Press 'DEL' to delete entire selected polygon
  - [x] ESC to deselect current polygon/cancel creation
  - [x] W/H keys deselect current polygon when starting new creation

- [x] **Polygon Mode Display**
  - [x] Shows current mode: "[SELECT]", "[CREATING WALKABLE]", or "[CREATING HOTSPOT]"

### 4.5.2 Scene Geometry JSON Format

- [x] **Per-Scene Geometry File**
  - [x] Location: `assets/scenes/<scene_name>/geometry.json`
  - [x] Example: `assets/scenes/test/geometry.json`
  - [x] Load on scene startup via `Debug::load_scene_geometry()`

- [x] **Automatic Geometry Saving**
  - [x] Every change auto-saves to geometry.json immediately
  - [x] No manual save command needed
  - [x] Pretty-printed JSON format

- [x] **Load Geometry from JSON**
  - [x] Parse walkable_areas[] with points array
  - [x] Parse hotspots[] with points, name, interaction_distance, enabled
  - [x] Populate Scene::geometry on load

### 4.5.3 Hole/Obstacle System

- [x] **Nested Walkable Areas as Obstacles**
  - [x] Walkable area completely inside another = hole/obstacle
  - [x] Uses odd/even rule: point in odd # of polygons = walkable
  - [x] `point_in_walkable_area()` handles this automatically
  - [x] Example: Large room polygon + smaller table polygon inside = player walks around table

### 4.5.4 Keyboard & Mouse Reference

| Input | Action |
|-------|--------|
| `D` | Toggle debug overlay & geometry editor |
| `W` | Start creating new walkable_area polygon |
| `H` | Start creating new hotspot polygon |
| `F` | Finish/close current polygon |
| `ESC` | Cancel creation / Deselect polygon |
| `DEL` | Delete selected polygon |
| Left-click vertex | Select vertex (enables dragging) |
| Left-drag vertex | Move vertex position |
| Left-click edge | Insert new vertex at click position |
| Right-click vertex | Delete vertex (keeps min 3) |
| Click first vertex | Close polygon (while creating) |

*All changes auto-save to `assets/scenes/<scene>/geometry.json`*

### 4.5.5 walkable component

- [x] player isn't the only objekt that can move/walk (but only one which is controllable) + the code is maybe a bit zu umständlich geschrieben? think about it!

### 4.5.6 how to configure hotspot callbacks

- [x] hotspots are clickable areas for objects in the 2.5D room. So when I click on a hotspot, I expect that the player is moving to the object (x y Z) it belongs to
- [ ] hotspots polygon geometry is now saved in the geometry json. but how do we define the callbacks? (same for enabled and distance)
- [ ] scene.cpp still defines polygons like walkable areas and hotspots, while they are not used and come from the geometry.json

---

## Phase 5: 2.5D Lighting (COMPLETE)

### 5.1 Foundation: Light & Shadow Data ✅

- [x] **ECS-Based Light System**
  - [x] `PointLightComponent`: position (3D), radius, intensity, color (RGB), casts_shadows flag
  - [x] Light falloff: quadratic with smooth attenuation in shader
  - [x] Lights stored as ECS entities in scene
  - [x] Max 8 lights per scene (MAX_LIGHTS in shader)
  - [x] Light on/off via `casts_shadows` flag

- [x] **Entity Factories for Lights**
  - [x] `create_point_light()` - OpenGL coords
  - [x] `create_point_light_at_pixel()` - pixel coords with auto-conversion
  - [x] `create_lamp()` - composite: sprite + light together
  - [x] `create_emissive_object()` - glowing objects

### 5.2 Ray-Quad Shadow System ✅

- [x] **Ray-Quad Intersection (NOT projection-based!)**
  - [x] Cast ray from fragment towards light source
  - [x] Test ray against all shadow caster quads (props)
  - [x] If intersection: sample shadow caster texture at hit UV
  - [x] Alpha test: if alpha > threshold → in shadow
  - [x] Shadow intensity configurable per caster

- [x] **Shadow Caster ECS Components**
  - [x] `ShadowCasterComponent`: enabled, alpha_threshold, shadow_intensity
  - [x] Entity factories: `create_shadow_casting_prop()` vs `create_static_prop()`
  - [x] Self-shadow prevention via entity_index exclusion
  - [x] Max 8 shadow casters (MAX_SHADOW_CASTERS in shader)

- [x] **Shadow Receiving**
  - [x] Background receives shadows from all props ✅
  - [x] Props receive shadows from other props ✅
  - [x] Player receives shadows from props ✅
  - [x] `render_sprite_lit_shadowed()` for static sprites
  - [x] `render_sprite_animated_lit_shadowed()` for animated sprites

### 5.3 Lighting Shader & Normal Maps ✅

- [x] **Normal Map Support**
  - [x] Normal map texture per sprite (SpriteComponent.normal_map)
  - [x] Background normal map for surface detail
  - [x] Fragment shader samples normal, applies to N·L calculation
  - [x] Normal decoding: `n * 2.0 - 1.0` with Z-flip for coordinate system

- [x] **Depth Map for Background**
  - [x] Depth map encodes Z-depth per pixel (white=near, black=far)
  - [x] Shader reconstructs 3D position from depth map
  - [x] Enables correct light distance calculation on 2.5D background
  - [x] Props use their transform z_depth directly

- [x] **GPU-Based Lighting (basic_lit.frag)**
  - [x] All light calculations in fragment shader
  - [x] Per-pixel diffuse lighting (N·L)
  - [x] Quadratic falloff with smooth attenuation
  - [x] Aspect ratio correction for circular light falloff
  - [x] Light uniforms: position[], color[], intensity[], radius[]

- [x] **Background vs Object Lighting**
  - [x] Objects: `NdotL <= 0` → no light (backface culling)
  - [x] Background: `abs(NdotL)` → no self-shadowing from normals
  - [x] Both receive cast shadows from props correctly

- [x] **Ambient Light**
  - [x] Global ambient color/intensity (shader uniforms)
  - [x] Configurable per scene
  - [x] Ensures minimum visibility

### 5.4 Rendering Pipeline ✅

- [x] **Single-Pass Lit Rendering**
  - [x] All lights processed in single fragment shader pass
  - [x] Shadow casters uploaded as uniform arrays
  - [x] Texture units: 0=diffuse, 1=normal, 2=depth, 3-6=shadow casters
  - [x] Efficient: no multi-pass compositing needed

- [x] **Offscreen Rendering (FBO)**
  - [x] Render at base resolution (320x180)
  - [x] Upscale to viewport (1280x720)
  - [x] Pixel-perfect filtering

### 5.5 Key Implementation Details

**Shader Files:**

- `basic_lit.frag` - main lighting + shadow shader
- `basic_lit.vert` - vertex transformation

**Critical Bug Fixes (documented):**

1. Ray direction must be UNNORMALIZED for t∈(0,1) intersection
2. Shadow UV needs Y-flip: `hitUV.y = 1.0 - hitUV.y`
3. Use BASE resolution (320x180) for shadow caster coordinate conversion

---

## Phase 6: Camera & Parallax

- [ ] **Camera System**
  - [ ] Camera follow player
  - [ ] Smooth camera movement
  - [ ] Camera bounds (scene limits)

- [ ] **Parallax Scrolling**
  - [ ] Parallax layer system
  - [ ] Multiple background layers at different speeds
  - [ ] Depth perception for 2D

---

## Phase 7: Debug Tools (PARTIAL)

- [x] **Debug Overlay**
  - [x] Show mouse world position
  - [x] Show player position
  - [x] Toggle with `D` key
  - [x] FPS counter
  - [x] Frame time display
  - [x] Entity counts (total, props, lights)

- [ ] **Scene Editor Mode**
  - [ ] Prop placement in debug mode
  - [ ] Grid snapping (16px)
  - [ ] Save props to file
  - [ ] Load/reload scene in editor

- [ ] **Quick Scene Loading**
  - [ ] Command-line `--scene` flag
  - [ ] Jump to specific scene
  - [ ] Testing without full playthrough

---

## Phase 8: Art & Content (PARTIAL)

- [x] **Test Assets Created**
  - [x] Player sprite (pixel art spritesheet)
  - [x] Background texture with normal map + depth map
  - [x] Props (box, tree, stone) with normal maps
  - [x] Test lights (warm main + blue fill)

- [ ] **Scene Data Files**
  - [ ] Start scene
  - [ ] Prop placements (JSON)
  - [ ] Light placements (JSON)

---

## Phase 9: Polish & Optimization

- [ ] **Input Feedback**
  - [ ] Cursor change on interactive objects
  - [ ] Visual feedback for movement

- [ ] **Performance**
  - [ ] Batch rendering if needed
  - [ ] Optimize lighting calculations
  - [ ] Profiling

- [ ] **Asset Packing** (Release Build)
  - [ ] Create `.pak` or `.bin` file format for asset bundling
  - [ ] Pack all PNG textures into single archive
  - [ ] Add optional compression (gzip/zstd)
  - [ ] Modify asset loader to read from `.pak` instead of loose files
  - [ ] Prevents asset tampering/modding in release builds

- [ ] **Audio** (if time)
  - [ ] Background music
  - [ ] Footstep sounds
  - [ ] Interaction sounds

---

## Phase 10: Story & Content (Later)

- [ ] **Dialogue System** (minimal)
- [ ] **Cutscenes** (simple)
- [ ] **Inventory System** (if needed)
- [ ] **Puzzle System** (if needed)

---

## Priority Order (Do This First)

1. ✅ **Sprite Loading System** - Foundation for everything
2. ✅ **Sprite Rendering** - See something on screen
3. ✅ **Input System** - Receive player interaction
4. ✅ **Player Movement** - See something move
5. ✅ **Scene Loading** - Have a place to put things
6. ✅ **Collision** - Objects interact with world
7. ✅ **Lighting & Shadows** - The 2.5D magic
8. ⏳ **Camera & Parallax** - Make it feel alive
9. ⏳ **Debug Tools** - Make dev faster
10. ⏳ **Content** - Fill the game world

---

## Notes

- Keep each step minimal
- Test frequently
- Don't add complexity until needed
- Keep all code in `src/` (C++ where possible, Objective-C++ only for Platform)
- Data before behavior (structs before functions)
- Geometry Editor enables rapid iteration without code recompilation

---

## Geometry Data Format (JSON)

```json
{
  "scene_config": {
    "horizon_y": 250,
    "scale_min": 0.01,
    "scale_max": 1.0
  },
  "walkable_areas": [
    {
      "name": "main_area",
      "points": [[100, 100], [500, 100], [500, 400], [100, 400]]
    },
    {
      "name": "hole_1",
      "points": [[200, 150], [300, 150], [300, 250], [200, 250]]
    }
  ],
  "hotspots": [
    {
      "name": "tree",
      "interaction_distance": 30,
      "points": [[250, 80], [280, 80], [290, 120], [260, 130], [230, 120]],
      "callback": "interact_tree"
    }
  ]
}
```

---

## Layer System (Established)

**Layers with Z-depth & Parallax:**

- BACKGROUND (0) - parallax 0.1 (far, slow)
- MIDGROUND (10) - parallax 1.0 (world background)
- ENTITIES (20) - parallax 1.0 (props, NPCs)
- PLAYER (30) - parallax 1.0 (player character) - FUTURE: use Y-sorting (z = player.y)
- OCCLUSION (40) - parallax 1.0 (trees, cover the player in depth)
- FOREGROUND (50) - parallax 1.2 (close obstacles)
- UI (100) - parallax 0.0 (fixed screen HUD)

**Point-and-Click Depth Logic:**
Player Y-position determines if they're in front of or behind occlusion objects (handled in Phase 4 with Y-Sorting)

---

## ECS Architecture (NEW)

**Entity-Component-System implemented in `src/ecs/`:**

- `World` - manages entities and component pools
- `EntityID` - unique entity identifier (uint32_t)
- **Components:**
  - `Transform2_5DComponent` - position, scale, z_depth
  - `SpriteComponent` - texture, size, animation, normal_map
  - `PointLightComponent` - 3D position, color, radius, intensity, casts_shadows
  - `ShadowCasterComponent` - enabled, alpha_threshold, shadow_intensity

**Entity Factories (`src/ecs/entity_factory.h`):**

- `create_static_prop()` - prop without shadow casting
- `create_shadow_casting_prop()` - prop that casts shadows
- `create_point_light()` / `create_point_light_at_pixel()` - lights
- `create_lamp()` - composite: sprite + attached light
- `create_emissive_object()` - glowing object

---

Last Updated: March 11, 2026

- Phase 5 Complete: 2.5D Lighting with ray-quad shadows, normal maps, depth maps
- ECS Architecture: Entity-Component-System for props, lights, shadow casters
- Shadow system: Props/Player can cast AND receive shadows
- Background lighting: Normal map support without self-shadowing
