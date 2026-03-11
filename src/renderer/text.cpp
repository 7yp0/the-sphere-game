#include "text.h"
#include "texture_loader.h"
#include <cstdio>
#include <cstring>

namespace Renderer {

static TextureID g_font_texture = 0;
static const char* FONT_CHARS = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
static constexpr float GLYPH_WIDTH = 12.0f;     // pixels
static constexpr float GLYPH_HEIGHT = 16.0f;    // pixels
static constexpr float CHARS_PER_ROW = 16.0f;
static constexpr float FONT_TEXTURE_WIDTH = 192.0f;
static constexpr float FONT_TEXTURE_HEIGHT = 96.0f;

void init_text_renderer() {
    if (g_font_texture == 0) {
        g_font_texture = load_texture("fonts/ui.png");
    }
}

void render_text(const char* text, Vec2 pos, float scale) {
    if (!g_font_texture) init_text_renderer();
    if (!text) return;
    
    Vec3 current_pos = Vec3(pos, -1.0f);  // Use z=-1.0f for UI text depth
    float start_x = pos.x;  // Remember start for newlines
    
    // Glyph size in pixels
    float glyph_width_px = GLYPH_WIDTH * scale;
    float glyph_height_px = GLYPH_HEIGHT * scale;
    
    for (const char* p = text; *p; p++) {
        // Handle newline
        if (*p == '\n') {
            current_pos.x = start_x;
            current_pos.y += glyph_height_px * 1.2f;  // Line spacing
            continue;
        }
        
        const char* char_idx = strchr(FONT_CHARS, *p);
        if (!char_idx) char_idx = strchr(FONT_CHARS, '?');
        
        if (char_idx) {
            int char_index = char_idx - FONT_CHARS;
            int row = char_index / (int)CHARS_PER_ROW;
            int col = char_index % (int)CHARS_PER_ROW;
            
            // Calculate UV coordinates for this glyph
            float min_u = (col * GLYPH_WIDTH) / FONT_TEXTURE_WIDTH;
            float max_u = ((col + 1) * GLYPH_WIDTH) / FONT_TEXTURE_WIDTH;
            float min_v = (row * GLYPH_HEIGHT) / FONT_TEXTURE_HEIGHT;
            float max_v = ((row + 1) * GLYPH_HEIGHT) / FONT_TEXTURE_HEIGHT;
            
            Vec4 uv_range(min_u, min_v, max_u, max_v);
            
            // render_sprite expects pixel coordinates and sizes, will convert internally
            render_sprite(g_font_texture, current_pos, Vec2(glyph_width_px, glyph_height_px), 
                         uv_range);
            current_pos.x += glyph_width_px * 1.1f;
        }
    }
}

}
