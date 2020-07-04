
#include <iostream>
#include <obs-module.h>
#if __has_include(<obs-frontend-api.h>)
#include <obs-frontend-api.h>
#else
#include <obs-frontend-api/obs-frontend-api.h>
#endif
#include <obs-data.h>
#include <string>
#include <map>
#include <iostream>
#include <utility>
#include <QtWidgets/QAction>
#include <QtWidgets/QMainWindow>
#include <QIcon>
#include "test-control.hpp"
#include "forms/settings-dialog.h"

using namespace std;

void ___source_dummy_addref(obs_source_t *) {}
void ___sceneitem_dummy_addref(obs_sceneitem_t *) {}
void ___data_dummy_addref(obs_data_t *) {}
void ___data_array_dummy_addref(obs_data_array_t *) {}
void ___output_dummy_addref(obs_output_t *) {}

void ___data_item_dummy_addref(obs_data_item_t *) {}
void ___data_item_release(obs_data_item_t *dataItem)
{
	obs_data_item_release(&dataItem);
}

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-midi", "en-US")



void register_gui(char *path)
{

	//SettingsDialog *cont = new SettingsDialog(sw);
	QWidget *cont = new SettingsDialog();
	QIcon *ic = new QIcon(path);
	QString *na = new QString("Test Control");
	//obs_frontend_add_control_window(icon, name, settingsDialog);
	obs_frontend_add_control_window(ic, na, cont);
	bfree(path);
}

bool obs_module_load(void)
{
	char *path = obs_module_file("test.svg");
	blog(LOG_INFO, "MIDI LOADED ");
	
	register_gui(path);
	return true;
}
void obs_module_unload()
{

	blog(LOG_INFO, "goodbye!");
}
