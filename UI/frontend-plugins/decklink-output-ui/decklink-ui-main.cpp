#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QAction>
#include <util/util.hpp>
#include <util/platform.h>
#include "DecklinkOutputUI.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("decklink-output-ui", "en-US")

DecklinkOutputUI *doUI;

bool main_output_running = false;

obs_output_t *output;

OBSData load_settings()
{
	BPtr<char> path = obs_module_get_config_path(obs_current_module(),
			"decklinkOutputProps.json");
	BPtr<char> jsonData = os_quick_read_utf8_file(path);
	if (!!jsonData) {
		obs_data_t *data = obs_data_create_from_json(jsonData);
		OBSData dataRet(data);
		obs_data_release(data);

		return dataRet;
	}

	return nullptr;
}

void output_start()
{
	if (!main_output_running) {
		OBSData settings = load_settings();

		if (settings != nullptr) {
			output = obs_output_create("decklink_output", 
					"decklink_output", settings, NULL);

			obs_output_start(output);
			obs_data_release(settings);

			main_output_running = true;
		}
	}
}

void output_stop()
{
	if (main_output_running) {
		obs_output_stop(output);
		obs_output_release(output);
		main_output_running = false;
	}
}

void addOutputUI(void)
{
	QAction *action = (QAction*)obs_frontend_add_tools_menu_qaction(
			obs_module_text("Decklink Output"));

	QMainWindow *window = (QMainWindow*)obs_frontend_get_main_window();

	doUI = new DecklinkOutputUI(window);

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
	}
}

bool obs_module_load(void)
{
	addOutputUI();

	obs_frontend_add_event_callback(OBSEvent, nullptr);

	return true;
}
