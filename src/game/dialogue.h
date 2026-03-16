#pragma once
#include <string>
#include "../ecs/ecs.h"

namespace Dialogue {

// Triggers a speech bubble for the given entity with the localized string
void say(ECS::EntityID entity, const std::string& string_key);
void say(ECS::EntityID entity, const std::string& string_key, float duration);

} // namespace Dialogue
