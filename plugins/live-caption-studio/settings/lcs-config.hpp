#pragma once

#include "transcription/engine.hpp"

#include <obs-data.h>

namespace lcs {

struct PluginSettings {
	EngineConfig engine;
	bool show_dock_on_startup = true;
	bool overlay_enabled = true;
};

PluginSettings load_settings(obs_data_t *obj);
void save_settings(obs_data_t *obj, const PluginSettings &settings);

void init_settings();
void free_settings();

PluginSettings current_settings();
void update_settings(const PluginSettings &settings);

} // namespace lcs
