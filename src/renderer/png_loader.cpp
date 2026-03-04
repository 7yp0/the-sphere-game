#include "png_loader.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <zlib.h>

// PNG format helpers
#define PNG_SIGNATURE "\x89PNG\r\n\x1a\n"

struct PNGChunk {
    uint32_t length;
    char type[4];
    std::vector<uint8_t> data;
};

static uint32_t read_be32(const uint8_t* data) {
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | (uint32_t)data[3];
}

static PNGImage error_image() {
    return {0, 0, nullptr};
}

PNGImage png_load(const char* filename) {
    // Assets are expected next to the executable: build/assets/
    // For development: build/assets/filename
    // Fallback: try current dir and assets/ subdir for flexibility
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "assets/%s", filename);
    
    FILE* f = fopen(full_path, "rb");
    if (!f) {
        // Try without assets/ prefix (in case we're already in assets dir)
        f = fopen(filename, "rb");
    }
    
    if (!f) {
        return error_image();
    }

    // Read PNG signature
    uint8_t sig[8];
    if (fread(sig, 8, 1, f) != 1 || memcmp(sig, PNG_SIGNATURE, 8) != 0) {
        printf("Error: %s is not a valid PNG file\n", filename);
        fclose(f);
        return error_image();
    }

    uint32_t width = 0, height = 0;
    uint8_t bit_depth = 0, color_type = 0;
    std::vector<uint8_t> idat_data;

    // Read chunks
    while (true) {
        uint8_t len_bytes[4], type[4];
        if (fread(len_bytes, 4, 1, f) != 1) break;
        
        uint32_t length = read_be32(len_bytes);
        if (fread(type, 4, 1, f) != 1) break;

        if (strncmp((char*)type, "IHDR", 4) == 0) {
            // Read image header
            uint8_t ihdr[13];
            if (fread(ihdr, 13, 1, f) != 1) {
                fclose(f);
                return error_image();
            }
            width = read_be32(&ihdr[0]);
            height = read_be32(&ihdr[4]);
            bit_depth = ihdr[8];
            color_type = ihdr[9];
            fseek(f, 4, SEEK_CUR);  // Skip CRC
        } else if (strncmp((char*)type, "IDAT", 4) == 0) {
            // Read image data
            uint8_t* chunk = new uint8_t[length];
            if (fread(chunk, length, 1, f) != 1) {
                delete[] chunk;
                fclose(f);
                return error_image();
            }
            idat_data.insert(idat_data.end(), chunk, chunk + length);
            delete[] chunk;
            fseek(f, 4, SEEK_CUR);  // Skip CRC
        } else if (strncmp((char*)type, "IEND", 4) == 0) {
            break;
        } else {
            // Skip unknown chunks
            fseek(f, length + 4, SEEK_CUR);
        }
    }
    fclose(f);

    // Validate format (only support 8-bit RGBA for now)
    if (bit_depth != 8 || color_type != 6) {
        printf("Warning: PNG format not fully supported (need 8-bit RGBA)\n");
        printf("  bit_depth=%d, color_type=%d\n", bit_depth, color_type);
        return error_image();
    }

    if (width == 0 || height == 0 || idat_data.empty()) {
        printf("Error: Invalid PNG dimensions or no image data\n");
        return error_image();
    }

    // Decompress IDAT data
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;

    if (inflateInit(&stream) != Z_OK) {
        printf("Error: Could not initialize decompression\n");
        return error_image();
    }

    stream.avail_in = idat_data.size();
    stream.next_in = idat_data.data();

    // Allocate output: width * height * 4 bytes (RGBA) + 1 byte per scanline (filter type)
    std::vector<uint8_t> decompressed;
    decompressed.resize(height * (width * 4 + 1));

    stream.avail_out = decompressed.size();
    stream.next_out = decompressed.data();

    int ret = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);

    if (ret != Z_STREAM_END) {
        printf("Error: Decompression failed (ret=%d)\n", ret);
        return error_image();
    }

    // Filter decompressed data (undo PNG filters)
    // PNG uses prediction filters per scanline - we need to reverse them
    uint8_t* pixels = new uint8_t[width * height * 4];
    
    for (uint32_t y = 0; y < height; y++) {
        uint8_t filter_type = decompressed[y * (width * 4 + 1)];
        uint8_t* scanline = &decompressed[y * (width * 4 + 1) + 1];
        uint8_t* output_line = &pixels[y * width * 4];
        uint8_t* prev_line = (y > 0) ? &pixels[(y - 1) * width * 4] : nullptr;

        uint32_t bytes_per_pixel = 4;  // RGBA
        
        for (uint32_t x = 0; x < width * 4; x++) {
            uint8_t left = (x >= bytes_per_pixel) ? output_line[x - bytes_per_pixel] : 0;
            uint8_t above = prev_line ? prev_line[x] : 0;
            uint8_t above_left = (prev_line && x >= bytes_per_pixel) ? prev_line[x - bytes_per_pixel] : 0;
            
            uint8_t filtered = scanline[x];
            uint8_t unfiltered = 0;
            
            if (filter_type > 4) {
                printf("ERROR: Invalid PNG filter type %d at scanline %u\n", filter_type, y);
                delete[] pixels;
                return error_image();
            }
            
            switch (filter_type) {
                case 0:  // None
                    unfiltered = filtered;
                    break;
                case 1:  // Sub: byte + left
                    unfiltered = filtered + left;
                    break;
                case 2:  // Up: byte + above
                    unfiltered = filtered + above;
                    break;
                case 3:  // Average: byte + (left + above) / 2
                    unfiltered = filtered + ((left + above) / 2);
                    break;
                case 4: {  // Paeth: byte + paeth(left, above, above_left)
                    // Paeth predictor - complex but needed for best compression
                    int p = left + above - above_left;
                    int pa = abs(p - left);
                    int pb = abs(p - above);
                    int pc = abs(p - above_left);
                    
                    uint8_t pr = left;
                    if (pb < pa) pr = above;
                    if (pc < pb) pr = above_left;
                    
                    unfiltered = filtered + pr;
                    break;
                }
                default:
                    printf("ERROR: Unknown PNG filter type %d\n", filter_type);
                    unfiltered = filtered;
                    break;
            }
            
            output_line[x] = unfiltered;
        }
    }

    return {width, height, pixels};
}

void png_free(PNGImage img) {
    if (img.pixels) {
        delete[] img.pixels;
        img.pixels = nullptr;
    }
}
