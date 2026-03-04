# The Sphere Game - TODO

## Phase 1: Sprite Loading & Rendering (FIRST!)

- [ ] **Texture & Asset System**
  - [ ] Add stb_image.h to project
  - [ ] `Renderer::load_texture(const char* path)` → returns TextureID
  - [ ] `Renderer::bind_texture(TextureID)`
  - [ ] Texture caching (don't load same texture twice)
  - [ ] Handle PNG loading

- [ ] **Sprite Renderer**
  - [ ] Replace `Platform::render_quad()` with proper sprite system
  - [ ] `Renderer::render_sprite(TextureID tex, Vec2 pos, Vec2 size, Color tint)`
  - [ ] move OpenGL shaders into platform independant folder
  - [ ] Update OpenGL shaders for textured rendering
  - [ ] Test with simple PNG sprite
  - [ ] add animation cycle system

- [ ] **Test Assets**
  - [ ] Create simple 64x64 red/blue test sprite (or simple PNG)
  - [ ] Verify loading and rendering works

---

## Phase 2: Core Input & Movement

- [ ] **Input System**
  - [ ] Platform abstraction for mouse input
  - [ ] `Platform::get_mouse_pos()` implementation (macOS)
  - [ ] `Platform::mouse_clicked()` detection

- [ ] **Player Entity**
  - [ ] Create `Game::Player` struct
  - [ ] Create `Game::Entity` base struct
  - [ ] Point-and-click movement to target position
  - [ ] Simple linear walking movement

- [ ] **Basic Game Loop**
  - [ ] Load test sprite for player
  - [ ] Render player at position
  - [ ] Test movement on click

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

Last Updated: March 4, 2026
