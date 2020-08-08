#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <cmath>
#include "mapper.hpp"
#include <qevent.h>
#include <util/dstr.hpp>
#include <QPointer>
#include <QStyle>
#include "obs-app.hpp"
#include "qt-wrappers.hpp"
#include <obs.hpp>
#include <functional>

#if __has_include(<obs-frontend-api.h>)

#include <obs-frontend-api.h>
#else
#include <obs-frontend-api/obs-frontend-api.h>
#endif
#define SECTION_NAME "MAPPER"
#define PARAM_DEBUG "debug_enabled"
#define PARAM_ALERT "alerts_enabled"
#define PARAM_DEVICES "Mapping"
#define DEFUALT_MAPPING "{\"mapping\": []}"

ControlMapper::ControlMapper()
	: debug_enabled(false), alerts_enabled(true), settings_loaded(false)
{
	map_config = get_mapping_store();
	set_defaults();
	connect(this, SIGNAL(map_load_action(obs_data_t *)), this,
		SLOT(do_load(obs_data_t *)));
	load_mapping();
}

ControlMapper::~ControlMapper()
{
	obs_data_array_release(map_array);
	delete this;
}

bool isJSon(QString val)
{
	return (val.startsWith(QChar('[')) || val.startsWith(QChar('{')));
}
void ControlMapper::set_defaults()
{

	if (map_config) {
		config_set_default_bool(map_config, SECTION_NAME, PARAM_DEBUG,
					debug_enabled);
		config_set_default_bool(map_config, SECTION_NAME, PARAM_ALERT,
					alerts_enabled);
		config_set_default_string(map_config, SECTION_NAME,
					  PARAM_DEVICES, DEFUALT_MAPPING);
	}
}

void ControlMapper::add_or_edit_mapping()
{

	map_config = get_mapping_store();

	obs_data_t *newson = obs_data_create();
	obs_data_set_string(newson, "action",
			    obs_data_get_json(current_action_string));
	obs_data_set_string(newson, "trigger",
			    obs_data_get_json(current_trigger_string));
	Mapping *newmap = new Mapping(newson);
	//	obs_data_array_push_back(map_array, data);
	if (!edit_mode) {
		int size = obs_data_array_count(map_array);
		obs_data_array_insert(map_array, size, newmap->setdata());
	} else {
		obs_data_array_erase(map_array, edit_row);
		obs_data_array_insert(map_array, edit_row, newmap->setdata());
	}

	save_mapping();
	load_mapping();
	edit_mode = false;
	edit_row = -1;
}
void ControlMapper::save_mapping()
{
	obs_data_t *newdata = obs_data_create();
	obs_data_set_array(newdata, "mapper", map_array);
	config_set_string(map_config, SECTION_NAME, PARAM_DEVICES,
			  obs_data_get_json(newdata));
	config_save(map_config);
	obs_data_release(newdata);
}
bool ControlMapper::load_mapping()
{
	set_defaults();
	map_config = get_mapping_store();
	debug_enabled = config_get_bool(map_config, SECTION_NAME, PARAM_DEBUG);
	alerts_enabled = config_get_bool(map_config, SECTION_NAME, PARAM_ALERT);
	obs_data_t *Data = obs_data_create_from_json(
		config_get_string(map_config, SECTION_NAME, PARAM_DEVICES));
	map_array = obs_data_get_array(Data, "mapper");
	emit(clear_table());
	for (int i = 0; i < obs_data_array_count(map_array); i++) {
		obs_data_t *data = obs_data_array_item(map_array, i);
		emit(map_load_action(data)); // executes do_load
	}
	return true;
}
config_t *ControlMapper::get_mapping_store()
{
	return obs_frontend_get_global_config();
}

void ControlMapper::delete_entry(int entry)
{
	map_config = get_mapping_store();
	obs_data_array_erase(map_array, entry);
	obs_data_t *newdata = obs_data_create();
	obs_data_set_array(newdata, "mapper", map_array);
	config_set_string(map_config, SECTION_NAME, PARAM_DEVICES,
			  obs_data_get_json(newdata));
	config_save(map_config);
	obs_data_release(newdata);
}

int ControlMapper::mapping_exists(obs_data_t *mapping)
{

	int count = obs_data_array_count(map_array);
	const char *X;
	if (isJSon(obs_data_get_string(mapping, "value"))) {
		X = obs_data_get_json(obs_data_create_from_json(
			obs_data_get_string(mapping, "value")));
	} else {
		X = "";
	}

	obs_data_set_string(mapping, "value", "");

	for (int i = 0; i < count; i++) {
		obs_data_t *item = obs_data_array_item(map_array, i);
		obs_data_t *listitem = obs_data_create_from_json(
			obs_data_get_string(item, "trigger"));
		obs_data_set_string(listitem, "value", "");

		if (QString(obs_data_get_json(mapping)).simplified() ==
		    QString(obs_data_get_json(listitem)).simplified()) {
			obs_data_release(listitem);
			obs_data_set_string(mapping, "value", X);
			return i;
		}
		obs_data_release(listitem);
	}
	obs_data_set_string(mapping, "value", X);
	return -1;
}
void ControlMapper::trigger_event(QString type, obs_data_t *triggerstring)
{
	int x = mapping_exists(triggerstring);
	if (x != -1) {
		obs_data_t *data = obs_data_array_item(map_array, x);
		obs_data_t *senddata = obs_data_create_from_json(
			obs_data_get_string(data, "action"));
		obs_data_set_string(senddata, "value",
				    obs_data_get_string(triggerstring,
							"value"));
		emit(do_action(senddata));
	} else {
	}
}
void ControlMapper::update_trigger(obs_data_t *trigger_string)
{
	previous_trigger_string = current_trigger_string;
	current_trigger_string = trigger_string;
}
void ControlMapper::update_action(obs_data_t *action_string)
{
	previous_action_string = current_action_string;
	current_action_string = action_string;
}

void ControlMapper::do_load(obs_data_t *mapa)
{
	Mapping *newmap = new Mapping(mapa);
	emit(add_table_row(newmap->trigger, newmap->action));
}

/********************Trigger Functions*********************/

/********************Mapping Functions*********************/

Mapping::Mapping(obs_data_t *mapson)
{
	map = mapson;
	trigger = obs_data_create_from_json(
		obs_data_get_string(mapson, "trigger"));
	action = obs_data_create_from_json(
		obs_data_get_string(mapson, "action"));
}
Mapping::Mapping()
{
	map = obs_data_create();
	trigger = obs_data_create();
	action = obs_data_create();
}
obs_data_t *Mapping::setdata()
{
	obs_data_set_string(map, "trigger", obs_data_get_json(trigger));
	obs_data_set_string(map, "action", obs_data_get_json(action));
	return map;
}
