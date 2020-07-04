#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <cmath>
#include "mapper.hpp"
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qjsondocument.h>
#if __has_include(<obs-frontend-api.h>)

#include <obs-frontend-api.h>
#else
#include <obs-frontend-api/obs-frontend-api.h>
#endif
#define SECTION_NAME "MAPPER"
#define PARAM_DEBUG "DebugEnabled"
#define PARAM_ALERT "AlertsEnabled"
#define PARAM_DEVICES "Mapping"
ControlMapper::ControlMapper() {
	obs_frontend_add_event_callback(OnFrontendEvent, this);
}
ControlMapper::~ControlMapper() {
	obs_frontend_add_event_callback(OnFrontendEvent, this);
}
QString ControlMapper::BroadcastControlEvent(QString input, QString inputAction,
					     QString output,
					     QString outputAction)
{
	emit(ControlMapper::EventCall(input,inputAction,
				      output,outputAction));
	return QString("return");
}
Mapping ControlMapper::MakeMapping(QString inType, QString inAct,
				  QString outType, QString OutAct)
{
	int beforesize = map.size();
	ControlMapper::map.push_back(Mapping());
	map[beforesize + 1].input = inType;
	map[beforesize + 1].inputAction = inAct;
	map[beforesize + 1].output = outType;
	map[beforesize + 1].outputAction = OutAct;
	return map[beforesize + 1];
	
}

bool ControlMapper::SaveMapping(Mapping entry)
{
	config_t *obsConfig = GetMappingStore();
	obs_frontend_add_event_callback(OnFrontendEvent, this);
	for (int i=0; i < map.size(); i++) {	
		
		QString *x = new QString("{'Input':'" + map.at(i).input + "','InputAction':'" +
			map.at(i).inputAction + "','Output':'" +
			map.at(i).output + "','OutputAction':'" +
			map.at(i).outputAction+"'}");
		config_get_string(obsConfig, SECTION_NAME, PARAM_DEVICES);
		QString sector = "MAPS" + QString::number(i);
			config_set_string(obsConfig, SECTION_NAME, sector.toStdString().c_str(),x->toStdString().c_str());
	}
	//obs_data_release(obsConfig);
	return true;
}
bool ControlMapper::LoadMapping()
{
	config_t *obsConfig = GetMappingStore();
	
	obs_data_t *deviceManagerData = obs_data_create_from_json(config_get_string(obsConfig, SECTION_NAME, PARAM_DEVICES));
	
	return true;
}
config_t* ControlMapper::GetMappingStore()
{
	return obs_frontend_get_profile_config();
}
void ControlMapper::OnFrontendEvent(enum obs_frontend_event event, void *param)
{
	ControlMapper *config = reinterpret_cast<ControlMapper *>(param);

	if (event == OBS_FRONTEND_EVENT_PROFILE_CHANGED) {
		config->LoadMapping();
	}
}
void ControlMapper::updateInputString(QString inputstring) {
	PreviousInputString = CurrentInputString;
	CurrentInputString = inputstring;
}
void ControlMapper::updateOutputString(QString outputstring)
{
	PreviousOutputString = CurrentOutputString;
	CurrentOutputString = outputstring;
}
