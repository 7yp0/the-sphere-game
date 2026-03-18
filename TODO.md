# TODO

- [ ] double click on roomchange hotspots changes room immediately (if path is clear)

---

## Close-Up System

### Close-Up Scenes

- [ ] Special game mode for puzzle close-ups
- [ ] Separate input handling per close-up
- [ ] no player rendered
- [ ] Enter close-up via scene action `Scene::enterCloseUp(scene)`
- [ ] Return to main game after completion
- [ ] Pass/fail state back to main game (set flags)

### Cable Physics (Wrap-Point System)

- [ ] Kabel-Ende folgt Mausposition
- [ ] Wrap-Point-Tracking um Säule/Objekt
- [ ] Umrundung tracken (Maus kreist um Objekt → Kabel wickelt sich)
- [ ] Richtungswechsel = Wickeln/Abwickeln
- [ ] Z-Order pro Segment (vor/hinter Säule alternierend)
- [ ] Connect cable to socket

---

## Camera & World Navigation

### Camera System

- [ ] Camera follow player with deadzone
- [ ] Smooth camera movement (lerp)
- [ ] Camera bounds (scene limits, no overscroll)
- [ ] Scrollable scenes (larger than viewport)

### Parallax Scrolling

- [ ] Parallax layer system (horizontal + vertical)
- [ ] Multiple background layers at different speeds
- [ ] Depth factor per layer

---

## Cutscene System

### Cutscene Engine

- [ ] Action queue system
- [ ] Sequential execution (default)
- [ ] Parallel execution blocks (all actions in block run simultaneously, block ends when all complete)
- [ ] Fluent C++ API for scripting
- [ ] Transition effects für scene changes (fade, etc.) (`fade` + `changeScene` actions)

### Actions

- [ ] `say(entity, textkey)`
- [ ] `movePlayer(pos)` / `moveEntity(entity, pos)`
- [ ] `wait(seconds)`
- [ ] `panCamera(pos)`
- [ ] `zoomCamera(level, duration)` - Smooth zoom (1.0 = normal, 2.0 = 2x zoom)
- [ ] `enableEntity(entity)` / `disableEntity(entity)`
- [ ] `playAnimation(entity, anim)`
- [ ] `fade(type, duration)` - FadeIn/FadeOut
- [ ] `changeScene(scene, spawnPoint)`
- [ ] `setFlag(name, value)` / `setValue(name, value)`
- [ ] `playSound(sound)` / `playMusic(track)`
- [ ] `showText(text)` - Einblendung

### Input Blocking (Optional)

- [ ] `blockInput()` / `unblockInput()` actions
- [ ] Cutscenes do NOT block input by default
- [ ] Explicit control when needed (e.g. scene transitions)

```cpp
// Example: Door exit cutscene
Cutscene()
    .parallel([](auto& p) {
        p.movePlayer(door_pos);
        p.panCamera(door_camera);
    })
    .wait(0.5f)
    .playAnimation(door, "open")
    .movePlayer(exit_pos)
    .fade(FadeType::Out, 0.3f)
    .changeScene("kitchen", "from_hallway")
    .fade(FadeType::In, 0.3f);
```

---

## Shader FX

dizzy shader:

- [ ] 'sternchen sehen' (bunte punkte die nach innen gehen)

pass out shader:

- [ ] screen shake
- [ ] chromatic abrevation
- [ ] unscharf, verzerrung

ripple wave shader:

- [ ] ripple wave shader

---

## Audio

### Audio Format

- OGG für alles (beste Qualität/Größe ratio)

### Music System

- [ ] Event/State-triggered music (nicht automatisch per scene)
- [ ] `playMusic(track)` action in Cutscenes
- [ ] Track metadata: BPM, time signature
- [ ] Bar/beat tracking während playback
- [ ] Transitions:
  - [ ] Hard cut
  - [ ] Crossfade (timed)
  - [ ] Fade out / Fade in
  - [ ] Beat-synced transition (wechsel auf Bar X)
- [ ] `waitForBar(bar)` action für Cutscenes (wartet bis Bar erreicht)

### Sound Effects

- [ ] Interaction sounds
- [ ] Item pickup sounds
- [ ] UI sounds

### Atmosphere

- [ ] Ambient/Atmo loops per scene
- [ ] Layered ambient (mehrere gleichzeitig)

### Voice

- [ ] Voice files via Localization System (`voice/{lang}/string.key.ogg`)
- [ ] Auto-load matching voice for `say()` action
- [ ] Voice spielt während Text angezeigt wird
- [ ] Separate Voice volume control

---

## Debug & Development Tools

### Act Loading

- [ ] `--act 1/2/3` flag - Load specific act
- [ ] Predefined start states per act (scene, flags, inventory)
- [ ] Skip previous acts for testing

### Close-Up Loading  

- [ ] `--closeup <name>` flag - Load specific close-up puzzle
- [ ] Isolated testing of puzzle mechanics

---

## Performance (falls nötig)

### Optimization

- [ ] Batch rendering if needed
- [ ] Optimize lighting calculations
- [ ] Profiling

---

## Asset Packaging (Optional)

- [ ] Nicht zwingend nötig für Steam/GOG
- [ ] Falls gewünscht: `.pak` archive mit Compression
- [ ] Niedrige Priorität

---

## Content

### Background Graphics

- [ ] BACKYARD
- [ ] CORRIDORANDKITCHEN
- [ ] BASEMENT
- [ ] CRASHSITE

### Close-Up Graphics

- [ ] NIGHTSKY-CU
- [ ] CRASHSITESPHERE-CU
- [ ] VISION (with animation)
- [ ] DAY-CARDBOARDBOX-CU
- [ ] PHYSICSBOOK-CU
- [ ] MAGNETASSEMBLY-CU1
- [ ] MAGNETASSEMBLY-CU2
- [ ] SPHERE-CU
- [ ] POLISHING-CU

### Player Animations

- [ ] idle
- [ ] walking
- [ ] walking drunk
- [ ] looking up
- [ ] kneeling
- [ ] picking up/interacting on mid level
- [ ] sitting in chair
- [ ] drinking beer sitting
- [ ] standing up from chair
- [ ] looking right/left
- [ ] sitting on ground
- [ ] getting up from ground
- [ ] falling into couch
- [ ] sitting on couch
- [ ] from laying to sitting on couch
- [ ] from sitting on couch to standing
- [ ] passed out on floor
- [ ] getting up after passed out
- [ ] head talking
- [ ] picking up/interacting on low level
- [ ] picking up/interacting on high level
- [ ] startled
- [ ] scratching with screwdriver into brick wall
- [ ] grabbing his head
- [ ] rubbing his temple while swaying

### Agent Animations

- [ ] idle
- [ ] walking
- [ ] looking up

### Inventory Items

- [ ] almost empty bottle of beer
- [ ] closed bottle of beer
- [ ] bottle opener
- [ ] keys
- [ ] screwdriver
- [ ] long cable
- [ ] knife
- [ ] wire
- [ ] physics book
- [ ] radio
- [ ] battery of radio
- [ ] shovel
- [ ] metal rod
- [ ] coil
- [ ] electromagnet
- [ ] small key
- [ ] singing bowl
- [ ] toothpaste
- [ ] telescope
- [ ] metal scrap
- [ ] oily rag
- [ ] polishing rag
- [ ] reflecting metal plate

---

## Act 1

### Scenes

- [ ] Build scenes (lighting, hotspots, props, walkable areas)

### Cutscenes

- [ ] Script cutscenes

### Puzzles

- [ ] Implement puzzles

### Dialogue

- [ ] Write dialogue text

### Close-Ups

- [ ] Program close-up mechanics

---

## Act 2

### Scenes

- [ ] Build scenes (lighting, hotspots, props, walkable areas)

### Cutscenes

- [ ] Script cutscenes

### Puzzles

- [ ] Implement puzzles

### Dialogue

- [ ] Write dialogue text

### Close-Ups

- [ ] Program close-up mechanics

---

## Act 3

### Scenes

- [ ] Build scenes (lighting, hotspots, props, walkable areas)

### Cutscenes

- [ ] Script cutscenes

### Puzzles

- [ ] Implement puzzles

### Dialogue

- [ ] Write dialogue text

### Close-Ups

- [ ] Program close-up mechanics

---

## Sound Effects

- [ ] Asset list TBD

---

## Music

- [ ] Track list TBD

---

## Voice Over

- [ ] TBD

---

## Translations

- [ ] TBD
