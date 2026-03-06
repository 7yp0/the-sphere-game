#pragma once

#include <cstdint>
#include <vector>
#include "animation.h"

namespace Renderer {

/**
 * Helper utilities for sprite sheet handling.
 * All sprite sheets are treated as grids (columns × rows).
 * Calculates UV coordinates for each frame automatically.
 */

// Calculate UV coordinates for frames in a grid (N columns × M rows)
// E.g., create_uv_grid(4, 1) = 4 frames horizontally in a 1-row grid
//       create_uv_grid(1, 4) = 4 frames vertically in a 4-row grid
//       create_uv_grid(4, 4) = 16 frames in a 4×4 grid
std::vector<SpriteFrame> create_uv_grid(uint32_t columns, uint32_t rows);

}
