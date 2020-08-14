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
#define PARAM_DEBUG "DebugEnabled"
#define PARAM_ALERT "AlertsEnabled"
#define PARAM_DEVICES "Mapping"
#define DEFUALT_MAPPING "{\"mapping\": []}"


ControlMapper::ControlMapper()
	: DebugEnabled(false), AlertsEnabled(true), SettingsLoaded(false)
{
	MapConfig = GetMappingStore();
	SetDefaults();
	connect(this, SIGNAL(mapLoadAction(obs_data_t *)), this,
		SLOT(doload(obs_data_t *)));
	LoadMapping();
}


ControlMapper::~ControlMapper()
{
	obs_data_array_release(MapArray);
	delete this;
}

bool isJSon(QString val)
{
	return (val.startsWith(QChar('[')) || val.startsWith(QChar('{')));
}
void ControlMapper::SetDefaults()
{
	// OBS Config defaults
	//obs_data_set_string(CurrentActionString, "action", "Start Streaming");

	if (MapConfig) {
		config_set_default_bool(MapConfig, SECTION_NAME, PARAM_DEBUG,
					DebugEnabled);
		config_set_default_bool(MapConfig, SECTION_NAME, PARAM_ALERT,
					AlertsEnabled);
		config_set_default_string(MapConfig, SECTION_NAME,
					  PARAM_DEVICES, DEFUALT_MAPPING);
	}
}

void ControlMapper::AddorEditMapping()
{

	MapConfig = GetMappingStore();
	Mapping *newmap = new Mapping();
	obs_data_t *newson = obs_data_create();
	obs_data_set_string(newson, "action", obs_data_get_json(CurrentActionString));
	obs_data_set_string(newson, "trigger", obs_data_get_json(CurrentTriggerString));

	newmap->init(obs_data_get_json(newson));
	newmap->trigger.setType(CurrentTriggerType);
	newmap->action.setType(CurrentActionType);


	//	obs_data_array_push_back(MapArray, data);
	if (!EditMode) {
		int size = obs_data_array_count(MapArray);
		obs_data_array_insert(MapArray, size, newmap->setdata());
	} else {
		obs_data_array_erase(MapArray, editRow);
		obs_data_array_insert(MapArray, editRow, newmap->setdata());
	}

	SaveMapping();
	LoadMapping();
	EditMode = false;
	editRow = -1;
}
void ControlMapper::SaveMapping()
{
	obs_data_t * newdata = obs_data_create();
	obs_data_set_array(newdata, "mapper", MapArray);
	config_set_string(MapConfig, SECTION_NAME, PARAM_DEVICES, obs_data_get_json(newdata));
	config_save(MapConfig);
	obs_data_release(newdata);
}
bool ControlMapper::LoadMapping()
{
	SetDefaults();
	MapConfig = GetMappingStore();

	DebugEnabled = config_get_bool(MapConfig, SECTION_NAME, PARAM_DEBUG);
	AlertsEnabled = config_get_bool(MapConfig, SECTION_NAME, PARAM_ALERT);

	obs_data_t * Data = obs_data_create_from_json(
		config_get_string(MapConfig, SECTION_NAME, PARAM_DEVICES));

	MapArray = obs_data_get_array(Data, "mapper");

	emit(ClearTable());
	for (int i = 0; i < obs_data_array_count(MapArray); i++) {
		obs_data_t *data = obs_data_array_item(MapArray, i);
		emit(mapLoadAction(data));
		blog(1, "mapping load");
	}

	return true;
}
config_t *ControlMapper::GetMappingStore()
{
	return obs_frontend_get_global_config();
}

void ControlMapper::deleteEntry(int entry)
{

	MapConfig = GetMappingStore();
	obs_data_array_erase(MapArray, entry);
	obs_data_t * newdata = obs_data_create();
	obs_data_set_array(newdata, "mapper", MapArray);
	config_set_string(MapConfig, SECTION_NAME, PARAM_DEVICES,
			  obs_data_get_json(newdata));
	config_save(MapConfig);
	obs_data_release(newdata);
}

int ControlMapper::MappingExists(obs_data_t *mapping)
{

	int count = obs_data_array_count(MapArray);
	const char *X;
	if (isJSon(obs_data_get_string(mapping, "value"))) {
		X = obs_data_get_json(obs_data_create_from_json(
			obs_data_get_string(mapping, "value")));
	} else {
		X = "";
	}

	obs_data_set_string(mapping, "value", "");

	for (int i = 0; i < count; i++) {
		obs_data_t *item = obs_data_array_item(MapArray, i);
		obs_data_t * listitem = obs_data_create_from_json(
			obs_data_get_string(item, "triggerstring"));
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
void ControlMapper::TriggerEvent(QString type, obs_data_t *triggerstring)
{
	int x = MappingExists(triggerstring);
	if (x != -1) {
		obs_data_t *data = obs_data_array_item(MapArray, x);
		obs_data_t * senddata = obs_data_create_from_json(
			obs_data_get_string(data, "triggerstring"));
		obs_data_set_string(senddata, "value",
				    obs_data_get_string(triggerstring,
							"value"));
		//controller->execute(senddata);
		emit(DoAction(senddata));
	} else {
	}
}
void ControlMapper::UpdateTrigger(QString type, obs_data_t *triggerstring)
{
	PreviousTriggerString = CurrentTriggerString;
	PreviousTriggerType = CurrentTriggerType;
	CurrentTriggerString = triggerstring;
	CurrentTriggerType = type;
	//triggerEvent(type, triggerstring);
}
void ControlMapper::UpdateAction(QString type, obs_data_t *outputstring)
{
	PreviousActionString = CurrentActionString;
	PreviousActionType = CurrentActionType;
	CurrentActionString = outputstring;
	CurrentActionType = type;
}

void ControlMapper::doload(obs_data_t *mapa)
{

	Mapping *newmap = new Mapping();
	newmap->init(mapa);
	emit(AddTableRow(newmap->trigger.data,newmap->action.data));
}

