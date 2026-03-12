# Geometry Editor - Debug Tool

The Geometry Editor is an in-game debug tool for creating and editing walkable areas, obstacles, and hotspots directly in the game.

## Activation

Press **D** to toggle the debug overlay and geometry editor. When active, you'll see:

- Green polygons = Walkable Areas (where player CAN walk)
- Orange polygons = Obstacles (where player CANNOT walk)
- Red polygons = Hotspots (interactive areas)
- Magenta cross = Hotspot target position (where player walks to)
- Current mode displayed at bottom: `[SELECT]`, `[CREATING WALKABLE]`, `[CREATING OBSTACLE]`, or `[CREATING HOTSPOT]`

## Quick Reference

| Key/Mouse | Action |
|-----------|--------|
| **D** | Toggle debug overlay & editor |
| **W** | Start creating walkable area |
| **O** | Start creating obstacle |
| **H** | Start creating hotspot |
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

### Hotspots (Red)

Define interactive areas the player can click on.

1. Press **H** to enter hotspot creation mode
2. Click to place vertices (minimum 3 required)
3. Close the polygon the same way as walkable areas
4. Hotspots are interactive areas the player can click on
5. **Optional:** Select the hotspot and **Shift+Click** to set a target position

**Target Position:** When set, the player walks to this exact point instead of calculating an approach point dynamically. This is useful for doors, items, or NPCs where the player should stand at a specific spot.

See [HOTSPOTS.md](HOTSPOTS.md) for detailed hotspot configuration.

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
    },
    {
      "name": "walkable_1",
      "points": [[120, 110], [180, 110], [180, 140], [120, 140]]
    }
  ],
  "hotspots": [
    {
      "name": "door",
      "interaction_distance": 20.0,
      "target_position": [150, 120],
      "points": [[100, 80], [150, 80], [150, 95], [100, 95]]
    }
  ]
}
```

**Note:** The `name` field is auto-generated (e.g., `hotspot_0`). Edit the JSON manually to give hotspots meaningful names for callback registration.

## Tips

- **Auto-save is always on** - every change saves immediately
- Use **ESC** liberally to cancel/deselect
- Selected polygon shows with brighter color and thicker lines
- Yellow preview line shows where next vertex will connect while creating
- Coordinates are in **base resolution** (320x180), not viewport resolution
