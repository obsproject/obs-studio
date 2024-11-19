#include "window-basic-transition-audio.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"

#ifndef NSEC_PER_MSEC
#define NSEC_PER_MSEC 1000000ll
#endif

OBSBasicTransitionAudio::OBSBasicTransitionAudio(QWidget *parent, OBSSource source_)
	: QDialog(parent),
	  ui(new Ui::OBSBasicTransitionAudio),
	  main(qobject_cast<OBSBasic *>(parent)),
	  source(source_)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	QString sourceName = QT_UTF8(obs_source_get_name(source));
	setWindowTitle(QTStr("%1 - %2").arg(QTStr("Basic.AdvAudio"), sourceName));

	signal_handler_t *handler = obs_source_get_signal_handler(source);
	sourceUpdateSignal.Connect(handler, "update", OBSSourceUpdated, this);

	VolumeType volType = (VolumeType)config_get_int(App()->GetUserConfig(), "BasicWindow", "AdvAudioVolumeType");
	if (volType == VolumeType::Percent)
		ui->usePercent->setChecked(true);
	SetVolumeType(volType);

	const char *speakers = config_get_string(main->Config(), "Audio", "ChannelSetup");
	if (strcmp(speakers, "Mono") == 0) {
		ui->balance->setEnabled(false);
	} else {
		ui->balance->setEnabled(true);
	}

	ui->balWidget->setFixedWidth(150);
	ui->stackedWidget->setFixedWidth(100);
	ui->syncOffset->setFixedWidth(100);

	if (obs_audio_monitoring_available()) {
		ui->monitoringType->addItem(QTStr("Basic.AdvAudio.Monitoring.None"), (int)OBS_MONITORING_TYPE_NONE);
		ui->monitoringType->addItem(QTStr("Basic.AdvAudio.Monitoring.MonitorOnly"),
					    (int)OBS_MONITORING_TYPE_MONITOR_ONLY);
		ui->monitoringType->addItem(QTStr("Basic.AdvAudio.Monitoring.Both"),
					    (int)OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT);
	} else {
		ui->monitoringType->setEnabled(false);
	}

	ui->mixer1->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track1"));
	ui->mixer2->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track2"));
	ui->mixer3->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track3"));
	ui->mixer4->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track4"));
	ui->mixer5->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track5"));
	ui->mixer6->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track6"));

	QWidget::connect(ui->volumeBox, SIGNAL(valueChanged(double)), this, SLOT(volumeChanged(double)));
	QWidget::connect(ui->percentBox, SIGNAL(valueChanged(int)), this, SLOT(percentChanged(int)));
	QWidget::connect(ui->forceMono, SIGNAL(clicked(bool)), this, SLOT(downmixMonoChanged(bool)));
	QWidget::connect(ui->balance, SIGNAL(valueChanged(int)), this, SLOT(balanceChanged(int)));
	QWidget::connect(ui->balance, SIGNAL(doubleClicked()), this, SLOT(ResetBalance()));
	QWidget::connect(ui->syncOffset, SIGNAL(valueChanged(int)), this, SLOT(syncOffsetChanged(int)));
	if (obs_audio_monitoring_available())
		QWidget::connect(ui->monitoringType, SIGNAL(currentIndexChanged(int)), this,
				 SLOT(monitoringTypeChanged(int)));
	QWidget::connect(ui->mixer1, SIGNAL(clicked(bool)), this, SLOT(mixer1Changed(bool)));
	QWidget::connect(ui->mixer2, SIGNAL(clicked(bool)), this, SLOT(mixer2Changed(bool)));
	QWidget::connect(ui->mixer3, SIGNAL(clicked(bool)), this, SLOT(mixer3Changed(bool)));
	QWidget::connect(ui->mixer4, SIGNAL(clicked(bool)), this, SLOT(mixer4Changed(bool)));
	QWidget::connect(ui->mixer5, SIGNAL(clicked(bool)), this, SLOT(mixer5Changed(bool)));
	QWidget::connect(ui->mixer6, SIGNAL(clicked(bool)), this, SLOT(mixer6Changed(bool)));
}

void OBSBasicTransitionAudio::Init()
{
	SourceUpdated();
	show();
}

void OBSBasicTransitionAudio::OBSSourceUpdated(void *param, calldata_t *calldata)
{
	QMetaObject::invokeMethod(reinterpret_cast<OBSBasicTransitionAudio *>(param), "SourceUpdated");
	UNUSED_PARAMETER(calldata);
}

static inline void setCheckboxState(QCheckBox *checkbox, bool checked)
{
	checkbox->blockSignals(true);
	checkbox->setChecked(checked);
	checkbox->blockSignals(false);
}

static inline bool checkSettingExists(obs_data_t *settings, const char *setting_name)
{
	return (obs_data_has_user_value(settings, setting_name) || obs_data_has_default_value(settings, setting_name));
}

void OBSBasicTransitionAudio::SourceUpdated()
{
	OBSDataAutoRelease settings = obs_source_get_settings(source);

	bool forceMonoVal = obs_data_get_bool(settings, "audio_force_mono");
	setCheckboxState(ui->forceMono, forceMonoVal);

	float volumeVal = checkSettingExists(settings, "audio_volume")
				  ? (float)obs_data_get_double(settings, "audio_volume")
				  : 1.0f;
	ui->volumeBox->blockSignals(true);
	ui->percentBox->blockSignals(true);
	ui->volumeBox->setValue(obs_mul_to_db(volumeVal));
	ui->percentBox->setValue((int)std::round(volumeVal * 100.0f));
	if (ui->volumeBox->value() < -96.0) {
		ui->volumeBox->setSpecialValueText("-inf dB");
		ui->volumeBox->setAccessibleDescription("-inf dB");
	}
	ui->percentBox->setValue((int)std::round(volumeVal * 100.0f));
	ui->percentBox->blockSignals(false);
	ui->volumeBox->blockSignals(false);

	int balanceVal = checkSettingExists(settings, "audio_balance")
				 ? (int)(obs_data_get_double(settings, "audio_balance") * 100.0l)
				 : 50;
	ui->balance->blockSignals(true);
	ui->balance->setValue(balanceVal);
	ui->balance->blockSignals(false);

	int64_t offsetVal = (int64_t)obs_data_get_int(settings, "audio_offset");
	ui->syncOffset->blockSignals(true);
	ui->syncOffset->setValue(offsetVal / NSEC_PER_MSEC);
	ui->syncOffset->blockSignals(false);

	int monitoringType = checkSettingExists(settings, "audio_monitoring")
				     ? (int)obs_data_get_int(settings, "audio_monitoring")
				     : (int)OBS_MONITORING_TYPE_NONE;
	int idx = ui->monitoringType->findData(monitoringType);
	ui->monitoringType->blockSignals(true);
	ui->monitoringType->setCurrentIndex(idx);
	ui->monitoringType->blockSignals(false);

	uint32_t mixers = checkSettingExists(settings, "audio_mixers")
				  ? (uint32_t)obs_data_get_int(settings, "audio_mixers")
				  : 0xFF;
	setCheckboxState(ui->mixer1, mixers & (1 << 0));
	setCheckboxState(ui->mixer2, mixers & (1 << 1));
	setCheckboxState(ui->mixer3, mixers & (1 << 2));
	setCheckboxState(ui->mixer4, mixers & (1 << 3));
	setCheckboxState(ui->mixer5, mixers & (1 << 4));
	setCheckboxState(ui->mixer6, mixers & (1 << 5));
}

static inline void transitionSignalAudioUpdate(obs_source_t *source)
{
	proc_handler_t *ph = obs_source_get_proc_handler(source);
	calldata_t data = {0};
	if (!proc_handler_call(ph, "transition_audio_update", &data)) {
		OBSDataAutoRelease settings = obs_source_get_settings(source);
		obs_source_update(source, settings);
	}
}

void OBSBasicTransitionAudio::volumeChanged(double db)
{
	OBSDataAutoRelease settings = obs_source_get_settings(source);

	float prev = (float)obs_data_get_double(settings, "audio_volume");

	if (db < -96.0) {
		ui->volumeBox->setSpecialValueText("-inf dB");
		db = -INFINITY;
	}
	float val = obs_db_to_mul(db);

	obs_data_set_double(settings, "audio_volume", (double)val);
	transitionSignalAudioUpdate(source);

	auto undo_redo = [](const std::string &uuid, float val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		OBSDataAutoRelease settings = obs_source_get_settings(source);
		obs_data_set_double(settings, "audio_volume", (double)val);
		obs_source_update(source, settings);
		transitionSignalAudioUpdate(source);
	};

	const char *uuid = obs_source_get_uuid(source);
	const char *name = obs_source_get_name(source);
	OBSBasic::Get()->undo_s.add_action(QTStr("Undo.Volume.Change").arg(name),
					   std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid, true);
}

void OBSBasicTransitionAudio::percentChanged(int percent)
{
	OBSDataAutoRelease settings = obs_source_get_settings(source);

	float prev = (float)obs_data_get_double(settings, "audio_volume");
	float val = (float)percent / 100.0f;

	obs_data_set_double(settings, "audio_volume", (double)val);
	transitionSignalAudioUpdate(source);

	auto undo_redo = [](const std::string &uuid, float val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		OBSDataAutoRelease settings = obs_source_get_settings(source);
		obs_data_set_double(settings, "audio_volume", (double)val);
		obs_source_update(source, settings);
		transitionSignalAudioUpdate(source);
	};
	const char *uuid = obs_source_get_uuid(source);
	const char *name = obs_source_get_name(source);
	OBSBasic::Get()->undo_s.add_action(QTStr("Undo.Volume.Change").arg(name),
					   std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid, true);
}

void OBSBasicTransitionAudio::downmixMonoChanged(bool val)
{
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	bool prev = obs_data_get_bool(settings, "audio_force_mono");

	obs_data_set_bool(settings, "audio_force_mono", val);
	transitionSignalAudioUpdate(source);

	auto undo_redo = [](const std::string &uuid, bool val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		OBSDataAutoRelease settings = obs_source_get_settings(source);
		obs_data_set_bool(settings, "audio_force_mono", val);
		obs_source_update(source, settings);
		transitionSignalAudioUpdate(source);
	};

	QString text = QTStr(val ? "Undo.ForceMono.On" : "Undo.ForceMono.Off");

	const char *uuid = obs_source_get_uuid(source);
	const char *name = obs_source_get_name(source);
	OBSBasic::Get()->undo_s.add_action(text.arg(name), std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid);
}

void OBSBasicTransitionAudio::balanceChanged(int bal)
{
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	double prev = obs_data_get_double(settings, "audio_balance");
	if (abs(50 - bal) < 10) {
		ui->balance->blockSignals(true);
		ui->balance->setValue(50);
		bal = 50;
		ui->balance->blockSignals(false);
	}

	double val = (double)bal / 100.0l;
	if (prev == val)
		return;

	obs_data_set_double(settings, "audio_balance", val);
	transitionSignalAudioUpdate(source);

	auto undo_redo = [](const std::string &uuid, int val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		OBSDataAutoRelease settings = obs_source_get_settings(source);
		obs_data_set_double(settings, "audio_balance", val);
		obs_source_update(source, settings);
		transitionSignalAudioUpdate(source);
	};

	const char *uuid = obs_source_get_uuid(source);
	const char *name = obs_source_get_name(source);
	OBSBasic::Get()->undo_s.add_action(QTStr("Undo.Balance.Change").arg(name),
					   std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid, true);
}

void OBSBasicTransitionAudio::ResetBalance()
{
	ui->balance->setValue(50);
}

void OBSBasicTransitionAudio::syncOffsetChanged(int milliseconds)
{
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	int64_t prev = (int64_t)obs_data_get_int(settings, "audio_offset");
	int64_t val = int64_t(milliseconds) * NSEC_PER_MSEC;

	if (prev / NSEC_PER_MSEC == milliseconds)
		return;

	obs_data_set_int(settings, "audio_offset", (long long)val);
	transitionSignalAudioUpdate(source);

	auto undo_redo = [](const std::string &uuid, int64_t val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		OBSDataAutoRelease settings = obs_source_get_settings(source);
		obs_data_set_int(settings, "audio_offset", (long long)val);
		obs_source_update(source, settings);
		transitionSignalAudioUpdate(source);
	};

	const char *uuid = obs_source_get_uuid(source);
	const char *name = obs_source_get_name(source);
	OBSBasic::Get()->undo_s.add_action(QTStr("Undo.SyncOffset.Change").arg(name),
					   std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid, true);
}

void OBSBasicTransitionAudio::monitoringTypeChanged(int index)
{
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	int prev = (int)obs_data_get_int(settings, "audio_monitoring");
	int val = ui->monitoringType->itemData(index).toInt();

	obs_data_set_int(settings, "audio_monitoring", (long long)val);
	transitionSignalAudioUpdate(source);

	const char *type = nullptr;

	switch (val) {
	case (int)OBS_MONITORING_TYPE_NONE:
		type = "none";
		break;
	case (int)OBS_MONITORING_TYPE_MONITOR_ONLY:
		type = "monitor only";
		break;
	case (int)OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT:
		type = "monitor and output";
		break;
	}

	const char *name = obs_source_get_name(source);
	blog(LOG_INFO, "User changed audio monitoring for transition '%s' to: %s", name ? name : "(null)", type);

	auto undo_redo = [](const std::string &uuid, int val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		OBSDataAutoRelease settings = obs_source_get_settings(source);
		obs_data_set_int(settings, "audio_monitoring", (long long)val);
		obs_source_update(source, settings);
		transitionSignalAudioUpdate(source);
	};

	const char *uuid = obs_source_get_uuid(source);
	OBSBasic::Get()->undo_s.add_action(QTStr("Undo.MonitoringType.Change").arg(name),
					   std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid);
}

static inline void setMixer(obs_source_t *source, const int mixerIdx, const bool checked)
{
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	uint32_t prev = checkSettingExists(settings, "audio_mixers")
				? (uint32_t)obs_data_get_int(settings, "audio_mixers")
				: 0xFF;
	uint32_t val = prev;

	if (checked)
		val |= (1 << mixerIdx);
	else
		val &= ~(1 << mixerIdx);

	obs_data_set_int(settings, "audio_mixers", (long long)val);
	transitionSignalAudioUpdate(source);

	auto undo_redo = [](const std::string &uuid, uint32_t val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		OBSDataAutoRelease settings = obs_source_get_settings(source);
		obs_data_set_int(settings, "audio_mixers", (long long)val);
		obs_source_update(source, settings);
		transitionSignalAudioUpdate(source);
	};

	const char *uuid = obs_source_get_uuid(source);
	const char *name = obs_source_get_name(source);
	OBSBasic::Get()->undo_s.add_action(QTStr("Undo.Mixers.Change").arg(name),
					   std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid);
}

void OBSBasicTransitionAudio::mixer1Changed(bool checked)
{
	setMixer(source, 0, checked);
}

void OBSBasicTransitionAudio::mixer2Changed(bool checked)
{
	setMixer(source, 1, checked);
}

void OBSBasicTransitionAudio::mixer3Changed(bool checked)
{
	setMixer(source, 2, checked);
}

void OBSBasicTransitionAudio::mixer4Changed(bool checked)
{
	setMixer(source, 3, checked);
}

void OBSBasicTransitionAudio::mixer5Changed(bool checked)
{
	setMixer(source, 4, checked);
}

void OBSBasicTransitionAudio::mixer6Changed(bool checked)
{
	setMixer(source, 5, checked);
}

void OBSBasicTransitionAudio::SetVolumeType(VolumeType volType)
{
	switch (volType) {
	case VolumeType::Percent:
		ui->stackedWidget->setCurrentWidget(ui->percent);
		break;
	case VolumeType::dB:
		ui->stackedWidget->setCurrentWidget(ui->volume);
		break;
	}
}

void OBSBasicTransitionAudio::on_usePercent_toggled(bool checked)
{
	VolumeType volType;

	if (checked)
		volType = VolumeType::Percent;
	else
		volType = VolumeType::dB;

	SetVolumeType(volType);

	config_set_int(App()->GetUserConfig(), "BasicWindow", "AdvAudioVolumeType", (int)volType);
}
