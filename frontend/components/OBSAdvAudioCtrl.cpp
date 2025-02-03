#include "OBSAdvAudioCtrl.hpp"

#include <components/BalanceSlider.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QCheckBox>

#include "moc_OBSAdvAudioCtrl.cpp"

#ifndef NSEC_PER_MSEC
#define NSEC_PER_MSEC 1000000
#endif

#define MIN_DB -96.0
#define MAX_DB 26.0

static inline void setMixer(obs_source_t *source, const int mixerIdx, const bool checked);

OBSAdvAudioCtrl::OBSAdvAudioCtrl(QGridLayout *, obs_source_t *source_) : source(source_)
{
	QHBoxLayout *hlayout;
	signal_handler_t *handler = obs_source_get_signal_handler(source);
	QString sourceName = QT_UTF8(obs_source_get_name(source));
	float vol = obs_source_get_volume(source);
	uint32_t flags = obs_source_get_flags(source);
	uint32_t mixers = obs_source_get_audio_mixers(source);

	mixerContainer = new QWidget();
	balanceContainer = new QWidget();
	labelL = new QLabel();
	labelR = new QLabel();
	iconLabel = new QLabel();
	nameLabel = new QLabel();
	active = new QLabel();
	stackedWidget = new QStackedWidget();
	volume = new QDoubleSpinBox();
	percent = new QSpinBox();
	forceMono = new QCheckBox();
	balance = new BalanceSlider();
	if (obs_audio_monitoring_available())
		monitoringType = new QComboBox();
	syncOffset = new QSpinBox();
	mixer1 = new QCheckBox();
	mixer2 = new QCheckBox();
	mixer3 = new QCheckBox();
	mixer4 = new QCheckBox();
	mixer5 = new QCheckBox();
	mixer6 = new QCheckBox();

	sigs.emplace_back(handler, "activate", OBSSourceActivated, this);
	sigs.emplace_back(handler, "deactivate", OBSSourceDeactivated, this);
	sigs.emplace_back(handler, "audio_activate", OBSSourceActivated, this);
	sigs.emplace_back(handler, "audio_deactivate", OBSSourceDeactivated, this);
	sigs.emplace_back(handler, "volume", OBSSourceVolumeChanged, this);
	sigs.emplace_back(handler, "audio_sync", OBSSourceSyncChanged, this);
	sigs.emplace_back(handler, "update_flags", OBSSourceFlagsChanged, this);
	if (obs_audio_monitoring_available())
		sigs.emplace_back(handler, "audio_monitoring", OBSSourceMonitoringTypeChanged, this);
	sigs.emplace_back(handler, "audio_mixers", OBSSourceMixersChanged, this);
	sigs.emplace_back(handler, "audio_balance", OBSSourceBalanceChanged, this);
	sigs.emplace_back(handler, "rename", OBSSourceRenamed, this);

	hlayout = new QHBoxLayout();
	hlayout->setContentsMargins(0, 0, 0, 0);
	mixerContainer->setLayout(hlayout);
	hlayout = new QHBoxLayout();
	hlayout->setContentsMargins(0, 0, 0, 0);
	balanceContainer->setLayout(hlayout);
	balanceContainer->setFixedWidth(150);

	labelL->setText("L");
	labelR->setText("R");

	OBSBasic *main = OBSBasic::Get();

	QIcon sourceIcon = main->GetSourceIcon(obs_source_get_id(source));
	QPixmap pixmap = sourceIcon.pixmap(QSize(16, 16));
	iconLabel->setPixmap(pixmap);
	iconLabel->setFixedSize(16, 16);
	iconLabel->setStyleSheet("background: none");

	SetSourceName(sourceName);
	nameLabel->setAlignment(Qt::AlignVCenter);

	bool isActive = obs_source_active(source) && obs_source_audio_active(source);
	active->setText(isActive ? QTStr("Basic.Stats.Status.Active") : QTStr("Basic.Stats.Status.Inactive"));
	if (isActive)
		setClasses(active, "text-danger");
	active->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

	volume->setMinimum(MIN_DB - 0.1);
	volume->setMaximum(MAX_DB);
	volume->setSingleStep(0.1);
	volume->setDecimals(1);
	volume->setSuffix(" dB");
	volume->setValue(obs_mul_to_db(vol));
	volume->setAccessibleName(QTStr("Basic.AdvAudio.VolumeSource").arg(sourceName));

	if (volume->value() < MIN_DB) {
		volume->setSpecialValueText("-inf dB");
		volume->setAccessibleDescription("-inf dB");
	}

	percent->setMinimum(0);
	percent->setMaximum(2000);
	percent->setSuffix("%");
	percent->setValue((int)(obs_source_get_volume(source) * 100.0f));
	percent->setAccessibleName(QTStr("Basic.AdvAudio.VolumeSource").arg(sourceName));

	stackedWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	stackedWidget->setFixedWidth(100);
	stackedWidget->addWidget(volume);
	stackedWidget->addWidget(percent);

	VolumeType volType = (VolumeType)config_get_int(App()->GetUserConfig(), "BasicWindow", "AdvAudioVolumeType");

	SetVolumeWidget(volType);

	forceMono->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
	forceMono->setChecked((flags & OBS_SOURCE_FLAG_FORCE_MONO) != 0);
	forceMono->setAccessibleName(QTStr("Basic.AdvAudio.MonoSource").arg(sourceName));

	balance->setOrientation(Qt::Horizontal);
	balance->setMinimum(0);
	balance->setMaximum(100);
	balance->setTickPosition(QSlider::TicksAbove);
	balance->setTickInterval(50);
	balance->setAccessibleName(QTStr("Basic.AdvAudio.BalanceSource").arg(sourceName));

	const char *speakers = config_get_string(main->Config(), "Audio", "ChannelSetup");

	if (strcmp(speakers, "Mono") == 0)
		balance->setEnabled(false);
	else
		balance->setEnabled(true);

	float bal = obs_source_get_balance_value(source) * 100.0f;
	balance->setValue((int)bal);

	int64_t cur_sync = obs_source_get_sync_offset(source);
	syncOffset->setMinimum(-950);
	syncOffset->setMaximum(20000);
	syncOffset->setSuffix(" ms");
	syncOffset->setValue(int(cur_sync / NSEC_PER_MSEC));
	syncOffset->setFixedWidth(100);
	syncOffset->setAccessibleName(QTStr("Basic.AdvAudio.SyncOffsetSource").arg(sourceName));

	int idx;
	if (obs_audio_monitoring_available()) {
		monitoringType->addItem(QTStr("Basic.AdvAudio.Monitoring.None"), (int)OBS_MONITORING_TYPE_NONE);
		monitoringType->addItem(QTStr("Basic.AdvAudio.Monitoring.MonitorOnly"),
					(int)OBS_MONITORING_TYPE_MONITOR_ONLY);
		monitoringType->addItem(QTStr("Basic.AdvAudio.Monitoring.Both"),
					(int)OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT);
		int mt = (int)obs_source_get_monitoring_type(source);
		idx = monitoringType->findData(mt);
		monitoringType->setCurrentIndex(idx);
		monitoringType->setAccessibleName(QTStr("Basic.AdvAudio.MonitoringSource").arg(sourceName));
		monitoringType->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
	}

	mixer1->setText("1");
	mixer1->setChecked(mixers & (1 << 0));
	mixer1->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track1"));
	mixer2->setText("2");
	mixer2->setChecked(mixers & (1 << 1));
	mixer2->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track2"));
	mixer3->setText("3");
	mixer3->setChecked(mixers & (1 << 2));
	mixer3->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track3"));
	mixer4->setText("4");
	mixer4->setChecked(mixers & (1 << 3));
	mixer4->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track4"));
	mixer5->setText("5");
	mixer5->setChecked(mixers & (1 << 4));
	mixer5->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track5"));
	mixer6->setText("6");
	mixer6->setChecked(mixers & (1 << 5));
	mixer6->setAccessibleName(QTStr("Basic.Settings.Output.Adv.Audio.Track6"));

	balanceContainer->layout()->addWidget(labelL);
	balanceContainer->layout()->addWidget(balance);
	balanceContainer->layout()->addWidget(labelR);

	speaker_layout sl = obs_source_get_speaker_layout(source);

	if (sl != SPEAKERS_STEREO)
		balanceContainer->setEnabled(false);

	mixerContainer->layout()->addWidget(mixer1);
	mixerContainer->layout()->addWidget(mixer2);
	mixerContainer->layout()->addWidget(mixer3);
	mixerContainer->layout()->addWidget(mixer4);
	mixerContainer->layout()->addWidget(mixer5);
	mixerContainer->layout()->addWidget(mixer6);
	mixerContainer->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

	connect(volume, &QDoubleSpinBox::valueChanged, this, &OBSAdvAudioCtrl::volumeChanged);
	connect(percent, &QSpinBox::valueChanged, this, &OBSAdvAudioCtrl::percentChanged);
	connect(forceMono, &QCheckBox::clicked, this, &OBSAdvAudioCtrl::downmixMonoChanged);
	connect(balance, &BalanceSlider::valueChanged, this, &OBSAdvAudioCtrl::balanceChanged);
	connect(balance, &BalanceSlider::doubleClicked, this, &OBSAdvAudioCtrl::ResetBalance);
	connect(syncOffset, &QSpinBox::valueChanged, this, &OBSAdvAudioCtrl::syncOffsetChanged);
	if (obs_audio_monitoring_available())
		connect(monitoringType, &QComboBox::currentIndexChanged, this, &OBSAdvAudioCtrl::monitoringTypeChanged);

	auto connectMixer = [this](QCheckBox *mixer, int num) {
		connect(mixer, &QCheckBox::clicked, [this, num](bool checked) { setMixer(source, num, checked); });
	};
	connectMixer(mixer1, 0);
	connectMixer(mixer2, 1);
	connectMixer(mixer3, 2);
	connectMixer(mixer4, 3);
	connectMixer(mixer5, 4);
	connectMixer(mixer6, 5);

	setObjectName(sourceName);
}

OBSAdvAudioCtrl::~OBSAdvAudioCtrl()
{
	iconLabel->deleteLater();
	nameLabel->deleteLater();
	active->deleteLater();
	stackedWidget->deleteLater();
	forceMono->deleteLater();
	balanceContainer->deleteLater();
	syncOffset->deleteLater();
	if (obs_audio_monitoring_available())
		monitoringType->deleteLater();
	mixerContainer->deleteLater();
}

void OBSAdvAudioCtrl::ShowAudioControl(QGridLayout *layout)
{
	int lastRow = layout->rowCount();
	int idx = 0;

	layout->addWidget(iconLabel, lastRow, idx++);
	layout->addWidget(nameLabel, lastRow, idx++);
	layout->addWidget(active, lastRow, idx++);
	layout->addWidget(stackedWidget, lastRow, idx++);
	layout->addWidget(forceMono, lastRow, idx++);
	layout->addWidget(balanceContainer, lastRow, idx++);
	layout->addWidget(syncOffset, lastRow, idx++);
	if (obs_audio_monitoring_available())
		layout->addWidget(monitoringType, lastRow, idx++);
	layout->addWidget(mixerContainer, lastRow, idx++);
	layout->layout()->setAlignment(mixerContainer, Qt::AlignVCenter);
	layout->setHorizontalSpacing(15);
}

/* ------------------------------------------------------------------------- */
/* OBS source callbacks */

void OBSAdvAudioCtrl::OBSSourceActivated(void *param, calldata_t *)
{
	QMetaObject::invokeMethod(reinterpret_cast<OBSAdvAudioCtrl *>(param), "SourceActiveChanged", Q_ARG(bool, true));
}

void OBSAdvAudioCtrl::OBSSourceDeactivated(void *param, calldata_t *)
{
	QMetaObject::invokeMethod(reinterpret_cast<OBSAdvAudioCtrl *>(param), "SourceActiveChanged",
				  Q_ARG(bool, false));
}

void OBSAdvAudioCtrl::OBSSourceFlagsChanged(void *param, calldata_t *calldata)
{
	uint32_t flags = (uint32_t)calldata_int(calldata, "flags");
	QMetaObject::invokeMethod(reinterpret_cast<OBSAdvAudioCtrl *>(param), "SourceFlagsChanged",
				  Q_ARG(uint32_t, flags));
}

void OBSAdvAudioCtrl::OBSSourceVolumeChanged(void *param, calldata_t *calldata)
{
	float volume = (float)calldata_float(calldata, "volume");
	QMetaObject::invokeMethod(reinterpret_cast<OBSAdvAudioCtrl *>(param), "SourceVolumeChanged",
				  Q_ARG(float, volume));
}

void OBSAdvAudioCtrl::OBSSourceSyncChanged(void *param, calldata_t *calldata)
{
	int64_t offset = calldata_int(calldata, "offset");
	QMetaObject::invokeMethod(reinterpret_cast<OBSAdvAudioCtrl *>(param), "SourceSyncChanged",
				  Q_ARG(int64_t, offset));
}

void OBSAdvAudioCtrl::OBSSourceMonitoringTypeChanged(void *param, calldata_t *calldata)
{
	int type = calldata_int(calldata, "type");
	QMetaObject::invokeMethod(reinterpret_cast<OBSAdvAudioCtrl *>(param), "SourceMonitoringTypeChanged",
				  Q_ARG(int, type));
}

void OBSAdvAudioCtrl::OBSSourceMixersChanged(void *param, calldata_t *calldata)
{
	uint32_t mixers = (uint32_t)calldata_int(calldata, "mixers");
	QMetaObject::invokeMethod(reinterpret_cast<OBSAdvAudioCtrl *>(param), "SourceMixersChanged",
				  Q_ARG(uint32_t, mixers));
}

void OBSAdvAudioCtrl::OBSSourceBalanceChanged(void *param, calldata_t *calldata)
{
	int balance = (float)calldata_float(calldata, "balance") * 100.0f;
	QMetaObject::invokeMethod(reinterpret_cast<OBSAdvAudioCtrl *>(param), "SourceBalanceChanged",
				  Q_ARG(int, balance));
}

void OBSAdvAudioCtrl::OBSSourceRenamed(void *param, calldata_t *calldata)
{
	QString newName = QT_UTF8(calldata_string(calldata, "new_name"));

	QMetaObject::invokeMethod(reinterpret_cast<OBSAdvAudioCtrl *>(param), "SetSourceName", Q_ARG(QString, newName));
}

/* ------------------------------------------------------------------------- */
/* Qt event queue source callbacks */

static inline void setCheckboxState(QCheckBox *checkbox, bool checked)
{
	checkbox->blockSignals(true);
	checkbox->setChecked(checked);
	checkbox->blockSignals(false);
}

void OBSAdvAudioCtrl::SourceActiveChanged(bool isActive)
{
	if (isActive && obs_source_audio_active(source)) {
		active->setText(QTStr("Basic.Stats.Status.Active"));
		setClasses(active, "text-danger");
	} else {
		active->setText(QTStr("Basic.Stats.Status.Inactive"));
		setClasses(active, "");
	}
}

void OBSAdvAudioCtrl::SourceFlagsChanged(uint32_t flags)
{
	bool forceMonoVal = (flags & OBS_SOURCE_FLAG_FORCE_MONO) != 0;
	setCheckboxState(forceMono, forceMonoVal);
}

void OBSAdvAudioCtrl::SourceVolumeChanged(float value)
{
	volume->blockSignals(true);
	percent->blockSignals(true);
	volume->setValue(obs_mul_to_db(value));
	percent->setValue((int)std::round(value * 100.0f));
	percent->blockSignals(false);
	volume->blockSignals(false);
}

void OBSAdvAudioCtrl::SourceBalanceChanged(int value)
{
	balance->blockSignals(true);
	balance->setValue(value);
	balance->blockSignals(false);
}

void OBSAdvAudioCtrl::SourceSyncChanged(int64_t offset)
{
	syncOffset->blockSignals(true);
	syncOffset->setValue(offset / NSEC_PER_MSEC);
	syncOffset->blockSignals(false);
}

void OBSAdvAudioCtrl::SourceMonitoringTypeChanged(int type)
{
	int idx = monitoringType->findData(type);
	monitoringType->blockSignals(true);
	monitoringType->setCurrentIndex(idx);
	monitoringType->blockSignals(false);
}

void OBSAdvAudioCtrl::SourceMixersChanged(uint32_t mixers)
{
	setCheckboxState(mixer1, mixers & (1 << 0));
	setCheckboxState(mixer2, mixers & (1 << 1));
	setCheckboxState(mixer3, mixers & (1 << 2));
	setCheckboxState(mixer4, mixers & (1 << 3));
	setCheckboxState(mixer5, mixers & (1 << 4));
	setCheckboxState(mixer6, mixers & (1 << 5));
}

/* ------------------------------------------------------------------------- */
/* Qt control callbacks */

void OBSAdvAudioCtrl::volumeChanged(double db)
{
	float prev = obs_source_get_volume(source);

	if (db < MIN_DB) {
		volume->setSpecialValueText("-inf dB");
		db = -INFINITY;
	}

	float val = obs_db_to_mul(db);
	obs_source_set_volume(source, val);

	auto undo_redo = [](const std::string &uuid, float val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		obs_source_set_volume(source, val);
	};

	const char *name = obs_source_get_name(source);
	const char *uuid = obs_source_get_uuid(source);
	OBSBasic *main = OBSBasic::Get();
	main->undo_s.add_action(QTStr("Undo.Volume.Change").arg(name),
				std::bind(undo_redo, std::placeholders::_1, prev),
				std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid, true);
}

void OBSAdvAudioCtrl::percentChanged(int percent)
{
	float prev = obs_source_get_volume(source);
	float val = (float)percent / 100.0f;

	obs_source_set_volume(source, val);

	auto undo_redo = [](const std::string &uuid, float val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		obs_source_set_volume(source, val);
	};

	const char *name = obs_source_get_name(source);
	const char *uuid = obs_source_get_uuid(source);
	OBSBasic::Get()->undo_s.add_action(QTStr("Undo.Volume.Change").arg(name),
					   std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid, true);
}

static inline void set_mono(obs_source_t *source, bool mono)
{
	uint32_t flags = obs_source_get_flags(source);
	if (mono)
		flags |= OBS_SOURCE_FLAG_FORCE_MONO;
	else
		flags &= ~OBS_SOURCE_FLAG_FORCE_MONO;
	obs_source_set_flags(source, flags);
}

void OBSAdvAudioCtrl::downmixMonoChanged(bool val)
{
	uint32_t flags = obs_source_get_flags(source);
	bool forceMonoActive = (flags & OBS_SOURCE_FLAG_FORCE_MONO) != 0;

	if (forceMonoActive == val)
		return;

	if (val)
		flags |= OBS_SOURCE_FLAG_FORCE_MONO;
	else
		flags &= ~OBS_SOURCE_FLAG_FORCE_MONO;

	obs_source_set_flags(source, flags);

	auto undo_redo = [](const std::string &uuid, bool val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		set_mono(source, val);
	};

	QString text = QTStr(val ? "Undo.ForceMono.On" : "Undo.ForceMono.Off");

	const char *name = obs_source_get_name(source);
	const char *uuid = obs_source_get_uuid(source);
	OBSBasic::Get()->undo_s.add_action(text.arg(name), std::bind(undo_redo, std::placeholders::_1, !val),
					   std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid);
}

void OBSAdvAudioCtrl::balanceChanged(int val)
{
	float prev = obs_source_get_balance_value(source);
	float bal = (float)val / 100.0f;

	if (abs(50 - val) < 10) {
		balance->blockSignals(true);
		balance->setValue(50);
		bal = 0.5f;
		balance->blockSignals(false);
	}

	obs_source_set_balance_value(source, bal);

	auto undo_redo = [](const std::string &uuid, float val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		obs_source_set_balance_value(source, val);
	};

	const char *name = obs_source_get_name(source);
	const char *uuid = obs_source_get_uuid(source);
	OBSBasic::Get()->undo_s.add_action(QTStr("Undo.Balance.Change").arg(name),
					   std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, bal), uuid, uuid, true);
}

void OBSAdvAudioCtrl::ResetBalance()
{
	balance->setValue(50);
}

void OBSAdvAudioCtrl::syncOffsetChanged(int milliseconds)
{
	int64_t prev = obs_source_get_sync_offset(source);
	int64_t val = int64_t(milliseconds) * NSEC_PER_MSEC;

	if (prev / NSEC_PER_MSEC == milliseconds)
		return;

	obs_source_set_sync_offset(source, val);

	auto undo_redo = [](const std::string &uuid, int64_t val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		obs_source_set_sync_offset(source, val);
	};

	const char *name = obs_source_get_name(source);
	const char *uuid = obs_source_get_uuid(source);
	OBSBasic::Get()->undo_s.add_action(QTStr("Undo.SyncOffset.Change").arg(name),
					   std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid, true);
}

void OBSAdvAudioCtrl::monitoringTypeChanged(int index)
{
	obs_monitoring_type prev = obs_source_get_monitoring_type(source);

	obs_monitoring_type mt = (obs_monitoring_type)monitoringType->itemData(index).toInt();
	obs_source_set_monitoring_type(source, mt);

	const char *type = nullptr;

	switch (mt) {
	case OBS_MONITORING_TYPE_NONE:
		type = "none";
		break;
	case OBS_MONITORING_TYPE_MONITOR_ONLY:
		type = "monitor only";
		break;
	case OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT:
		type = "monitor and output";
		break;
	}

	const char *name = obs_source_get_name(source);
	blog(LOG_INFO, "User changed audio monitoring for source '%s' to: %s", name ? name : "(null)", type);

	auto undo_redo = [](const std::string &uuid, obs_monitoring_type val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		obs_source_set_monitoring_type(source, val);
	};

	const char *uuid = obs_source_get_uuid(source);
	OBSBasic::Get()->undo_s.add_action(QTStr("Undo.MonitoringType.Change").arg(name),
					   std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, mt), uuid, uuid);
}

static inline void setMixer(obs_source_t *source, const int mixerIdx, const bool checked)
{
	uint32_t mixers = obs_source_get_audio_mixers(source);
	uint32_t new_mixers = mixers;

	if (checked)
		new_mixers |= (1 << mixerIdx);
	else
		new_mixers &= ~(1 << mixerIdx);

	obs_source_set_audio_mixers(source, new_mixers);

	auto undo_redo = [](const std::string &uuid, uint32_t mixers) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		obs_source_set_audio_mixers(source, mixers);
	};

	const char *name = obs_source_get_name(source);
	const char *uuid = obs_source_get_uuid(source);
	OBSBasic::Get()->undo_s.add_action(QTStr("Undo.Mixers.Change").arg(name),
					   std::bind(undo_redo, std::placeholders::_1, mixers),
					   std::bind(undo_redo, std::placeholders::_1, new_mixers), uuid, uuid);
}

void OBSAdvAudioCtrl::SetVolumeWidget(VolumeType type)
{
	switch (type) {
	case VolumeType::Percent:
		stackedWidget->setCurrentWidget(percent);
		break;
	case VolumeType::dB:
		stackedWidget->setCurrentWidget(volume);
		break;
	}
}

void OBSAdvAudioCtrl::SetIconVisible(bool visible)
{
	visible ? iconLabel->show() : iconLabel->hide();
}

void OBSAdvAudioCtrl::SetSourceName(QString newName)
{
	TruncateLabel(nameLabel, newName);
}
