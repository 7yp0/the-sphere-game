#include "dialogue.h"
#include "game.h"
#include "../debug/debug_log.h"
#include "../core/localization.h"
#include "../core/voice.h"
#include "../ecs/components/talkable.h"
#include "../ui/speechbubble_ui.h"
#include <cstdio>

namespace Dialogue {

void say(ECS::EntityID entity, const std::string& string_key) {
    // Lookup localized text first
    const std::string& text = Localization::get(string_key);
    // Duration: 1.5s + 0.05s per char, clamp 1.5-5.0s
    float duration = 1.5f + 0.05f * text.length();
    if (duration > 5.0f) duration = 5.0f;
    if (duration < 1.5f) duration = 1.5f;
    say(entity, string_key, duration);
}

void say(ECS::EntityID entity, const std::string& string_key, float duration) {
    auto* talkable = Game::g_state.ecs_world.get_component<ECS::TalkableComponent>(entity);
    if (!talkable) {
        DEBUG_ERROR("[Dialogue] Entity %d is not talkable!", entity);
        return;
    }
    // Lookup localized text
    const std::string& text = Localization::get(string_key);
    // Lookup voice file path (not played here)
    std::string voice_path = Voice::get_voice_path(string_key, Localization::get_language());

    // Show speech bubble – position is looked up dynamically in render_speechbubbles()
    UI::show_speechbubble(entity, text, talkable->text_color, duration);

    // TODO: Play voice audio if needed
}

} // namespace Dialogue
