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

## Phase 3: Scene & Game World

- [ ] **Scene System**
  - [ ] `Game::Scene` struct
  - [ ] `Game::Prop` struct
  - [ ] Load props from simple data format (JSON/text)
  - [ ] Render background texture
  - [ ] Render all props in scene

- [ ] **Asset & Scene Loader**
  - [ ] Asset loader for scenes
  - [ ] Scene file format (JSON or text)

- [ ] **Game Loop Architecture**
  - [ ] Main update function
  - [ ] Collision detection stub
  - [ ] Render order (background → props → player → lights)

---

## Phase 4: Collision & Interaction

- [ ] **Collision Detection**
  - [ ] AABB collision for props
  - [ ] Pathfinding (simple grid-based A*)
  - [ ] Walk around obstacles
  - [ ] **Y-Sorting for Z-Depth** - Player z_depth = player.position.y to render behind/in front of props based on screen depth

- [ ] **Prop Interaction**
  - [ ] Interaction detection (proximity to prop)
  - [ ] Interactive vs non-interactive props
  - [ ] Callback system for prop events

- [ ] **Player State Machine** (minimal)
  - [ ] Walk state
  - [ ] Idle state
  - [ ] Interaction state (if needed)

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

Last Updated: March 4, 2026 - Phase 1 Complete (Sprites, Animation, Layers)

## FUTURE: Pixel-Art & Scaling Roadmap

- [ ] Pixel-art graphics system with depth scaling
- [ ] Integer-based scaling for pixel-perfect rendering
- [ ] Depth-based character scaling (4x→1x based on depth)
- [ ] Handle base resolution vs window resolution separation
