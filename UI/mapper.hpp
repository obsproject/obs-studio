#pragma once
#if __has_include(<obs-frontend-api.h>)
#include <obs-frontend-api.h>
#else
#include <obs-frontend-api/obs-frontend-api.h>
#endif
#include <util/config-file.h>
#include <QtCore/QString>
#include <QtCore/QSharedPointer>
#include <obs.hpp>
#include <QWidget>
#include <QPointer>
#include <QDoubleSpinBox>
#include <QStackedWidget>
#include <vector>
#include <map>


class Mapping : public QObject
{
	Q_OBJECT

public:

	Mapping();
	Mapping(obs_data_t *mapson);
	obs_data_t *trigger;
	obs_data_t* action;
	obs_data_t *map;
	obs_data_t *setdata();
	const char *get_JSON() { return obs_data_get_json(map); }
};

class ControlMapper : public QObject {
	Q_OBJECT

public:
	ControlMapper();
	~ControlMapper();
	bool debug_enabled = true;
	bool alerts_enabled = false;
	int  edit_row = -1;
	bool edit_mode = false;
	bool settings_loaded;
	
	obs_data_t *current_trigger_string = obs_data_create();
	obs_data_t *current_action_string = obs_data_create();
	obs_data_t *previous_trigger_string;
	obs_data_t *previous_action_string;
	config_t *get_mapping_store();
	int mapping_exists(obs_data_t *triggerstring);
signals:
	void add_table_row(obs_data_t *mapdata);
	void add_table_row(obs_data_t *triggerdata, obs_data_t *actiondata);
	void edit_table_row(int row, obs_data_t *mapdata);
	void edit_table_row(int row, obs_data_t *triggerdata,
			  obs_data_t *actiondata);
	void do_action(obs_data_t *actionString);
	void map_load_action(obs_data_t *map);
	void clear_table();
	void edit_action(obs_data_t *data);
	void edit_trigger(obs_data_t *data);
	void reset_to_defaults();
public slots:
	bool load_mapping();
	void update_trigger(obs_data_t *string);
	void update_action(obs_data_t *string);
	void save_mapping();
	void add_or_edit_mapping();
	void do_load(obs_data_t *map);
	void trigger_event(QString triggertype, obs_data_t *TriggerString);

	void delete_entry(int row);

private:
	void set_defaults();
	config_t *map_config;
	obs_data_array *map_array;
};
