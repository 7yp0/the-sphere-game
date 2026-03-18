#pragma once
#include <string>

namespace Settings {

float              get_music_volume();
float              get_sfx_volume();
float              get_voice_volume();
const std::string& get_language();

void set_music_volume(float v);
void set_sfx_volume(float v);
void set_voice_volume(float v);
void set_language(const std::string& lang);

// Load from settings.json (or keep defaults if file not found)
void init();

// Write current values to settings.json
void save();

} // namespace Settings
