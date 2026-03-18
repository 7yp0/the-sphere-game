# Hotspot System

Hotspots are interactive regions in the game world. They support click-based interactions, inventory item usage, and collision-based triggers.

## Overview

The hotspot system separates **geometry** (stored in JSON) from **behavior** (defined in code):

- **Geometry** → `assets/scenes/<scene>/geometry.json`
- **Callbacks** → `src/scene/scene_<name>.cpp`

This allows level designers to edit polygon shapes visually while programmers define the interaction logic in code.

## Interaction Types

| Type | Color in Editor | Behavior |
| ---- | --------------- | -------- |
| `IMMEDIATE` | Blue | Callback triggers instantly on click (no walking) |
| `WALK_TO_HOTSPOT` | Blue | Player walks to nearest point on hotspot boundary, then triggers |
| `WALK_TO_TARGET` | Blue | Player walks to exact `target_position`, then triggers |
| `TRIGGER` | Yellow | Fires once when the player walks into the area (no click required) |

The type can be changed in the Geometry Editor by selecting a hotspot and pressing **T** to cycle through all types.

## Creating a Hotspot

### Step 1: Draw the Polygon (Geometry Editor)

1. Press **D** to open the debug overlay
2. Press **H** to enter hotspot creation mode
3. Click to place vertices around the interactive area
4. Press **F** or click the first vertex to close the polygon

### Step 2: Set Interaction Type

Select the hotspot and press **T** to cycle to the desired type. Trigger hotspots turn yellow.

### Step 3: Set Target Position (WALK_TO_TARGET only)

1. Select the hotspot (click inside it or on a vertex)
2. **Shift+Click** where the player should stand
3. A magenta cross marks the target position
4. Right-click the magenta cross to delete it

### Step 4: Name the Hotspot

1. Open `assets/scenes/<scene>/geometry.json`
2. Find your hotspot (auto-named `hotspot_0`, `hotspot_1`, etc.)
3. Change the `name` to something meaningful:

```json
{
  "name": "door",
  "interaction_type": "walk_to_target",
  "tooltip_key": "Open door",
  "target_position": [150, 120],
  "points": [[100, 80], [150, 80], [150, 95], [100, 95]]
}
```

### Step 5: Register the Callback (Code)

In `src/scene/scene_<name>.cpp`, inside `init_scene_<name>()`:

```cpp
// Click hotspot
register_hotspot_callback("door", []() {
    DEBUG_LOG("Player opened the door!");
});

// Trigger hotspot (fires on enter)
register_hotspot_callback("zone_exit", []() {
    Scene::load_scene("kitchen", "from_hallway");
});
```

## Hotspot Properties

| Property | Type | Description |
|----------|------|-------------|
| `name` | string | Unique identifier for callback registration |
| `tooltip_key` | string | Text shown when hovering (empty = use name) |
| `bounds` | Polygon | Clickable/trigger area (set via editor) |
| `interaction_type` | enum | IMMEDIATE, WALK_TO_HOTSPOT, WALK_TO_TARGET, or TRIGGER |
| `target_position` | Vec2 | Walk destination for WALK_TO_TARGET (optional) |
| `enabled` | bool | Whether hotspot responds to input |
| `callback` | function | Code executed on interaction |
| `item_callbacks` | map | Item-specific callbacks (item_id → function) |

## API Functions

### register_hotspot_callback

Register a callback function for a hotspot by name.

```cpp
bool register_hotspot_callback(const std::string& name, std::function<void()> callback);
```

Works for all interaction types, including `TRIGGER`.

**Example:**

```cpp
register_hotspot_callback("chest", []() {
    DEBUG_LOG("Found treasure!");
    Inventory::add_item("gold");
});
```

### register_hotspot_item_callback

Register a callback for using a specific item on a hotspot (click-based types only).

```cpp
bool register_hotspot_item_callback(const std::string& hotspot_name,
                                    const std::string& item_id,
                                    std::function<void()> callback);
```

**Example:**

```cpp
register_hotspot_item_callback("locked_door", "key", []() {
    DEBUG_LOG("Door unlocked!");
    Inventory::remove_item("key");
    set_hotspot_enabled("locked_door", false);
    set_hotspot_enabled("open_door", true);
});
```

### set_hotspot_enabled

Enable or disable a hotspot at runtime.

```cpp
bool set_hotspot_enabled(const std::string& name, bool enabled);
```

**Example:**

```cpp
set_hotspot_enabled("locked_door", true);

register_hotspot_callback("locked_door", []() {
    DEBUG_LOG("The door is locked.");
});

register_hotspot_item_callback("locked_door", "key", []() {
    DEBUG_LOG("You unlock the door!");
    Inventory::remove_item("key");
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
    door->interaction_distance = 50.0f;
    door->enabled = player_has_key;
}
```

## Interaction Flow

### Click-Based Hotspots (IMMEDIATE / WALK_TO_HOTSPOT / WALK_TO_TARGET)

```
1. Click detected inside hotspot polygon
   ↓
2. Is hotspot enabled?
   → No: Ignore click
   → Yes: Continue
   ↓
3. Does player have an item selected?
   → Yes: Check item_callbacks for that item
   → No: Use default callback
   ↓
4. What is the interaction_type?
   → IMMEDIATE: Execute callback now
   → WALK_TO_HOTSPOT: Walk to nearest boundary point
   → WALK_TO_TARGET: Walk to target_position
   ↓
5. Player arrives at destination (if walking)
   ↓
6. Execute callback function
```

### Trigger Hotspots (TRIGGER)

```
Each frame:
   ↓
Is player position inside trigger polygon?
   → Entering (was outside, now inside): Execute callback once
   → Staying inside: No action
   → Exiting: Reset so it can fire again next entry
```

Trigger hotspots:

- Do **not** require a click
- Do **not** show a tooltip or change the cursor
- Fire **once per entry** (edge-triggered)
- Can be manually deactivated inside the callback via `set_hotspot_enabled("name", false)`

## Item-on-Hotspot Interaction

When hovering a hotspot with a selected item cursor, the item displays with a white outline to indicate a potential interaction.

```cpp
register_hotspot_callback("safe", []() {
    DEBUG_LOG("A locked safe. I need something to open it.");
});

register_hotspot_item_callback("safe", "crowbar", []() {
    DEBUG_LOG("*crack* The safe opens!");
    Inventory::remove_item("crowbar");
    Inventory::add_item("diamond");
});

register_hotspot_item_callback("safe", "key", []() {
    DEBUG_LOG("This key doesn't fit this lock.");
    // Item not consumed - player keeps it
});
```

## Common Patterns

### Pickup Item

```cpp
register_hotspot_callback("gold_coin", []() {
    Inventory::add_item("gold_coin");
    set_hotspot_enabled("gold_coin", false);
    set_entity_visible("gold_coin_sprite", false);
});
```

### Scene Exit (Trigger)

```cpp
// Hotspot type: TRIGGER
register_hotspot_callback("exit_to_kitchen", []() {
    Scene::load_scene("kitchen", "from_hallway");
});
```

### Locked Door (Requires Item)

```cpp
register_hotspot_callback("locked_door", []() {
    DEBUG_LOG("The door is locked. I need a key.");
});

register_hotspot_item_callback("locked_door", "key", []() {
    DEBUG_LOG("*click* The door unlocks!");
    Inventory::remove_item("key");
});
```

### One-Time Trigger Zone

```cpp
// Fires once, then disables itself
register_hotspot_callback("ambient_trigger", []() {
    Dialogue::say(g_state.player_entity, "dialogue.player.notice_something");
    set_hotspot_enabled("ambient_trigger", false);
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

- Click hotspot polygons shown in **red**
- Trigger hotspot polygons shown in **yellow**
- Target positions shown as magenta crosses
- Selected hotspot highlighted
- Press **T** on a selected hotspot to cycle its interaction type
- Press **R** to reload geometry from JSON (callbacks are preserved)

## Tooltip Display

When hovering a click-based hotspot, a tooltip is displayed:

- Uses `tooltip_key` if set, otherwise falls back to `name`
- When hovering with selected item cursor, item gets white outline
- **Trigger hotspots never show tooltips** — they are invisible to the player

## File Locations

| File | Purpose |
|------|---------|
| `assets/scenes/<scene>/geometry.json` | Hotspot geometry (polygons, names, type, target_position, tooltip_key) |
| `src/scene/scene_<name>.cpp` | Callback registration per scene |
| `src/scene/scene.h` | Hotspot struct and InteractionType definition |
| `src/game/player.cpp` | Click handling, approach logic, and trigger detection |
