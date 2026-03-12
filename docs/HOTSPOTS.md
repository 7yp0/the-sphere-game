# Hotspot System

Hotspots are clickable interactive regions in the game world. When the player clicks on a hotspot, the character walks to it and triggers a callback function.

## Overview

The hotspot system separates **geometry** (stored in JSON) from **behavior** (defined in code):

- **Geometry** → `assets/scenes/<scene>/geometry.json`
- **Callbacks** → `src/scene/scene.cpp`

This allows level designers to edit polygon shapes visually while programmers define the interaction logic in code.

## Creating a Hotspot

### Step 1: Draw the Polygon (Geometry Editor)

1. Press **D** to open the debug overlay
2. Press **H** to enter hotspot creation mode
3. Click to place vertices around the interactive area
4. Press **F** or click the first vertex to close the polygon

### Step 2: Set Target Position (Optional)

1. Select the hotspot (click inside it or on a vertex)
2. **Shift+Click** where the player should stand
3. A magenta cross marks the target position

Without a target position, the player walks to the nearest point on the hotspot boundary.

### Step 3: Name the Hotspot

1. Open `assets/scenes/<scene>/geometry.json`
2. Find your hotspot (auto-named `hotspot_0`, `hotspot_1`, etc.)
3. Change the `name` to something meaningful:

```json
{
  "name": "door",
  "interaction_distance": 20.0,
  "target_position": [150, 120],
  "points": [[100, 80], [150, 80], [150, 95], [100, 95]]
}
```

### Step 4: Register the Callback (Code)

In `src/scene/scene.cpp`, inside `init_scene_test()`:

```cpp
// Register callback by name (after geometry is loaded)
register_hotspot_callback("door", []() {
    DEBUG_LOG("Player opened the door!");
    // Add your game logic here
});
```

## Hotspot Properties

| Property | Type | Description |
|----------|------|-------------|
| `name` | string | Unique identifier for callback registration |
| `bounds` | Polygon | Clickable area (set via editor) |
| `interaction_distance` | float | How close player must be to interact (pixels) |
| `target_position` | Vec2 | Exact position where player walks to (optional) |
| `enabled` | bool | Whether hotspot responds to clicks |
| `callback` | function | Code executed when player interacts |

## API Functions

### register_hotspot_callback

Register a callback function for a hotspot by name.

```cpp
bool register_hotspot_callback(const std::string& name, std::function<void()> callback);
```

**Example:**

```cpp
register_hotspot_callback("chest", []() {
    DEBUG_LOG("Found treasure!");
    player_inventory.add("gold", 100);
});
```

### set_hotspot_enabled

Enable or disable a hotspot at runtime.

```cpp
bool set_hotspot_enabled(const std::string& name, bool enabled);
```

**Example:**

```cpp
// Door is locked until player has key
set_hotspot_enabled("door", false);

register_hotspot_callback("key", []() {
    DEBUG_LOG("Got the key!");
    set_hotspot_enabled("door", true);  // Now door can be clicked
});
```

### get_hotspot

Get direct access to a hotspot for advanced manipulation.

```cpp
Hotspot* get_hotspot(const std::string& name);
```

**Example:**

```cpp
Hotspot* door = get_hotspot("door");
if (door) {
    door->interaction_distance = 50.0f;  // Increase range
    door->enabled = player_has_key;
}
```

## Interaction Flow

When the player clicks on a hotspot:

```
1. Click detected inside hotspot polygon
   ↓
2. Is hotspot enabled?
   → No: Ignore click
   → Yes: Continue
   ↓
3. Does hotspot have target_position?
   → Yes: Walk to target_position
   → No: Walk to nearest point on boundary + interaction_distance
   ↓
4. Player arrives at destination
   ↓
5. Execute callback function
```

## Target Position vs Dynamic Approach

### With target_position (Recommended)

The player walks to the exact coordinates specified. Use this for:

- Doors (stand in front)
- NPCs (stand at conversation distance)
- Items on tables (reach from specific angle)
- Switches/levers (stand where animation looks natural)

```json
{
  "name": "door",
  "target_position": [150, 120],
  "interaction_distance": 10.0,
  ...
}
```

### Without target_position (Dynamic)

The player walks to the nearest point on the hotspot boundary, offset by `interaction_distance`. Use this for:

- Large interactive areas
- Objects that can be approached from any direction

```json
{
  "name": "pool",
  "interaction_distance": 30.0,
  ...
}
```

## Common Patterns

### Pickup Item (Disappears)

```cpp
register_hotspot_callback("gold_coin", []() {
    player.gold += 10;
    set_hotspot_enabled("gold_coin", false);  // Can't pick up twice
    // TODO: Hide sprite
});
```

### Locked Door

```cpp
set_hotspot_enabled("locked_door", false);

register_hotspot_callback("key", []() {
    DEBUG_LOG("Got the key!");
    set_hotspot_enabled("locked_door", true);
    set_hotspot_enabled("key", false);
});

register_hotspot_callback("locked_door", []() {
    DEBUG_LOG("Door opened!");
    // Trigger door animation or scene transition
});
```

### Examine Object (Look At)

```cpp
register_hotspot_callback("painting", []() {
    show_dialogue("A beautiful painting of a sunset.");
});
```

### State-Dependent Interaction

```cpp
register_hotspot_callback("lever", []() {
    static bool is_pulled = false;
    is_pulled = !is_pulled;
    
    if (is_pulled) {
        DEBUG_LOG("Lever pulled - gate opens!");
        set_hotspot_enabled("gate_passage", true);
    } else {
        DEBUG_LOG("Lever released - gate closes!");
        set_hotspot_enabled("gate_passage", false);
    }
});
```

## Debugging

With debug overlay active (**D** key):

- Hotspot polygons shown in red
- Target positions shown as magenta crosses
- Selected hotspot highlighted
- Hotspot name printed to console on interaction
- Press **R** to reload geometry from JSON (callbacks are preserved)

## File Locations

| File | Purpose |
|------|---------|
| `assets/scenes/<scene>/geometry.json` | Hotspot geometry (polygons, names, target_position) |
| `src/scene/scene.cpp` | Callback registration |
| `src/scene/scene.h` | Hotspot struct definition |
| `src/game/player.cpp` | Click handling and approach logic |
