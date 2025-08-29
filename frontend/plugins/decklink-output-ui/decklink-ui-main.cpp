#include <obs.hpp>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QAction>
#include <util/util.hpp>
#include <util/platform.h>
#include "DecklinkOutputUI.h"
#include "../../../plugins/decklink/const.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("decklink-output-ui", "en-US")

DecklinkOutputUI *doUI;

bool shutting_down = false;

bool main_output_running = false;
bool preview_output_running = false;

struct decklink_ui_output {
	bool enabled;

	OBSCanvasAutoRelease canvas;
	OBSOutputAutoRelease output;
};

static struct decklink_ui_output context = {0};
static struct decklink_ui_output context_preview = {0};

OBSData load_settings()
{
	BPtr<char> path = obs_module_get_config_path(obs_current_module(), "decklinkOutputProps.json");
	BPtr<char> jsonData = os_quick_read_utf8_file(path);
	if (!!jsonData) {
		obs_data_t *data = obs_data_create_from_json(jsonData);
		OBSData dataRet(data);
		obs_data_release(data);

		return dataRet;
	}

	return nullptr;
}

void output_stop()
{
	obs_output_stop(context.output);

	context.canvas = nullptr;
	context.output = nullptr;

	main_output_running = false;

	if (!shutting_down)
		doUI->OutputStateChanged(false);
}

void output_start()
{
	OBSData settings = load_settings();

	if (settings != nullptr) {
		context.output = obs_output_create("decklink_output", "decklink_output", settings, NULL);
		context.canvas = obs_get_main_canvas();

		obs_output_set_media(context.output, obs_canvas_get_video(context.canvas), obs_get_audio());
		bool started = obs_output_start(context.output);

		main_output_running = started;

		if (!shutting_down)
			doUI->OutputStateChanged(started);

		if (!started)
			output_stop();
	}
}

void output_toggle()
{
	if (main_output_running)
		output_stop();
	else
		output_start();
}

OBSData load_preview_settings()
{
	BPtr<char> path = obs_module_get_config_path(obs_current_module(), "decklinkPreviewOutputProps.json");
	BPtr<char> jsonData = os_quick_read_utf8_file(path);
	if (!!jsonData) {
		obs_data_t *data = obs_data_create_from_json(jsonData);
		OBSData dataRet(data);
		obs_data_release(data);

		return dataRet;
	}

	return nullptr;
}

void on_preview_scene_changed(enum obs_frontend_event event, void *param);

void set_current_preview_scene()
{
	OBSSourceAutoRelease source = obs_frontend_get_current_preview_scene();
	obs_canvas_set_channel(context_preview.canvas, 0, source);
}

void preview_output_stop()
{
	obs_output_stop(context_preview.output);

	context_preview.canvas = nullptr;
	context_preview.output = nullptr;

	preview_output_running = false;

	if (!shutting_down)
		doUI->PreviewOutputStateChanged(false);
}

void preview_output_start()
{
	OBSData settings = load_preview_settings();

	if (settings != nullptr) {
		context_preview.output = obs_output_create("decklink_output", "decklink_output", settings, NULL);

		obs_video_info ovi;
		obs_get_video_info(&ovi);
		context_preview.canvas = obs_canvas_create_private(nullptr, &ovi, DEVICE);

		set_current_preview_scene();

		obs_output_set_media(context_preview.output, obs_canvas_get_video(context_preview.canvas),
				     obs_get_audio());
		bool started = obs_output_start(context_preview.output);

		preview_output_running = started;
		if (!shutting_down)
			doUI->PreviewOutputStateChanged(started);

		if (!started)
			preview_output_stop();
	}
}

void preview_output_toggle()
{
	if (preview_output_running)
		preview_output_stop();
	else
		preview_output_start();
}

void on_preview_scene_changed(enum obs_frontend_event event, void * /* param */)
{
	if (event == OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED)
		set_current_preview_scene();
}

void addOutputUI(void)
{
	QAction *action = (QAction *)obs_frontend_add_tools_menu_qaction(obs_module_text("Decklink Output"));

	QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();

	obs_frontend_push_ui_translation(obs_module_get_string);
	doUI = new DecklinkOutputUI(window);
	obs_frontend_pop_ui_translation();

	auto cb = []() {
		doUI->ShowHideDialog();
	};

	action->connect(action, &QAction::triggered, cb);
}

static void OBSEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		OBSData settings = load_settings();

		if (settings && obs_data_get_bool(settings, "auto_start"))
			output_start();

		OBSData previewSettings = load_preview_settings();

		if (previewSettings && obs_data_get_bool(previewSettings, "auto_start"))
			preview_output_start();
	} else if (event == OBS_FRONTEND_EVENT_EXIT) {
		shutting_down = true;

		if (preview_output_running)
			preview_output_stop();

		if (main_output_running)
			output_stop();
	}
}

bool obs_module_load(void)
{
	return true;
}

void obs_module_unload(void)
{
	shutting_down = true;

	if (preview_output_running)
		preview_output_stop();

	if (main_output_running)
		output_stop();
}

void obs_module_post_load(void)
{
	if (!obs_get_module("decklink"))
		return;

	addOutputUI();

	obs_frontend_add_event_callback(OBSEvent, nullptr);
}
