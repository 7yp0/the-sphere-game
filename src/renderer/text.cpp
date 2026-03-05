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
    
    Vec2 current_pos = pos;
    float glyph_width_opengl = (GLYPH_WIDTH / FONT_TEXTURE_WIDTH) * 2.0f * scale;
    float glyph_height_opengl = (GLYPH_HEIGHT / FONT_TEXTURE_HEIGHT) * 2.0f * scale;
    
    for (const char* p = text; *p; p++) {
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
            
            render_sprite(g_font_texture, current_pos, Vec2(glyph_width_opengl, glyph_height_opengl), 
                         uv_range, -1.0f);
            current_pos.x += glyph_width_opengl * 1.1f;
        }
    }
}

}
