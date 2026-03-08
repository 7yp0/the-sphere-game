#include "png_loader.h"
#include "asset_manager.h"
#include "../debug/debug_log.h"
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
    std::string full_path = Renderer::get_asset_path(filename);
    
    FILE* f = fopen(full_path.c_str(), "rb");
    if (!f) {
        DEBUG_ERROR("Could not find asset '%s' at '%s'", filename, full_path.c_str());
        return error_image();
    }

    // Read PNG signature
    uint8_t sig[8];
    if (fread(sig, 8, 1, f) != 1 || memcmp(sig, PNG_SIGNATURE, 8) != 0) {
        DEBUG_ERROR("%s is not a valid PNG file", filename);
        fclose(f);
        return error_image();
    }

    uint32_t width = 0, height = 0;
    uint8_t bit_depth = 0, color_type = 0;
    std::vector<uint8_t> idat_data;
    std::vector<uint8_t> palette;  // For indexed color
    std::vector<uint8_t> transparency;  // tRNS chunk for indexed color transparency

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
        } else if (strncmp((char*)type, "PLTE", 4) == 0) {
            // Read palette for indexed color
            palette.resize(length);
            if (fread(palette.data(), length, 1, f) != 1) {
                fclose(f);
                return error_image();
            }
            fseek(f, 4, SEEK_CUR);  // Skip CRC
        } else if (strncmp((char*)type, "tRNS", 4) == 0) {
            // Read transparency chunk for indexed color
            transparency.resize(length);
            if (fread(transparency.data(), length, 1, f) != 1) {
                fclose(f);
                return error_image();
            }
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

    // Validate format (support 8-bit RGBA, 8-bit indexed color, or 8-bit grayscale)
    if (bit_depth != 8 || (color_type != 6 && color_type != 3 && color_type != 0)) {
        DEBUG_LOG("PNG format not fully supported (need 8-bit RGBA, 8-bit indexed, or 8-bit grayscale)");
        DEBUG_LOG("  bit_depth=%d, color_type=%d", bit_depth, color_type);
        return error_image();
    }

    if (width == 0 || height == 0 || idat_data.empty()) {
        DEBUG_ERROR("Invalid PNG dimensions or no image data");
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
        DEBUG_ERROR("Could not initialize decompression");
        return error_image();
    }

    stream.avail_in = idat_data.size();
    stream.next_in = idat_data.data();

    // For indexed/grayscale: width * height * 1 byte; for RGBA: width * height * 4 bytes
    // Add 1 byte per scanline for filter type
    uint32_t bytes_per_pixel = (color_type == 3 || color_type == 0) ? 1 : 4;
    std::vector<uint8_t> decompressed;
    decompressed.resize(height * (width * bytes_per_pixel + 1));

    stream.avail_out = decompressed.size();
    stream.next_out = decompressed.data();

    int ret = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);

    if (ret != Z_STREAM_END) {
        DEBUG_ERROR("Decompression failed (ret=%d)", ret);
        return error_image();
    }

    // Filter decompressed data (undo PNG filters)
    // PNG uses prediction filters per scanline - we need to reverse them
    uint8_t* pixels = new uint8_t[width * height * 4];
    
    // For indexed/grayscale color, keep previous scanline's unfiltered data for filtering
    uint8_t* prev_unfiltered_data = nullptr;
    if (color_type == 3 || color_type == 0) {
        prev_unfiltered_data = new uint8_t[width];
        memset(prev_unfiltered_data, 0, width);
    }
    
    for (uint32_t y = 0; y < height; y++) {
        uint8_t filter_type = decompressed[y * (width * bytes_per_pixel + 1)];
        uint8_t* scanline = &decompressed[y * (width * bytes_per_pixel + 1) + 1];
        uint8_t* output_line = (color_type == 3 || color_type == 0) ? nullptr : &pixels[y * width * 4];
        
        // For indexed/grayscale, we need an intermediate buffer to store unfiltered data
        uint8_t* unfiltered_data = nullptr;
        if (color_type == 3 || color_type == 0) {
            unfiltered_data = new uint8_t[width];
        }
        
        for (uint32_t x = 0; x < width * bytes_per_pixel; x++) {
            uint8_t left = (x >= bytes_per_pixel) ? 
                ((color_type == 3 || color_type == 0) ? unfiltered_data[x - 1] : output_line[x - bytes_per_pixel]) : 0;
            
            // For indexed/grayscale: get from previous scanline's data; for RGBA: get from previous pixels
            uint8_t above = 0;
            if (color_type == 3 || color_type == 0) {
                // For indexed/grayscale, x is the pixel index (bytes_per_pixel=1), so use x directly
                if (prev_unfiltered_data) {
                    above = prev_unfiltered_data[x];
                }
            } else if (y > 0) {
                above = pixels[(y - 1) * width * 4 + x];
            }
            
            uint8_t above_left = 0;
            if (color_type == 3 || color_type == 0) {
                // For indexed/grayscale, get left neighbor from previous scanline
                if (prev_unfiltered_data && x >= 1) {
                    above_left = prev_unfiltered_data[x - 1];
                }
            } else if (y > 0 && x >= bytes_per_pixel) {
                above_left = pixels[(y - 1) * width * 4 + (x - bytes_per_pixel)];
            }
            
            uint8_t filtered = scanline[x];
            uint8_t unfiltered = 0;
            
            if (filter_type > 4) {
                DEBUG_ERROR("Invalid PNG filter type %d at scanline %u", filter_type, y);
                delete[] pixels;
                if (unfiltered_data) delete[] unfiltered_data;
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
                    DEBUG_ERROR("Unknown PNG filter type %d", filter_type);
                    unfiltered = filtered;
                    break;
            }
            
            if (color_type == 3 || color_type == 0) {
                unfiltered_data[x] = unfiltered;
            } else {
                output_line[x] = unfiltered;
            }
        }
        
        // Convert indexed/grayscale color to RGBA
        if (color_type == 3) {
            uint8_t* output_line = &pixels[y * width * 4];
            for (uint32_t x = 0; x < width; x++) {
                uint8_t index = unfiltered_data[x];
                
                // Ensure index is within palette bounds
                if (index * 3 + 2 < palette.size()) {
                    output_line[x * 4 + 0] = palette[index * 3 + 0];  // R
                    output_line[x * 4 + 1] = palette[index * 3 + 1];  // G
                    output_line[x * 4 + 2] = palette[index * 3 + 2];  // B
                    
                    // Get alpha from transparency chunk, or 255 if not present
                    if (index < transparency.size()) {
                        output_line[x * 4 + 3] = transparency[index];
                    } else {
                        output_line[x * 4 + 3] = 255;  // Opaque by default
                    }
                } else {
                    // Out of bounds - use black with full transparency as fallback
                    output_line[x * 4 + 0] = 0;
                    output_line[x * 4 + 1] = 0;
                    output_line[x * 4 + 2] = 0;
                    output_line[x * 4 + 3] = 255;
                }
            }
            
            // Save current unfiltered data as previous for next scanline
            memcpy(prev_unfiltered_data, unfiltered_data, width);
            delete[] unfiltered_data;
        } else if (color_type == 0) {
            // Grayscale to RGBA conversion
            uint8_t* output_line = &pixels[y * width * 4];
            for (uint32_t x = 0; x < width; x++) {
                uint8_t gray = unfiltered_data[x];
                output_line[x * 4 + 0] = gray;  // R
                output_line[x * 4 + 1] = gray;  // G
                output_line[x * 4 + 2] = gray;  // B
                output_line[x * 4 + 3] = 255;  // Full opacity
            }
            
            // Save current unfiltered data as previous for next scanline
            memcpy(prev_unfiltered_data, unfiltered_data, width);
            delete[] unfiltered_data;
        }
    }

    if ((color_type == 3 || color_type == 0) && prev_unfiltered_data) {
        delete[] prev_unfiltered_data;
    }

    return {width, height, pixels};
}

void png_free(PNGImage img) {
    if (img.pixels) {
        delete[] img.pixels;
        img.pixels = nullptr;
    }
}
