#include "text.h"
#include "texture_loader.h"
#include "opengl_compat.h"
#include <cstdio>
#include <cstring>

namespace Renderer {

static TextureID g_font_texture = 0;
// ASCII printable + Latin Extended (German, Spanish, French)
// Source file must be UTF-8 encoded for this to work correctly
static const char FONT_CHARS[] = 
    " !\"#$%&'()*+,-./0123456789:;<=>?@"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
    "abcdefghijklmnopqrstuvwxyz{|}~"
    "\xC3\x84\xC3\x96\xC3\x9C"  // Ä Ö Ü
    "\xC3\xA4\xC3\xB6\xC3\xBC"  // ä ö ü
    "\xC3\x9F"                   // ß
    "\xC3\xA1\xC3\xA9\xC3\xAD\xC3\xB3\xC3\xBA"  // á é í ó ú
    "\xC3\xB1\xC3\x91"          // ñ Ñ
    "\xC2\xBF\xC2\xA1"          // ¿ ¡
    "\xC3\xA0\xC3\xA2\xC3\xA7"  // à â ç
    "\xC3\xA8\xC3\xAA\xC3\xAB"  // è ê ë
    "\xC3\xAE\xC3\xAF"          // î ï
    "\xC3\xB4\xC3\xB9\xC3\xBB\xC3\xBF"  // ô ù û ÿ
    "\xC5\x93\xC3\xA6"          // œ æ
    "\xC3\x87\xC3\x89\xC3\x88"  // Ç É È
    "\xC3\x8A\xC3\x8B"          // Ê Ë
    "\xC3\x8E\xC3\x8F"          // Î Ï
    "\xC3\x94\xC3\x99\xC3\x9B"  // Ô Ù Û
    "\xC5\xB8\xC5\x92\xC3\x86"  // Ÿ Œ Æ
    "\xC3\x80\xC3\x82"          // À Â
    "\xC2\xAB\xC2\xBB";         // « »
static constexpr float GLYPH_WIDTH = 24.0f;     // pixels
static constexpr float GLYPH_HEIGHT = 32.0f;    // pixels
static constexpr float CHARS_PER_ROW = 16.0f;
static constexpr float FONT_TEXTURE_WIDTH = 384.0f;
static constexpr float FONT_TEXTURE_HEIGHT = 288.0f;

void init_text_renderer() {
    if (g_font_texture == 0) {
        g_font_texture = load_texture("fonts/ui.png");
        // Override to GL_LINEAR so glyphs interpolate smoothly when drawn at non-1x scale
        glBindTexture(GL_TEXTURE_2D, g_font_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void render_text(const char* text, Vec2 pos, float scale, Vec4 color, float z, int max_chars) {
    if (!g_font_texture) init_text_renderer();
    if (!text) return;
    Vec3 current_pos = Vec3(pos, z);
    float start_x = pos.x;
    float glyph_width_px = GLYPH_WIDTH * scale;
    float glyph_height_px = GLYPH_HEIGHT * scale;
    const char* p = text;
    int chars_drawn = 0;
    while (*p) {
        if (max_chars >= 0 && chars_drawn >= max_chars) break;
        if (*p == '\n') {
            current_pos.x = start_x;
            current_pos.y += glyph_height_px * 1.2f;
            p++;
            continue;
        }
        int char_len = 1;
        unsigned char c = (unsigned char)*p;
        if ((c & 0x80) == 0) char_len = 1;
        else if ((c & 0xE0) == 0xC0) char_len = 2;
        else if ((c & 0xF0) == 0xE0) char_len = 3;
        else if ((c & 0xF8) == 0xF0) char_len = 4;
        int char_index = -1;
        const char* fc = FONT_CHARS;
        int index = 0;
        while (*fc) {
            int fc_len = 1;
            unsigned char fc_c = (unsigned char)*fc;
            if ((fc_c & 0x80) == 0) fc_len = 1;
            else if ((fc_c & 0xE0) == 0xC0) fc_len = 2;
            else if ((fc_c & 0xF0) == 0xE0) fc_len = 3;
            else if ((fc_c & 0xF8) == 0xF0) fc_len = 4;
            if (fc_len == char_len && memcmp(p, fc, char_len) == 0) {
                char_index = index;
                break;
            }
            fc += fc_len;
            index++;
        }
        
        // Fallback to '?' if character not found
        if (char_index < 0) {
            char_index = 31;  // '?' is at position 31 in ASCII
        }
        
        int row = char_index / (int)CHARS_PER_ROW;
        int col = char_index % (int)CHARS_PER_ROW;
        // Half-texel inset on all sides: GL_LINEAR averages with neighboring texels at
        // exact cell boundaries, causing narrow glyphs (like 'l') to bleed adjacent chars.
        const float hu = 0.5f / FONT_TEXTURE_WIDTH;
        const float hv = 0.5f / FONT_TEXTURE_HEIGHT;
        float min_u = (col * GLYPH_WIDTH)        / FONT_TEXTURE_WIDTH  + hu;
        float max_u = ((col + 1) * GLYPH_WIDTH)  / FONT_TEXTURE_WIDTH  - hu;
        float min_v = (row * GLYPH_HEIGHT)        / FONT_TEXTURE_HEIGHT + hv;
        float max_v = ((row + 1) * GLYPH_HEIGHT)  / FONT_TEXTURE_HEIGHT - hv;
        Vec4 uv_range(min_u, min_v, max_u, max_v);
        // Set color for this glyph
        Renderer::render_tinted_sprite(g_font_texture, current_pos, Vec2(glyph_width_px, glyph_height_px), uv_range, color);
        current_pos.x += glyph_width_px * Renderer::CHAR_SPACING;
        p += char_len;
        chars_drawn++;
    }
}

float calculate_text_width(const char* text, float scale) {
    if (!text) return 0.0f;
    float glyph_width = 24.0f * scale;
    float spacing = glyph_width * Renderer::CHAR_SPACING;
    // Count UTF-8 codepoints (not bytes) to handle multibyte chars (ä, ö, ü, etc.)
    int char_count = 0;
    for (const char* p = text; *p; ) {
        if (*p == '\n') { p++; continue; }
        unsigned char c = (unsigned char)*p;
        if      ((c & 0x80) == 0x00) p += 1;
        else if ((c & 0xE0) == 0xC0) p += 2;
        else if ((c & 0xF0) == 0xE0) p += 3;
        else                          p += 4;
        char_count++;
    }
    if (char_count == 0) return 0.0f;
    // Visual width = advance of all chars except last + full glyph width of last char
    // (avoids the trailing spacing gap that makes centering appear off)
    return (char_count - 1) * spacing + glyph_width;
}
}
