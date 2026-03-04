#pragma once

#include <stdint.h>

struct PNGImage {
    uint32_t width;
    uint32_t height;
    uint8_t* pixels;  // RGBA format
};

// Load PNG from file
// Assets location: build/assets/{filename}
// Example: png_load("test.png") loads from build/assets/test.png
// Returns image with pixels allocated (YOU MUST FREE with png_free)
PNGImage png_load(const char* filename);

// Free allocated PNG image
void png_free(PNGImage img);
