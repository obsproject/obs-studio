#include "ai-dock/ai-control-dock.hpp"
#include "captions/caption-filter.h"
#include "settings/lcs-config.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QMainWindow>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("live-caption-studio", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Live Caption Studio — live transcription, captions, and AI overlays";
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return "Live Caption Studio";
}

static void frontend_event(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		blog(LOG_INFO, "[Live Caption Studio] Frontend ready");
	}
}

bool obs_module_load(void)
{
	blog(LOG_INFO, "[Live Caption Studio] Loading plugin");

	lcs_register_caption_filter();
	lcs::init_settings();

	obs_frontend_push_ui_translation(obs_module_get_string);

	QMainWindow *main = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	auto *dock = new AIControlDock(main);
	const bool ok = obs_frontend_add_dock_by_id("lcsAIControlDock", obs_module_text("LCS.Dock.Title"), dock);
	if (!ok) {
		blog(LOG_WARNING, "[Live Caption Studio] Failed to register AI dock (duplicate id?)");
		delete dock;
	}

	obs_frontend_pop_ui_translation();
	obs_frontend_add_event_callback(frontend_event, nullptr);

	blog(LOG_INFO, "[Live Caption Studio] Plugin loaded");
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(frontend_event, nullptr);
	lcs::free_settings();
	obs_frontend_remove_dock("lcsAIControlDock");
	blog(LOG_INFO, "[Live Caption Studio] Plugin unloaded");
}
