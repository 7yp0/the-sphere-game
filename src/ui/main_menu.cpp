#include "main_menu.h"
#include "ui.h"
#include <algorithm>
#include "game/game.h"
#include "save/save_system.h"
#include "core/settings.h"
#include "core/localization.h"
#include "renderer/renderer.h"
#include "renderer/text.h"
#include "platform.h"
#include "types.h"
// Note: no direct act_registry or SaveSystem calls here — use Game::start_new_game / continue_game

// Platform-specific ESC key code
#ifdef __APPLE__
    static constexpr int KEY_ESC = 53;
#else
    static constexpr int KEY_ESC = 0x1B;  // VK_ESCAPE on Windows/Linux
#endif

namespace UI {

// ============================================================================
// Scale helpers — all values are in UI-FBO pixel space
// ============================================================================

static float ts()   { return Renderer::get_ui_text_scale() * 1.5f; }   // item text scale
static float tts()  { return Renderer::get_ui_text_scale() * 2.0f; }   // title scale
static float qs()   { return Renderer::get_ui_text_scale() * 1.2f; }   // confirm question scale
static float lh(float scale) { return Renderer::GLYPH_VISUAL_HEIGHT * scale; }

// ============================================================================
// State
// ============================================================================

enum class MenuState { NORMAL, CONFIRM_NEW_GAME, SETTINGS };

enum class ItemID { CONTINUE, NEW_GAME, LOAD, SETTINGS, EXIT };

struct MenuItem {
    const char* label;
    ItemID      id;
    bool        enabled;
};

// Dynamic item list — rebuilt each frame based on g_can_resume
// Max 5 items: Continue + New Game + Load + Settings + Exit
static MenuItem g_items[5];
static int      g_item_count   = 0;
static float    g_max_item_w   = 0.0f;  // widest item text width, for panel sizing

static int       g_hovered      = -1;
static bool      g_can_resume   = false;
static bool      g_suppress_esc = false;
static MenuState g_state        = MenuState::NORMAL;

// label stores the localization key; use L10N(item.label) to get display text
#define L10N(key) Localization::get(key).c_str()

static void build_items() {
    g_item_count = 0;
    if (g_can_resume)
        g_items[g_item_count++] = { "menu.continue", ItemID::CONTINUE,  true };
    g_items[g_item_count++] = { "menu.new_game", ItemID::NEW_GAME,  true };
    g_items[g_item_count++] = { "menu.load",     ItemID::LOAD,      SaveSystem::has_save() };
    g_items[g_item_count++] = { "menu.settings", ItemID::SETTINGS,  true };
    g_items[g_item_count++] = { "menu.exit",     ItemID::EXIT,      true };

    g_max_item_w = 0.0f;
    for (int i = 0; i < g_item_count; ++i) {
        float w = Renderer::calculate_text_width(L10N(g_items[i].label), ts());
        if (w > g_max_item_w) g_max_item_w = w;
    }
}

// ============================================================================
// Layout
// ============================================================================

struct Layout {
    float cx, cy;
    float panel_x, panel_y, panel_w, panel_h;
    float title_y;
    float items_start_y;
    float item_step;
};

static Layout compute_layout(int num_items) {
    Layout L;
    L.cx = (float)Renderer::get_ui_fbo_width()  * 0.5f;
    L.cy = (float)Renderer::get_ui_fbo_height() * 0.5f;

    float ui = Renderer::get_ui_text_scale();
    float pad_v    = 24.0f * ui;
    float gap      = 20.0f * ui;   // title-to-items gap
    float title_lh = lh(tts());
    float item_lh  = lh(ts());
    L.item_step    = item_lh * 2.0f;

    float content_h  = title_lh + gap + num_items * L.item_step;
    L.panel_h = content_h + pad_v * 2.0f;
    // Width: fit the widest item text with generous side padding
    float title_w  = Renderer::calculate_text_width(L10N("menu.title"), tts());
    float widest   = std::max(g_max_item_w, title_w);
    float side_pad = 48.0f * ui;
    L.panel_w = std::max(widest + side_pad * 2.0f, 260.0f * ui);
    L.panel_x = L.cx - L.panel_w * 0.5f;
    L.panel_y = L.cy - L.panel_h * 0.5f;

    L.title_y       = L.panel_y + pad_v;
    L.items_start_y = L.title_y + title_lh + gap;
    return L;
}

static float item_top_y(const Layout& L, int i) {
    return L.items_start_y + i * L.item_step;
}

static bool hit(Vec2 p, float x, float y, float w, float h) {
    return p.x >= x && p.x < x + w && p.y >= y && p.y < y + h;
}

// Returns true if ESC was just pressed (edge-detect, respects suppress flag)
static bool esc_just_pressed() {
    static bool prev = false;
    bool cur = Platform::key_pressed(KEY_ESC);
    bool fired = false;
    if (g_suppress_esc) {
        g_suppress_esc = false;
    } else if (cur && !prev) {
        fired = true;
    }
    prev = cur;
    return fired;
}

// ============================================================================
// Shared panel + title renderer
// ============================================================================

static void render_panel_and_title(const Layout& L) {
    float fw = (float)Renderer::get_ui_fbo_width();
    float fh = (float)Renderer::get_ui_fbo_height();

    Renderer::render_rect(Vec3(0.0f, 0.0f, ZDepth::MAIN_MENU),
                          Vec2(fw, fh), Vec4(0.0f, 0.0f, 0.0f, 0.60f));

    Renderer::render_rounded_rect(
        Vec3(L.panel_x, L.panel_y, ZDepth::MAIN_MENU - 0.002f),
        Vec2(L.panel_w, L.panel_h),
        Vec4(0.0f, 0.0f, 0.0f, 0.82f),
        12.0f * Renderer::get_ui_text_scale());

    const char* title = L10N("menu.title");
    float tw = Renderer::calculate_text_width(title, tts());
    Renderer::render_text(title,
        Vec2(L.cx - tw * 0.5f,
             L.title_y + lh(tts()) * 0.5f - Renderer::GLYPH_VISUAL_CENTER * tts()),
        tts(), Vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

static void render_text_centered(const char* text, float center_x, float top_y,
                                  float scale, Vec4 color) {
    float tw = Renderer::calculate_text_width(text, scale);
    Renderer::render_text(text,
        Vec2(center_x - tw * 0.5f,
             top_y + lh(scale) * 0.5f - Renderer::GLYPH_VISUAL_CENTER * scale),
        scale, color);
}

// ============================================================================
// NORMAL state — update & render
// ============================================================================

static void update_normal(Vec2 mouse_pos) {
    build_items();

    Layout L = compute_layout(g_item_count);

    g_hovered = -1;
    for (int i = 0; i < g_item_count; ++i) {
        if (!g_items[i].enabled) continue;
        float iw = Renderer::calculate_text_width(L10N(g_items[i].label), ts());
        float ix = L.cx - iw * 0.5f;
        float iy = item_top_y(L, i);
        if (hit(mouse_pos, ix - 8.0f, iy, iw + 16.0f, lh(ts()) * 1.1f))
            g_hovered = i;
    }

    if (esc_just_pressed() && g_can_resume) {
        Game::g_state.mode = Game::GameMode::GAMEPLAY;
        return;
    }

    if (Platform::mouse_clicked() && g_hovered >= 0) {
        switch (g_items[g_hovered].id) {
            case ItemID::CONTINUE:
                Game::g_state.mode = Game::GameMode::GAMEPLAY;
                break;
            case ItemID::NEW_GAME:
                if (SaveSystem::has_save()) {
                    g_state = MenuState::CONFIRM_NEW_GAME;
                } else {
                    g_can_resume = true;
                    Game::start_new_game();
                }
                break;
            case ItemID::LOAD:
                g_can_resume = true;
                Game::continue_game();
                break;
            case ItemID::SETTINGS:
                g_state = MenuState::SETTINGS;
                break;
            case ItemID::EXIT:
                Platform::request_quit();
                break;
        }
    }
}

static void render_normal() {
    Layout L = compute_layout(g_item_count);
    render_panel_and_title(L);

    for (int i = 0; i < g_item_count; ++i) {
        bool hovered = (g_hovered == i);
        bool enabled = g_items[i].enabled;
        Vec4 color = !enabled ? Vec4(0.4f, 0.4f, 0.4f, 0.6f)
                   : hovered  ? Vec4(1.0f, 0.85f, 0.3f, 1.0f)
                              : Vec4(0.9f, 0.9f, 0.9f, 1.0f);
        render_text_centered(L10N(g_items[i].label), L.cx, item_top_y(L, i), ts(), color);
    }
}

// ============================================================================
// CONFIRM_NEW_GAME state — update & render
// ============================================================================
// Layout: title row, then question text, then [Yes]  [No] side by side

struct ConfirmLayout {
    Layout base;
    float question_y;
    // Yes / No hit rects
    float yes_x, no_x, btn_y, btn_w, btn_h;
    int hovered;  // 0=Yes, 1=No, -1=none
};

static ConfirmLayout compute_confirm_layout(Vec2 mouse_pos) {
    ConfirmLayout CL;
    Layout& L = CL.base;

    float ui       = Renderer::get_ui_text_scale();
    float pad      = 24.0f * ui;
    float title_gap = 20.0f * ui;   // same as main menu
    float line_gap  = 8.0f  * ui;
    float btn_gap   = 14.0f * ui;

    float title_h   = lh(tts());
    float q_h       = lh(qs());
    float w_h       = lh(qs() * 0.85f);
    float btn_h     = lh(ts()) * 1.4f;
    float btn_w     = Renderer::calculate_text_width(L10N("menu.confirm.yes"), ts()) + 24.0f * ui;
    float btn_sep   = 20.0f * ui;

    // Content stack from items_start_y:
    //   question | line_gap | warning | btn_gap | buttons
    float q_to_w   = q_h + line_gap;
    float w_to_btn = w_h + btn_gap;
    float total_content = q_to_w + w_to_btn + btn_h;

    // Exact panel height
    L.panel_h = pad + title_h + title_gap + total_content + pad;

    // Width: fit widest text line + padding
    float q_w = Renderer::calculate_text_width(L10N("menu.confirm.question"), qs());
    float w_w = Renderer::calculate_text_width(L10N("menu.confirm.warning"),  qs() * 0.85f);
    float two_btns = btn_w * 2.0f + btn_sep + 16.0f * ui;
    L.panel_w = std::max({q_w, w_w, two_btns}) + 48.0f * ui;

    L.cx = (float)Renderer::get_ui_fbo_width()  * 0.5f;
    L.cy = (float)Renderer::get_ui_fbo_height() * 0.5f;
    L.panel_x = L.cx - L.panel_w * 0.5f;
    L.panel_y = L.cy - L.panel_h * 0.5f;
    L.title_y       = L.panel_y + pad;
    L.items_start_y = L.title_y + title_h + title_gap;

    CL.question_y = L.items_start_y;
    CL.btn_y      = L.items_start_y + q_to_w + w_to_btn;
    CL.btn_w      = btn_w;
    CL.btn_h      = btn_h;
    CL.yes_x      = L.cx - btn_w - btn_sep * 0.5f;
    CL.no_x       = L.cx + btn_sep * 0.5f;

    CL.hovered = -1;
    if (hit(mouse_pos, CL.yes_x, CL.btn_y, CL.btn_w, CL.btn_h)) CL.hovered = 0;
    if (hit(mouse_pos, CL.no_x,  CL.btn_y, CL.btn_w, CL.btn_h)) CL.hovered = 1;
    return CL;
}

static void update_confirm(Vec2 mouse_pos) {
    ConfirmLayout CL = compute_confirm_layout(mouse_pos);

    if (esc_just_pressed()) {
        g_state = MenuState::NORMAL;
        return;
    }

    if (Platform::mouse_clicked()) {
        if (CL.hovered == 0) {       // Yes — overwrite confirmed
            SaveSystem::clear_save();
            g_can_resume = true;
            g_state = MenuState::NORMAL;
            Game::start_new_game();  // resets all state + loads Act 1
        } else if (CL.hovered == 1) { // No — cancel
            g_state = MenuState::NORMAL;
        }
    }
}

static void render_confirm(Vec2 mouse_pos) {
    ConfirmLayout CL = compute_confirm_layout(mouse_pos);
    Layout& L = CL.base;
    render_panel_and_title(L);

    float ui = Renderer::get_ui_text_scale();

    // Question text (two lines, wrapped manually)
    render_text_centered(L10N("menu.confirm.question"),
                         L.cx, CL.question_y, qs(), Vec4(1.0f, 0.9f, 0.6f, 1.0f));
    render_text_centered(L10N("menu.confirm.warning"),
                         L.cx, CL.question_y + lh(qs()) * 1.4f,
                         qs() * 0.85f, Vec4(0.7f, 0.7f, 0.7f, 1.0f));

    // Yes button
    {
        Vec4 bg = CL.hovered == 0 ? Vec4(0.7f, 0.15f, 0.1f, 0.9f)
                                  : Vec4(0.4f, 0.1f, 0.1f, 0.8f);
        Renderer::render_rounded_rect(
            Vec3(CL.yes_x, CL.btn_y, ZDepth::MAIN_MENU - 0.003f),
            Vec2(CL.btn_w, CL.btn_h), bg, 6.0f * ui);
        render_text_centered(L10N("menu.confirm.yes"), CL.yes_x + CL.btn_w * 0.5f,
                             CL.btn_y + (CL.btn_h - lh(ts())) * 0.5f,
                             ts(), Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // No button
    {
        Vec4 bg = CL.hovered == 1 ? Vec4(0.25f, 0.45f, 0.25f, 0.9f)
                                  : Vec4(0.15f, 0.3f, 0.15f, 0.8f);
        Renderer::render_rounded_rect(
            Vec3(CL.no_x, CL.btn_y, ZDepth::MAIN_MENU - 0.003f),
            Vec2(CL.btn_w, CL.btn_h), bg, 6.0f * ui);
        render_text_centered(L10N("menu.confirm.no"), CL.no_x + CL.btn_w * 0.5f,
                             CL.btn_y + (CL.btn_h - lh(ts())) * 0.5f,
                             ts(), Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
}

// ============================================================================
// SETTINGS state — update & render
// ============================================================================

static const char* k_languages[]   = { "en", "de" };
static const char* k_lang_labels[] = { "English", "Deutsch" };
static constexpr int k_lang_count  = 2;

// All positions computed once; shared by update and render to stay in sync.
struct SettingsLayout {
    float cx;
    float panel_x, panel_y, panel_w, panel_h;
    float title_y;
    float label_x;
    float track_x, track_w, track_h;
    float row_step;
    float content_y;

    // Slider rows (index 0-2)
    float slider_track_y[3];   // top-y of each track rect (for hit + draw)

    // Language row
    float lang_center_y;
    float arrow_size;          // square hit area for < >
    float arrow_left_x, arrow_right_x, arrow_y;  // top-y of arrow hit rect

    // Back button
    float back_x, back_y, back_w, back_h;
};

static SettingsLayout compute_settings_layout() {
    SettingsLayout L;
    float ui  = Renderer::get_ui_text_scale();

    L.cx = (float)Renderer::get_ui_fbo_width()  * 0.5f;
    float cy = (float)Renderer::get_ui_fbo_height() * 0.5f;

    // Each row needs room for text label AND slider track side-by-side,
    // with clear visual separation between rows.
    L.row_step = lh(ts()) * 1.8f;
    L.track_h  = 8.0f * ui;

    // Label column: wide enough for the longest label in the current language
    const char* label_keys[4] = {
        "menu.settings.music", "menu.settings.sfx",
        "menu.settings.voice", "menu.settings.language"
    };
    float max_lw = 0.0f;
    for (int i = 0; i < 4; ++i) {
        float w = Renderer::calculate_text_width(L10N(label_keys[i]), ts());
        if (w > max_lw) max_lw = w;
    }
    float label_col_w = max_lw + 16.0f * ui;

    // Percentage label on right of slider
    float pct_w = Renderer::calculate_text_width("100%", ts() * 0.75f) + 8.0f * ui;

    // Arrow buttons for language selector
    L.arrow_size = lh(ts()) * 1.2f;

    // Minimum track width: needs to fit the widest language name + 2 arrows with margin
    float max_lang_w = 0.0f;
    for (int i = 0; i < k_lang_count; ++i) {
        float w = Renderer::calculate_text_width(k_lang_labels[i], ts());
        if (w > max_lang_w) max_lang_w = w;
    }
    float min_track_w = L.arrow_size * 2.0f + max_lang_w + 16.0f * ui;

    float inner    = 20.0f * ui;
    float pad      = 24.0f * ui;
    float gap      = 12.0f * ui;
    float title_lh = lh(tts());

    // Panel width: fit label + track + pct, with generous margin
    float needed_track_w = std::max(min_track_w, 160.0f * ui);
    L.panel_w = inner * 2.0f + label_col_w + needed_track_w + pct_w;
    // Enforce a minimum panel width
    float min_panel = 360.0f * ui;
    if (L.panel_w < min_panel) L.panel_w = min_panel;

    // 5 rows: Music, SFX, Voice, Language, Back
    float content_h = title_lh + gap + 5.0f * L.row_step;
    L.panel_h  = content_h + pad * 2.0f;
    L.panel_x  = L.cx - L.panel_w * 0.5f;
    L.panel_y  = cy - L.panel_h * 0.5f;
    L.title_y  = L.panel_y + pad;
    L.content_y = L.title_y + title_lh + gap;

    L.label_x = L.panel_x + inner;
    L.track_x = L.label_x + label_col_w;
    L.track_w = L.panel_w - inner * 2.0f - label_col_w - pct_w;

    // Slider tracks (rows 0-2)
    for (int i = 0; i < 3; ++i) {
        float row_center = L.content_y + i * L.row_step + L.row_step * 0.5f;
        L.slider_track_y[i] = row_center - L.track_h * 0.5f;
    }

    // Language row (row 3)
    L.lang_center_y = L.content_y + 3 * L.row_step + L.row_step * 0.5f;
    L.arrow_y       = L.lang_center_y - L.arrow_size * 0.5f;
    L.arrow_left_x  = L.track_x;
    L.arrow_right_x = L.track_x + L.track_w - L.arrow_size;

    // Back button (row 4)
    float back_center = L.content_y + 4 * L.row_step + L.row_step * 0.5f;
    L.back_w = Renderer::calculate_text_width(L10N("menu.back"), ts()) + 40.0f * ui;
    L.back_h = lh(ts()) * 1.4f;
    L.back_x = L.cx - L.back_w * 0.5f;
    L.back_y = back_center - L.back_h * 0.5f;

    return L;
}

// Which slider is currently being dragged: 0=music,1=sfx,2=voice,-1=none
static int g_drag_slider = -1;

static void update_settings(Vec2 mouse_pos) {
    if (esc_just_pressed()) {
        Settings::save();
        g_state = MenuState::NORMAL;
        return;
    }

    // Read once — mouse_clicked() is consume-on-read
    bool clicked = Platform::mouse_clicked();
    bool held    = Platform::mouse_down();

    SettingsLayout L = compute_settings_layout();

    if (!held) g_drag_slider = -1;

    // ---- Slider rows (Music=0, SFX=1, Voice=2) ----
    for (int i = 0; i < 3; ++i) {
        bool in_track = mouse_pos.x >= L.track_x
                     && mouse_pos.x <= L.track_x + L.track_w
                     && mouse_pos.y >= L.slider_track_y[i]
                     && mouse_pos.y <= L.slider_track_y[i] + L.track_h;

        if (clicked && in_track) g_drag_slider = i;

        if (g_drag_slider == i && held) {
            float t = (mouse_pos.x - L.track_x) / L.track_w;
            t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
            if      (i == 0) Settings::set_music_volume(t);
            else if (i == 1) Settings::set_sfx_volume(t);
            else             Settings::set_voice_volume(t);
        }
    }

    // ---- Language row ----
    if (clicked) {
        int cur_lang = 0;
        for (int i = 0; i < k_lang_count; ++i)
            if (Settings::get_language() == k_languages[i]) { cur_lang = i; break; }

        if (hit(mouse_pos, L.arrow_left_x, L.arrow_y, L.arrow_size, L.arrow_size)) {
            cur_lang = (cur_lang + k_lang_count - 1) % k_lang_count;
            Settings::set_language(k_languages[cur_lang]);
            Localization::set_language(k_languages[cur_lang]);
        }
        if (hit(mouse_pos, L.arrow_right_x, L.arrow_y, L.arrow_size, L.arrow_size)) {
            cur_lang = (cur_lang + 1) % k_lang_count;
            Settings::set_language(k_languages[cur_lang]);
            Localization::set_language(k_languages[cur_lang]);
        }

        // ---- Back button ----
        if (hit(mouse_pos, L.back_x, L.back_y, L.back_w, L.back_h)) {
            Settings::save();
            g_state = MenuState::NORMAL;
        }
    }
}

static void render_settings(Vec2 mouse_pos) {
    SettingsLayout L = compute_settings_layout();
    float ui = Renderer::get_ui_text_scale();

    // Full-screen dim + panel
    float fw = (float)Renderer::get_ui_fbo_width();
    float fh = (float)Renderer::get_ui_fbo_height();
    Renderer::render_rect(Vec3(0.0f, 0.0f, ZDepth::MAIN_MENU),
                          Vec2(fw, fh), Vec4(0.0f, 0.0f, 0.0f, 0.60f));
    Renderer::render_rounded_rect(
        Vec3(L.panel_x, L.panel_y, ZDepth::MAIN_MENU - 0.002f),
        Vec2(L.panel_w, L.panel_h),
        Vec4(0.0f, 0.0f, 0.0f, 0.82f),
        12.0f * ui);

    // Title
    const char* title = L10N("menu.settings.title");
    float tw = Renderer::calculate_text_width(title, tts());
    Renderer::render_text(title,
        Vec2(L.cx - tw * 0.5f, L.title_y + lh(tts()) * 0.5f - Renderer::GLYPH_VISUAL_CENTER * tts()),
        tts(), Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // ---- Slider rows ----
    const char* slider_keys[3] = {
        "menu.settings.music", "menu.settings.sfx", "menu.settings.voice"
    };
    float vol_values[3] = {
        Settings::get_music_volume(),
        Settings::get_sfx_volume(),
        Settings::get_voice_volume()
    };

    float thumb_w = 6.0f * ui;

    for (int i = 0; i < 3; ++i) {
        float row_center = L.content_y + i * L.row_step + L.row_step * 0.5f;
        float track_y    = L.slider_track_y[i];

        // Label
        Renderer::render_text(L10N(slider_keys[i]),
            Vec2(L.label_x, row_center - Renderer::GLYPH_VISUAL_CENTER * ts()),
            ts(), Vec4(0.85f, 0.85f, 0.85f, 1.0f));

        // Track background
        Renderer::render_rounded_rect(
            Vec3(L.track_x, track_y, ZDepth::MAIN_MENU - 0.004f),
            Vec2(L.track_w, L.track_h),
            Vec4(0.2f, 0.2f, 0.2f, 1.0f),
            L.track_h * 0.5f);

        // Filled portion
        float filled_w = L.track_w * vol_values[i];
        if (filled_w > thumb_w)
            Renderer::render_rounded_rect(
                Vec3(L.track_x, track_y, ZDepth::MAIN_MENU - 0.005f),
                Vec2(filled_w, L.track_h),
                Vec4(0.3f, 0.6f, 1.0f, 1.0f),
                L.track_h * 0.5f);

        // Thumb
        float thumb_x = L.track_x + L.track_w * vol_values[i] - thumb_w * 0.5f;
        Renderer::render_rounded_rect(
            Vec3(thumb_x, track_y - 2.0f * ui, ZDepth::MAIN_MENU - 0.006f),
            Vec2(thumb_w, L.track_h + 4.0f * ui),
            Vec4(1.0f, 1.0f, 1.0f, 1.0f),
            thumb_w * 0.5f);

        // Percentage
        char pct[8];
        snprintf(pct, sizeof(pct), "%d%%", (int)(vol_values[i] * 100.0f + 0.5f));
        Renderer::render_text(pct,
            Vec2(L.track_x + L.track_w + 6.0f * ui,
                 row_center - Renderer::GLYPH_VISUAL_CENTER * ts() * 0.8f),
            ts() * 0.8f, Vec4(0.6f, 0.6f, 0.6f, 1.0f));
    }

    // ---- Language row ----
    Renderer::render_text(L10N("menu.settings.language"),
        Vec2(L.label_x, L.lang_center_y - Renderer::GLYPH_VISUAL_CENTER * ts()),
        ts(), Vec4(0.85f, 0.85f, 0.85f, 1.0f));

    const char* lang_name = k_lang_labels[0];
    for (int i = 0; i < k_lang_count; ++i)
        if (Settings::get_language() == k_languages[i]) { lang_name = k_lang_labels[i]; break; }

    bool hover_left  = hit(mouse_pos, L.arrow_left_x,  L.arrow_y, L.arrow_size, L.arrow_size);
    bool hover_right = hit(mouse_pos, L.arrow_right_x, L.arrow_y, L.arrow_size, L.arrow_size);

    render_text_centered("<", L.arrow_left_x  + L.arrow_size * 0.5f,
        L.lang_center_y - lh(ts()) * 0.5f, ts(),
        hover_left ? Vec4(1.0f, 0.85f, 0.3f, 1.0f) : Vec4(0.7f, 0.7f, 0.7f, 1.0f));
    render_text_centered(">", L.arrow_right_x + L.arrow_size * 0.5f,
        L.lang_center_y - lh(ts()) * 0.5f, ts(),
        hover_right ? Vec4(1.0f, 0.85f, 0.3f, 1.0f) : Vec4(0.7f, 0.7f, 0.7f, 1.0f));

    float mid_x = (L.arrow_left_x + L.arrow_size + L.arrow_right_x) * 0.5f;
    render_text_centered(lang_name, mid_x, L.lang_center_y - lh(ts()) * 0.5f,
                         ts(), Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // ---- Back button ----
    bool hover_back = hit(mouse_pos, L.back_x, L.back_y, L.back_w, L.back_h);
    Vec4 back_bg = hover_back ? Vec4(0.3f, 0.3f, 0.3f, 0.9f)
                              : Vec4(0.15f, 0.15f, 0.15f, 0.8f);
    Renderer::render_rounded_rect(
        Vec3(L.back_x, L.back_y, ZDepth::MAIN_MENU - 0.003f),
        Vec2(L.back_w, L.back_h), back_bg, 6.0f * ui);
    render_text_centered(L10N("menu.back"), L.cx,
        L.back_y + (L.back_h - lh(ts())) * 0.5f,
        ts(), Vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

// ============================================================================
// Public API
// ============================================================================

void init_main_menu() {
    g_hovered      = -1;
    g_state        = MenuState::NORMAL;
    g_suppress_esc = false;
    g_can_resume   = false;
    build_items();
}

// Persistent mouse_pos for render (render() is called after update())
static Vec2 g_last_mouse;

bool update_main_menu(Vec2 mouse_pos) {
    g_last_mouse = mouse_pos;
    switch (g_state) {
        case MenuState::NORMAL:          update_normal(mouse_pos);   break;
        case MenuState::CONFIRM_NEW_GAME: update_confirm(mouse_pos); break;
        case MenuState::SETTINGS:        update_settings(mouse_pos); break;
    }
    return true;
}

void render_main_menu() {
    switch (g_state) {
        case MenuState::NORMAL:          render_normal();                break;
        case MenuState::CONFIRM_NEW_GAME: render_confirm(g_last_mouse); break;
        case MenuState::SETTINGS:        render_settings(g_last_mouse); break;
    }
}

void set_can_resume(bool can_resume) { g_can_resume = can_resume; }
void mark_just_opened()              { g_suppress_esc = true; }

}  // namespace UI
