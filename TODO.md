# TODO

## Close-Up System

### Puzzle Typ A: Wickel-Kabel (eine Säule, N Umwicklungen)

Läuft im `CLOSE_UP`-Modus. Der Spieler zieht ein Kupferkabel per Drag um eine einzelne Säule. Ziel: eine bestimmte Anzahl Wicklungen erreichen (z.B. 5). Kein Socket, kein Einstecken — die Wicklungszahl ist die Erfolgs-Bedingung.

#### Datenmodell

```cpp
struct Pillar {
    Vec2 position;  // Zentrum der Säule
    float radius;   // Kollisionsradius
};

struct WrapPoint {
    Vec2 position;  // Kontaktpunkt auf der Säule
    int side;       // +1 = links, -1 = rechts (bestimmt Z-Order)
};

struct WrapCableState {
    Vec2 anchor;                   // Fixer Anfangspunkt (an Wand/Gerät)
    Vec2 free_end;                 // Folgt der Maus beim Drag
    std::vector<WrapPoint> wraps;  // Aktive Wrap-Points (in Reihenfolge)
    int wind_count = 0;            // Netto-Wicklungen (+ = im Uhrzeigersinn)
    bool dragging = false;
};

struct WrapCableConfig {
    Vec2 anchor_pos;
    Pillar pillar;
    int required_winds;                     // z.B. 5 → Puzzle gelöst
    std::function<void()> on_complete;      // Feuert wenn required_winds erreicht
};
```

**Kabelstruktur:** `anchor → wrap[0] → wrap[1] → ... → free_end`
Jeder Abschnitt = ein Segment (orientiertes Quad, Kupferfarbe).

#### Wrap-Logik (Tangenten-Methode)

- Wrap **hinzufügen**: aktives Segment (letzter Punkt → Maus) kreuzt Tangentenlinie der Säule → `wind_count` ±1
- Wrap **entfernen**: Winkelumkehr am letzten Wrap-Point → `wind_count` ∓1
- `side` beim Hinzufügen: links oder rechts um die Säule → bestimmt Z-Order
- `on_complete` feuert wenn `abs(wind_count) >= required_winds`

#### Z-Order pro Segment

- Segment vor dem Wrap: vor der Säule
- Segment nach dem Wrap: hinter der Säule
- Alterniert bei jeder weiteren Wicklung

#### Rendering

- Jedes Segment = orientiertes Rechteck (Quad) entlang Segment-Richtung
- Breite: ~3px in Base-Resolution
- Farbe: Kupfer (`Vec4(0.72, 0.45, 0.20, 1.0)`)
- Textur nachrüstbar ohne Architekturänderung

#### Implementierungs-Checkliste

- [ ] `WrapCableState` + `WrapCableConfig` + `Pillar` + `WrapPoint` structs
- [ ] Drag-Input: Maus-Down auf free_end startet Drag, Maus-Up beendet
- [ ] Tangenten-Kreuzungs-Test gegen die Säule → Wrap hinzufügen, `wind_count` ±1
- [ ] Winkelumkehr-Test am letzten Wrap-Point → Wrap entfernen, `wind_count` ∓1
- [ ] Z-Order pro Segment aus `side`-Flag ableiten
- [ ] Segment-Rendering: orientierte Quads, Kupferfarbe, korrekte Z-Tiefe
- [ ] `on_complete` feuern wenn `abs(wind_count) >= required_winds`
- [ ] `WrapCablePuzzle`-Klasse: self-contained `update(mouse)` + `render()`
- [ ] Beispiel-Close-Up mit einer Säule, `required_winds = 5`

---

### Puzzle Typ B: Kabel verbinden (zwei freie Enden)

Läuft im `CLOSE_UP`-Modus. Zwei Kabelenden hängen lose herum; der Spieler zieht jedes per Drag zu einem fixen Anschlusspunkt. Kein Wickeln, keine Säule.

#### Datenmodell (Typ B)

```cpp
struct CableEnd {
    Vec2 position;       // Aktuelle Position des Endes
    Vec2 socket_pos;     // Ziel-Anschluss
    float socket_radius;
    bool connected = false;
    bool dragging = false;
};

struct ConnectCableConfig {
    Vec2 cable_mid;                       // Visueller Mittelpunkt des Kabels (hängt durch)
    CableEnd end_a;
    CableEnd end_b;
    std::function<void()> on_both_connected;  // Feuert wenn beide eingesteckt
};
```

**Kabelstruktur:** `end_a → cable_mid (durchhängend) → end_b`
Darstellung als geschwungene Kurve (Bezier oder Catmull-Rom durch 3 Punkte).

#### Rendering (Typ B)

- Gleiche Quad-Methode wie Typ A entlang der Kurve (mehrere kurze Segmente)
- Durchhängen simulieren: `cable_mid` hängt leicht nach unten (fixer Offset oder Schwerkraft-Approx.)
- Sobald ein Ende connected ist: das Ende rastet ein, Rest des Kabels hängt von dort

#### Implementierungs-Checkliste (Typ B)

- [ ] `CableEnd` + `ConnectCableConfig` structs
- [ ] Drag-Input: Maus-Down auf end_a oder end_b startet Drag des jeweiligen Endes
- [ ] Socket-Snap pro Ende: nah genug → einrasten
- [ ] Durchhänge-Berechnung für Kabel-Mitte
- [ ] Kurven-Rendering entlang Bezier (Quads)
- [ ] `on_both_connected` feuern wenn beide Enden eingesteckt
- [ ] `ConnectCablePuzzle`-Klasse: self-contained `update(mouse)` + `render()`
- [ ] Beispiel-Close-Up mit zwei Anschlüssen verdrahten

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
