# Geometry Editor - Debug Tool

The Geometry Editor is an in-game debug tool for creating and editing walkable areas, obstacles, hotspots, and spawn points directly in the game.

## Activation

Press **D** to toggle the debug overlay and geometry editor. When active, you'll see:

- Green polygons = Walkable Areas (where player CAN walk)
- Orange polygons = Obstacles (where player CANNOT walk)
- Blue polygons = Hotspots (interactive areas)
- Also Blue polygons = Trigger Hotspots (collision-based, fire once on enter)
- Magenta cross = Hotspot target position (where player walks to)
- Green cross + label = Spawn points
- Current mode displayed at bottom: `[SELECT]`, `[CREATING WALKABLE]`, `[CREATING OBSTACLE]`, `[CREATING HOTSPOT]`, or `[PLACE SPAWN POINT (click)]`

## Quick Reference

| Key/Mouse | Action |
|-----------|--------|
| **D** | Toggle debug overlay & editor |
| **W** | Start creating walkable area |
| **O** | Start creating obstacle |
| **H** | Start creating hotspot |
| **T** | Cycle interaction type of selected hotspot |
| **P** | Place spawn point mode (single click) |
| **F** | Finish/close polygon |
| **R** | Reload geometry from JSON (preserves callbacks) |
| **ESC** | Cancel creation / Deselect |
| **DEL** | Delete selected polygon |
| **Left-click vertex** | Select & drag vertex |
| **Left-click edge** | Insert new vertex |
| **Left-click inside hotspot** | Select hotspot |
| **Shift+Left-click** | Set hotspot target position |
| **Right-click vertex** | Delete vertex |
| **Right-click target** | Delete target position |
| **Right-click spawn point** | Delete spawn point |

## Creating Polygons

### Walkable Areas (Green)

Define areas where the player CAN walk.

1. Press **W** to enter walkable area creation mode
2. Click to place vertices (minimum 3 required)
3. Close the polygon by:
   - Clicking the first vertex (turns green when close enough), OR
   - Pressing **F** to finish
4. Polygon auto-saves to `assets/scenes/<scene>/geometry.json`

### Obstacles (Orange)

Define areas where the player CANNOT walk (tables, pillars, pits, etc.).

1. Press **O** to enter obstacle creation mode
2. Click to place vertices around the obstacle
3. Close the polygon the same way as walkable areas
4. Place obstacles INSIDE walkable areas to block movement

### Hotspots (Red / Yellow)

Define interactive or trigger areas.

1. Press **H** to enter hotspot creation mode
2. Click to place vertices (minimum 3 required)
3. Close the polygon the same way as walkable areas
4. **Optional:** Select the hotspot and **Shift+Click** to set a target position
5. **Optional:** Press **T** to cycle the interaction type

**Interaction types** (cycle with **T** on a selected hotspot):

| Type | Color | Behavior |
| ---- | ----- | -------- |
| `IMMEDIATE` | Red | Callback fires immediately on click |
| `WALK_TO_HOTSPOT` | Red | Player walks to nearest edge point, then callback fires |
| `WALK_TO_TARGET` | Red | Player walks to the set target position, then callback fires |
| `TRIGGER` | Yellow | Fires once when the player walks into the area (no click required) |

**Target Position:** When set, the player walks to this exact point instead of calculating an approach point dynamically. Useful for doors, items, or NPCs. Right-click the target marker to delete it.

See [HOTSPOTS.md](HOTSPOTS.md) for detailed hotspot configuration.

## Spawn Points

Spawn points define named positions where the player can appear when a scene is loaded via `Scene::load_scene("scene_name", "spawn_point_name")`.

1. Press **P** to enter spawn point placement mode
2. Click anywhere to place a spawn point
3. Enter a name in the prompt (e.g. `default`, `from_kitchen`)
4. The spawn point is saved immediately

**Deleting:** Right-click on the green cross marker of a spawn point to delete it.

**Usage in code:**

```cpp
Scene::load_scene("living_room", "from_hallway");
```

If the named spawn point doesn't exist or no name is given, the player spawns at scene center.

## Editing Polygons

### Moving Vertices

1. Click on a vertex to select it (turns cyan)
2. Drag to move it
3. Release to confirm - changes auto-save

### Adding Vertices

- Click on a polygon **edge** (between two vertices)
- A new vertex is inserted at the click position
- The new vertex is auto-selected for immediate dragging

### Deleting Vertices

- **Right-click** on a vertex to delete it
- Minimum 3 vertices are maintained (polygon needs at least 3 points)

### Deleting Entire Polygon

1. Select any vertex of the polygon
2. Press **DEL** to delete the entire polygon

## Holes / Obstacles

To create an obstacle inside a walkable area:

1. Create a large walkable area (the room boundary)
2. Create a **second walkable area** completely inside the first
3. The inner polygon acts as a "hole" - player cannot walk there

**How it works:** The engine uses an odd/even rule. If a point is inside an odd number of polygons, it's walkable. If inside an even number (including 0), it's not walkable.

**Example:** Table in middle of room

```bash
[Large room polygon] contains point → count = 1 (odd, walkable)
[Table polygon] also contains same point → count = 2 (even, NOT walkable)
```

## File Format

Geometry is saved per-scene in JSON format:

```bash
assets/scenes/<scene_name>/geometry.json
```

Example content:

```json
{
  "walkable_areas": [
    {
      "name": "walkable_0",
      "points": [[50, 100], [250, 100], [250, 150], [50, 150]]
    }
  ],
  "hotspots": [
    {
      "name": "door",
      "interaction_type": "walk_to_target",
      "interaction_distance": 20.0,
      "target_position": [150, 120],
      "points": [[100, 80], [150, 80], [150, 95], [100, 95]]
    },
    {
      "name": "zone_trigger",
      "interaction_type": "trigger",
      "interaction_distance": 20.0,
      "target_position": [0, 0],
      "points": [[60, 100], [120, 100], [120, 130], [60, 130]]
    }
  ],
  "spawn_points": [
    { "name": "default", "position": [160, 135] },
    { "name": "from_kitchen", "position": [30, 140] }
  ]
}
```

**Note:** The `name` field for hotspots is auto-generated (e.g., `hotspot_0`). Edit the JSON manually to give hotspots meaningful names for callback registration.

## Tips

- **Auto-save is always on** - every change saves immediately
- Use **ESC** liberally to cancel/deselect
- Selected polygon shows with brighter color and thicker lines
- Yellow preview line shows where next vertex will connect while creating
- Coordinates are in **base resolution** (320x180), not viewport resolution
- Trigger hotspots do **not** show a tooltip or change the cursor — they are invisible to the player
