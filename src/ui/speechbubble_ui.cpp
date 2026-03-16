#include "../core/timing.h"
#include "speechbubble_ui.h"
#include "../renderer/text.h"
#include "../game/game.h"
#include "../ecs/components/talkable.h"
#include "../ecs/components/all_components.h"
#include <vector>

namespace UI {

static constexpr float TYPEWRITER_CHARS_PER_SEC = 25.0f;

struct Speechbubble {
    ECS::EntityID entity;
    std::string text;
    Vec4 color;
    float start_time;  // when the bubble appeared (for typewriter)
    float expiry_time;
};

static std::vector<Speechbubble> g_bubbles;

void show_speechbubble(ECS::EntityID entity, const std::string& text, const Vec4& color, float duration) {
    float now = Core::get_time_seconds();
    // Pro Entity nur eine Bubble erlauben
    g_bubbles.erase(std::remove_if(g_bubbles.begin(), g_bubbles.end(), [&](const Speechbubble& b) {
        return b.entity == entity;
    }), g_bubbles.end());
    g_bubbles.push_back({entity, text, color, now, now + duration});
}

void render_speechbubbles() {
    float now = Core::get_time_seconds();
    g_bubbles.erase(std::remove_if(g_bubbles.begin(), g_bubbles.end(), [now](const Speechbubble& b) {
        return b.expiry_time <= now;
    }), g_bubbles.end());

    for (const auto& bubble : g_bubbles) {
        // Look up current entity position every frame so bubble tracks the entity
        auto* t2d     = Game::g_state.ecs_world.get_component<ECS::Transform2_5DComponent>(bubble.entity);
        auto* talkable = Game::g_state.ecs_world.get_component<ECS::TalkableComponent>(bubble.entity);
        auto* sprite  = Game::g_state.ecs_world.get_component<ECS::SpriteComponent>(bubble.entity);
        if (!t2d || !talkable) continue;

        // Compute the depth-scaled visual sprite height so the bubble sits
        // consistently above the entity's head regardless of depth/distance.
        // Assumes BOTTOM_CENTER pivot: t2d->position is the entity's feet.
        float visual_height_base = 0.0f;
        if (sprite) {
            float depth_scale = ECS::TransformHelpers::compute_depth_scale(t2d->z_depth);
            visual_height_base = sprite->base_size.y * t2d->scale.y * depth_scale;
        }

        // Anchor = visual head position + small gap (bubble_offset.y as gap in BASE coords)
        Vec2 anchor = Vec2(
            (t2d->position.x + talkable->bubble_offset.x) * UI::UI_SCALE(),
            (t2d->position.y - visual_height_base + talkable->bubble_offset.y) * UI::UI_SCALE()
        );

        float padding = UI::bubble_padding();
        float scale = Renderer::get_ui_text_scale();
        float text_width = Renderer::calculate_text_width(bubble.text.c_str(), scale);
        float visual_h   = Renderer::GLYPH_VISUAL_HEIGHT * scale;

        float bg_width  = text_width + padding;
        float bg_height = visual_h   + padding / 2.0f;

        // Center horizontally, bubble bottom sits at anchor (expands upward)
        Vec2 bg_pos = Vec2(anchor.x - bg_width / 2.0f, anchor.y - bg_height);

        // Clamp to keep bubble fully visible
        bg_pos.x = std::max(0.0f, std::min(bg_pos.x, (float)Renderer::get_ui_fbo_width()  - bg_width));
        bg_pos.y = std::max(0.0f, std::min(bg_pos.y, (float)Renderer::get_ui_fbo_height() - bg_height));

        Renderer::render_rounded_rect(Vec3(bg_pos, ZDepth::DIALOGUE), Vec2(bg_width, bg_height),
                                      Vec4(0.0f, 0.0f, 0.0f, 0.8f), bg_height/2);

        Vec2 text_pos = Vec2(
            bg_pos.x + (bg_width  - text_width) / 2.0f,
            bg_pos.y + (bg_height - visual_h)   / 2.0f - Renderer::GLYPH_TOP_PADDING * scale
        );
        int chars_visible = (int)((now - bubble.start_time) * TYPEWRITER_CHARS_PER_SEC);
        Renderer::render_text(bubble.text.c_str(), text_pos, scale, bubble.color, chars_visible);
    }
}

void clear_speechbubbles() {
    g_bubbles.clear();
}

} // namespace UI
