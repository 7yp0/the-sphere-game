#include "main_menu.h"
#include "ui.h"
#include "game/game.h"
#include "save/save_system.h"
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

enum class MenuState { NORMAL, CONFIRM_NEW_GAME };

enum class ItemID { CONTINUE, NEW_GAME, LOAD, EXIT };

struct MenuItem {
    const char* label;
    ItemID      id;
    bool        enabled;
};

// Dynamic item list — rebuilt each frame based on g_can_resume
static MenuItem g_items[4];
static int      g_item_count = 0;

static int       g_hovered      = -1;
static bool      g_can_resume   = false;
static bool      g_suppress_esc = false;
static MenuState g_state        = MenuState::NORMAL;

static void build_items() {
    g_item_count = 0;
    if (g_can_resume)
        g_items[g_item_count++] = { "Continue", ItemID::CONTINUE, true };
    g_items[g_item_count++] = { "New Game", ItemID::NEW_GAME, true };
    g_items[g_item_count++] = { "Load",     ItemID::LOAD,     SaveSystem::has_save() };
    g_items[g_item_count++] = { "Exit",     ItemID::EXIT,     true };
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
    float gap      = 8.0f  * ui;
    float title_lh = lh(tts());
    float item_lh  = lh(ts());
    L.item_step    = item_lh * 2.0f;

    float content_h  = title_lh + gap + num_items * L.item_step;
    L.panel_h = content_h + pad_v * 2.0f;
    L.panel_w = 320.0f * ui;
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

    const char* title = "THE SPHERE GAME";
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
        float iw = Renderer::calculate_text_width(g_items[i].label, ts());
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
        render_text_centered(g_items[i].label, L.cx, item_top_y(L, i), ts(), color);
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
    CL.base = compute_layout(2);  // 2 "rows" below title (question + buttons)
    Layout& L = CL.base;

    float ui     = Renderer::get_ui_text_scale();
    float qy     = L.items_start_y;
    float btn_y  = qy + lh(qs()) * 2.0f;
    float btn_w  = Renderer::calculate_text_width("Yes", ts()) + 24.0f * ui;
    float gap    = 20.0f * ui;

    CL.question_y = qy;
    CL.btn_y      = btn_y;
    CL.btn_w      = btn_w;
    CL.btn_h      = lh(ts()) * 1.4f;
    CL.yes_x      = L.cx - btn_w - gap * 0.5f;
    CL.no_x       = L.cx + gap * 0.5f;

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
    render_text_centered("Overwrite saved progress?",
                         L.cx, CL.question_y, qs(), Vec4(1.0f, 0.9f, 0.6f, 1.0f));
    render_text_centered("This cannot be undone.",
                         L.cx, CL.question_y + lh(qs()) * 1.4f,
                         qs() * 0.85f, Vec4(0.7f, 0.7f, 0.7f, 1.0f));

    // Yes button
    {
        Vec4 bg = CL.hovered == 0 ? Vec4(0.7f, 0.15f, 0.1f, 0.9f)
                                  : Vec4(0.4f, 0.1f, 0.1f, 0.8f);
        Renderer::render_rounded_rect(
            Vec3(CL.yes_x, CL.btn_y, ZDepth::MAIN_MENU - 0.003f),
            Vec2(CL.btn_w, CL.btn_h), bg, 6.0f * ui);
        render_text_centered("Yes", CL.yes_x + CL.btn_w * 0.5f,
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
        render_text_centered("No", CL.no_x + CL.btn_w * 0.5f,
                             CL.btn_y + (CL.btn_h - lh(ts())) * 0.5f,
                             ts(), Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
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
    if (g_state == MenuState::NORMAL)
        update_normal(mouse_pos);
    else
        update_confirm(mouse_pos);
    return true;
}

void render_main_menu() {
    if (g_state == MenuState::NORMAL)
        render_normal();
    else
        render_confirm(g_last_mouse);
}

void set_can_resume(bool can_resume) { g_can_resume = can_resume; }
void mark_just_opened()              { g_suppress_esc = true; }

}  // namespace UI
