# Hotspot System

Hotspots are clickable interactive regions in the game world. They support direct interactions as well as using inventory items on them.

## Overview

The hotspot system separates **geometry** (stored in JSON) from **behavior** (defined in code):

- **Geometry** → `assets/scenes/<scene>/geometry.json`
- **Callbacks** → `src/scene/scene.cpp`

This allows level designers to edit polygon shapes visually while programmers define the interaction logic in code.

## Interaction Types

Hotspots support three interaction types:

| Type | Behavior |
|------|----------|
| `IMMEDIATE` | Callback triggers instantly on click (no walking) |
| `WALK_TO_HOTSPOT` | Player walks to nearest point on hotspot boundary, then triggers |
| `WALK_TO_TARGET` | Player walks to exact `target_position`, then triggers |

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

### Step 3: Name the Hotspot

1. Open `assets/scenes/<scene>/geometry.json`
2. Find your hotspot (auto-named `hotspot_0`, `hotspot_1`, etc.)
3. Change the `name` to something meaningful:

```json
{
  "name": "door",
  "tooltip_key": "Open door",
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
| `tooltip_key` | string | Text shown when hovering (empty = use name) |
| `bounds` | Polygon | Clickable area (set via editor) |
| `interaction_type` | enum | IMMEDIATE, WALK_TO_HOTSPOT, or WALK_TO_TARGET |
| `target_position` | Vec2 | Position for WALK_TO_TARGET type (optional) |
| `enabled` | bool | Whether hotspot responds to clicks |
| `callback` | function | Code executed on default interaction |
| `item_callbacks` | map | Item-specific callbacks (item_id → function) |

## API Functions

### register_hotspot_callback

Register a callback function for a hotspot by name (default interaction, no item).

```cpp
bool register_hotspot_callback(const std::string& name, std::function<void()> callback);
```

**Example:**

```cpp
register_hotspot_callback("chest", []() {
    DEBUG_LOG("Found treasure!");
    Inventory::add_item("gold");
});
```

### register_hotspot_item_callback

Register a callback for using a specific item on a hotspot.

```cpp
bool register_hotspot_item_callback(const std::string& hotspot_name, 
                                    const std::string& item_id, 
                                    std::function<void()> callback);
```

**Example:**

```cpp
// Using key on locked door
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
// Door is locked until player uses key
set_hotspot_enabled("locked_door", true);  // Can click but item needed

register_hotspot_callback("locked_door", []() {
    DEBUG_LOG("The door is locked.");  // Default interaction (no item)
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

## Item-on-Hotspot Interaction

When hovering a hotspot with a selected item cursor, the item displays with a white outline to indicate a potential interaction.

```cpp
// Setup: Register both default and item-specific callbacks
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

## Interaction Type Examples

### IMMEDIATE (UI Elements, Instant Actions)

```cpp
Hotspot* btn = get_hotspot("pause_button");
if (btn) {
    btn->interaction_type = InteractionType::IMMEDIATE;
}

register_hotspot_callback("pause_button", []() {
    toggle_pause_menu();
});
```

### WALK_TO_HOTSPOT (Large Areas)

The player walks to the nearest point on the hotspot boundary. Good for large interactive areas.

```json
{
  "name": "pool",
  "points": [[50, 100], [200, 100], [200, 150], [50, 150]]
}
```

```cpp
Hotspot* pool = get_hotspot("pool");
if (pool) {
    pool->interaction_type = InteractionType::WALK_TO_HOTSPOT;
}
```

### WALK_TO_TARGET (Precise Positioning)

The player walks to the exact `target_position`. Best for doors, NPCs, items.

```json
{
  "name": "door",
  "target_position": [150, 120],
  "points": [[100, 80], [150, 80], [150, 95], [100, 95]]
}
```

```cpp
Hotspot* door = get_hotspot("door");
if (door) {
    door->interaction_type = InteractionType::WALK_TO_TARGET;
}
```

## Common Patterns

### Pickup Item (Add to Inventory)

```cpp
register_hotspot_callback("gold_coin", []() {
    Inventory::add_item("gold_coin");
    set_hotspot_enabled("gold_coin", false);
    set_entity_visible("gold_coin_sprite", false);
});
```

### Locked Door (Requires Item)

```cpp
// Default: Tell player it's locked
register_hotspot_callback("locked_door", []() {
    DEBUG_LOG("The door is locked. I need a key.");
});

// With key: Unlock and open
register_hotspot_item_callback("locked_door", "key", []() {
    DEBUG_LOG("*click* The door unlocks!");
    Inventory::remove_item("key");
    // Trigger scene transition or animation
});
```

### Examine Object (Look At)

```cpp
register_hotspot_callback("painting", []() {
    DEBUG_LOG("A beautiful painting of a sunset.");
});
```

### Use Tool on Object

```cpp
register_hotspot_callback("rusty_grate", []() {
    DEBUG_LOG("A grate. It's rusted shut.");
});

register_hotspot_item_callback("rusty_grate", "crowbar", []() {
    DEBUG_LOG("*screech* The grate comes loose!");
    Inventory::remove_item("crowbar");
    set_hotspot_enabled("rusty_grate", false);
    set_hotspot_enabled("open_passage", true);
});

register_hotspot_item_callback("rusty_grate", "oil", []() {
    DEBUG_LOG("I oil the hinges...");
    Inventory::remove_item("oil");
    // Now crowbar works easier (game logic)
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

## Tooltip Display

When hovering a hotspot, a tooltip is displayed:
- Uses `tooltip_key` if set, otherwise falls back to `name`
- When hovering with selected item cursor, item gets white outline

## File Locations

| File | Purpose |
|------|---------|
| `assets/scenes/<scene>/geometry.json` | Hotspot geometry (polygons, names, target_position, tooltip_key) |
| `src/scene/scene.cpp` | Callback registration (default + item-specific) |
| `src/scene/scene.h` | Hotspot struct definition |
| `src/scene/scene.h` | Hotspot struct definition |
| `src/game/player.cpp` | Click handling and approach logic |
