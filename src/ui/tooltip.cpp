
#include "tooltip.h"
#include "../types.h"
#include "cursor.h" // For ZDepth
#include "../renderer/renderer.h"
#include "../renderer/text.h"
#include "ui.h"
#include <string>

namespace Tooltip {
    static std::string tooltip_text;

    void set(const std::string& text) {
        tooltip_text = text;
    }
    void clear() {
        tooltip_text.clear();
    }
    const std::string& get() {
        return tooltip_text;
    }

    void render(Vec2 mouse_pos, const char* text) {
        if (!text || text[0] == '\0') return;
        float scale = Renderer::get_ui_text_scale();
        float text_width  = Renderer::calculate_text_width(text, scale);
        float visual_h    = Renderer::GLYPH_VISUAL_HEIGHT * scale;  // actual filled pixel height
        float padding     = UI::CursorConfig::tooltip_padding();
        float bg_width    = text_width + padding / 2.0f;
        float bg_height   = visual_h   + padding / 2.0f;
        // Calculate tooltip position (smart positioning)
        float tooltip_x = mouse_pos.x + UI::CursorConfig::tooltip_offset_x();
        float tooltip_y = mouse_pos.y + UI::CursorConfig::tooltip_offset_y();
        float right_edge  = tooltip_x + bg_width;
        float bottom_edge = tooltip_y + bg_height;
        if (right_edge > (float)Renderer::get_ui_fbo_width()) {
            tooltip_x = mouse_pos.x - UI::CursorConfig::tooltip_offset_x() - bg_width;
        }
        if (bottom_edge > (float)Renderer::get_ui_fbo_height()) {
            tooltip_y = mouse_pos.y - UI::CursorConfig::tooltip_offset_y() - bg_height;
        }
        if (tooltip_x < 0) tooltip_x = 0;
        if (tooltip_y < 0) tooltip_y = 0;
        Vec3 bg_pos = Vec3(tooltip_x, tooltip_y, ZDepth::TOOLTIP);
        Vec4 bg_color = Vec4(0.0f, 0.0f, 0.0f, 0.8f);
        Renderer::render_rounded_rect(bg_pos, Vec2(bg_width, bg_height), bg_color, 8.0f);
        // Place glyph so its visual center (GLYPH_VISUAL_CENTER rows from cell top)
        // aligns exactly with the background center.
        Vec2 text_pos = Vec2(
            tooltip_x + (bg_width - text_width) / 2.0f,
            tooltip_y + bg_height * 0.5f - Renderer::GLYPH_VISUAL_CENTER * scale
        );
        Renderer::render_text(text, text_pos, scale);
    }
}
