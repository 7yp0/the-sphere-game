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

- [ ] **Y-Sorting for Z-Depth**
  - [ ] Player z_depth = player.position.y (camera depth coordinate)
  - [ ] Props z_depth based on position.y
  - [ ] Hotspots don't affect rendering (invisible, only interactive)
  - [ ] Correct rendering order: behind/in-front based on screen depth

- [ ] **Player State Machine** (minimal)
  - [ ] Idle state
  - [ ] Walking state  
  - [ ] Interacting state (brief, often immediate)

---

## Phase 5: 2.5D Lighting

- [ ] **Dynamic Lights**
  - [ ] `Game::Light` struct
  - [ ] Multiple lights in scene
  - [ ] Light rendering pass (additive)
  - [ ] Light color and intensity

- [ ] **Shadow System**
  - [ ] Shadow generation from props
  - [ ] Simple shadow rendering (multiplicative blend)
  - [ ] Light-to-shadow calculations

- [ ] **Night/Day Lighting**
  - [ ] Ambient light control
  - [ ] Transition between light states

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

## Phase 7: Debug Tools

- [ ] **Debug Overlay**
  - [ ] Show mouse world position
  - [ ] Show player position
  - [ ] Toggle with `D` key
  - [ ] FPS counter

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

## Phase 8: Art & Content

- [ ] **Create Test Assets**
  - [ ] Player sprite (pixel art)
  - [ ] Background texture
  - [ ] Simple props
  - [ ] Placeholder lights

- [ ] **Scene Data Files**
  - [ ] Start scene
  - [ ] Prop placements
  - [ ] Light placements

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

Last Updated: March 6, 2026

- Phase 1 Complete (Sprites, Animation, Layers) + Logging System + Player Architecture
- Phase 4 Interaction System redesigned for Point-and-Click Adventure Game (Hotspot-based)
