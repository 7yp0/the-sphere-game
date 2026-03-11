# Geometry Editor - Debug Tool

The Geometry Editor is an in-game debug tool for creating and editing walkable areas and hotspots directly in the game.

## Activation

Press **D** to toggle the debug overlay and geometry editor. When active, you'll see:

- Green polygons = Walkable Areas
- Red polygons = Hotspots
- Current mode displayed at bottom: `[SELECT]`, `[CREATING WALKABLE]`, or `[CREATING HOTSPOT]`

## Quick Reference

| Key/Mouse | Action |
|-----------|--------|
| **D** | Toggle debug overlay & editor |
| **W** | Start creating walkable area |
| **H** | Start creating hotspot |
| **F** | Finish/close polygon |
| **ESC** | Cancel creation / Deselect |
| **DEL** | Delete selected polygon |
| **Left-click vertex** | Select & drag vertex |
| **Left-click edge** | Insert new vertex |
| **Right-click vertex** | Delete vertex |

## Creating Polygons

### Walkable Areas (Green)

1. Press **W** to enter walkable area creation mode
2. Click to place vertices (minimum 3 required)
3. Close the polygon by:
   - Clicking the first vertex (turns green when close enough), OR
   - Pressing **F** to finish
4. Polygon auto-saves to `assets/scenes/<scene>/geometry.json`

### Hotspots (Red)

1. Press **H** to enter hotspot creation mode
2. Click to place vertices (minimum 3 required)
3. Close the polygon the same way as walkable areas
4. Hotspots are interactive areas the player can click on

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
      "name": "walkable_1",
      "points": [[50, 100], [250, 100], [250, 150], [50, 150]]
    }
  ],
  "hotspots": [
    {
      "name": "hotspot_1",
      "points": [[100, 80], [150, 80], [150, 95], [100, 95]],
      "interaction_distance": 20.0,
      "enabled": true
    }
  ]
}
```

## Tips

- **Auto-save is always on** - every change saves immediately
- Use **ESC** liberally to cancel/deselect
- Selected polygon shows with brighter color and thicker lines
- Yellow preview line shows where next vertex will connect while creating
- Coordinates are in **base resolution** (320x180), not viewport resolution
