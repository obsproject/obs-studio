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


typedef struct {
	obs_data_t *data;
	obs_data_t *value;
	const char *Type = obs_data_get_string(data, "Type");
	void setType(const char *type)
	{
		obs_data_set_string(data, "Type", type);
	};
	void setType(QString type)
	{
		obs_data_set_string(data, "Type", type.toStdString().c_str());
	};
	QString String = QString(Type);
} Trigger;
typedef struct {
	
	obs_data_t *data;
	obs_data_t *value;
	const char *Type = obs_data_get_string(data, "Type");

	void setType(const char *type)
	{
		obs_data_set_string(data, "Type", type);
	};
	void setType(QString type)
	{
		obs_data_set_string(data, "Type", type.toStdString().c_str());
	};
	QString String = QString(Type);
} Action;
typedef struct {

	Trigger trigger;
	Action action;
	obs_data_t *map;
	void init(const char *mapson)
	{
		map = obs_data_create_from_json(mapson);
		trigger.data = obs_data_create_from_json(obs_data_get_string(map,"trigger"));
		action.data = obs_data_create_from_json(obs_data_get_string(map, "action"));

	}
	void init(obs_data_t *mapson) { init(obs_data_get_json(mapson));}
	void init()
	{
		map = obs_data_create();
		trigger.data = obs_data_create();
		action.data = obs_data_create();
	}
	obs_data_t * setdata()
	{
		obs_data_set_string(map, "trigger",
				    obs_data_get_json(trigger.data));
		obs_data_set_string(map, "action",
				    obs_data_get_json(action.data));
		return map;
	}
	const char *GetJSON() { return obs_data_get_json(map); }
} Mapping;

class ControlMapper : public QObject {
	Q_OBJECT

public:
	ControlMapper();
	~ControlMapper();

	bool DebugEnabled = true;
	bool AlertsEnabled = false;
	int editRow = -1;
	bool EditMode = false;
	bool SettingsLoaded;
	bool LoadMapping();
	config_t *GetMappingStore();

	obs_data_t * CurrentTriggerString = obs_data_create();
	QString CurrentTriggerType = "Hotkeys";
	obs_data_t * CurrentActionString = obs_data_create();
	QString CurrentActionType = "OBS";

	obs_data_t * PreviousTriggerString;
	QString PreviousTriggerType;
	obs_data_t * PreviousActionString;
	QString PreviousActionType;
	int MappingExists(obs_data_t *triggerstring);
signals:
	void AddTableRow(obs_data_t* mapdata);
	void EditTableRow(int row, obs_data_t *mapdata);
	void EditTableRow(int row, obs_data_t *triggerdata,obs_data_t * actiondata);
	void AddTableRow(obs_data_t *triggerdata, obs_data_t * actiondata);
	void DoAction(obs_data_t *actionString);
	void mapLoadAction(obs_data_t *map);
	void ClearTable();
	void EditAction(QString type, obs_data_t * data);
	void EditTrigger(QString type, obs_data_t * data);
	void ResetToDefaults();
public slots:
	void UpdateTrigger(QString type, obs_data_t *string);
	void UpdateAction(QString type, obs_data_t *string);
	void SaveMapping();
	void AddorEditMapping();
	void doload(obs_data_t *map);
	void TriggerEvent(QString triggertype, obs_data_t *TriggerString);

	void deleteEntry(int row);

private:

	void SetDefaults();
	config_t *MapConfig;
	obs_data_array* MapArray;


};
