#include "settings/lcs-config.hpp"
#include "transcription/engine-registry.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/config-file.h>

#include <cstring>

namespace lcs {

namespace {

PluginSettings g_settings;
const char *const kSaveKey = "live_caption_studio";

EngineKind kind_from_string(const char *value)
{
	if (!value)
		return EngineKind::Whisper;
	if (strcmp(value, "openai") == 0)
		return EngineKind::OpenAI;
	if (strcmp(value, "gemini") == 0)
		return EngineKind::Gemini;
	if (strcmp(value, "ollama") == 0)
		return EngineKind::Ollama;
	if (strcmp(value, "custom") == 0)
		return EngineKind::CustomApi;
	return EngineKind::Whisper;
}

const char *kind_to_string(EngineKind kind)
{
	switch (kind) {
	case EngineKind::OpenAI:
		return "openai";
	case EngineKind::Gemini:
		return "gemini";
	case EngineKind::Ollama:
		return "ollama";
	case EngineKind::CustomApi:
		return "custom";
	case EngineKind::Whisper:
	default:
		return "whisper";
	}
}

void save_callback(obs_data_t *save_data, bool saving, void *)
{
	if (saving) {
		obs_data_t *obj = obs_data_create();
		save_settings(obj, g_settings);
		obs_data_set_obj(save_data, kSaveKey, obj);
		obs_data_release(obj);
	} else {
		obs_data_t *obj = obs_data_get_obj(save_data, kSaveKey);
		if (obj) {
			g_settings = load_settings(obj);
			EngineRegistry::instance().set_config(g_settings.engine);
			obs_data_release(obj);
		}
	}
}

} // namespace

PluginSettings load_settings(obs_data_t *obj)
{
	PluginSettings settings;
	if (!obj)
		return settings;

	settings.show_dock_on_startup = obs_data_get_bool(obj, "show_dock_on_startup");
	settings.overlay_enabled = obs_data_get_bool(obj, "overlay_enabled");

	obs_data_t *engine = obs_data_get_obj(obj, "engine");
	if (engine) {
		settings.engine.kind = kind_from_string(obs_data_get_string(engine, "kind"));
		settings.engine.api_key = obs_data_get_string(engine, "api_key");
		settings.engine.endpoint = obs_data_get_string(engine, "endpoint");
		settings.engine.model = obs_data_get_string(engine, "model");
		settings.engine.source_language = obs_data_get_string(engine, "source_language");
		settings.engine.target_language = obs_data_get_string(engine, "target_language");
		settings.engine.translate = obs_data_get_bool(engine, "translate");
		settings.engine.ai_correct = obs_data_get_bool(engine, "ai_correct");
		obs_data_release(engine);
	}

	if (settings.engine.source_language.empty())
		settings.engine.source_language = "auto";

	return settings;
}

void save_settings(obs_data_t *obj, const PluginSettings &settings)
{
	obs_data_set_bool(obj, "show_dock_on_startup", settings.show_dock_on_startup);
	obs_data_set_bool(obj, "overlay_enabled", settings.overlay_enabled);

	obs_data_t *engine = obs_data_create();
	obs_data_set_string(engine, "kind", kind_to_string(settings.engine.kind));
	obs_data_set_string(engine, "api_key", settings.engine.api_key.c_str());
	obs_data_set_string(engine, "endpoint", settings.engine.endpoint.c_str());
	obs_data_set_string(engine, "model", settings.engine.model.c_str());
	obs_data_set_string(engine, "source_language", settings.engine.source_language.c_str());
	obs_data_set_string(engine, "target_language", settings.engine.target_language.c_str());
	obs_data_set_bool(engine, "translate", settings.engine.translate);
	obs_data_set_bool(engine, "ai_correct", settings.engine.ai_correct);
	obs_data_set_obj(obj, "engine", engine);
	obs_data_release(engine);
}

void init_settings()
{
	obs_frontend_add_save_callback(save_callback, nullptr);

	config_t *config = obs_frontend_get_profile_config();
	if (config) {
		const char *kind = config_get_string(config, "LiveCaptionStudio", "EngineKind");
		if (kind && *kind)
			g_settings.engine.kind = kind_from_string(kind);
	}

	EngineRegistry::instance().set_config(g_settings.engine);
}

void free_settings()
{
	obs_frontend_remove_save_callback(save_callback, nullptr);
}

PluginSettings current_settings()
{
	return g_settings;
}

void update_settings(const PluginSettings &settings)
{
	g_settings = settings;
	EngineRegistry::instance().set_config(g_settings.engine);

	config_t *config = obs_frontend_get_profile_config();
	if (config) {
		config_set_string(config, "LiveCaptionStudio", "EngineKind", kind_to_string(settings.engine.kind));
		config_save_safe(config, "tmp", nullptr);
	}
}

} // namespace lcs
