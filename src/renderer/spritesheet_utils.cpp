#include "spritesheet_utils.h"
#include <vector>

namespace Renderer {

std::vector<SpriteFrame> create_uv_grid(uint32_t columns, uint32_t rows) {
    std::vector<SpriteFrame> frames;
    if (columns == 0 || rows == 0) return frames;
    
    float frame_width = 1.0f / columns;
    float frame_height = 1.0f / rows;
    
    for (uint32_t row = 0; row < rows; ++row) {
        for (uint32_t col = 0; col < columns; ++col) {
            float u0 = col * frame_width;
            float u1 = (col + 1) * frame_width;
            float v0 = row * frame_height;
            float v1 = (row + 1) * frame_height;
            frames.push_back({ u0, v0, u1, v1 });
        }
    }
    return frames;
}

}
