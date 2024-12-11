#include "VolControl.hpp"
#include "VolumeMeter.hpp"
#include "OBSBasic.hpp"

#include <components/MuteCheckBox.hpp>
#include <components/OBSSourceLabel.hpp>
#include <components/VolumeSlider.hpp>

#include <QMessageBox>

#include "moc_VolControl.cpp"

static inline Qt::CheckState GetCheckState(bool muted, bool unassigned)
{
	if (muted)
		return Qt::Checked;
	else if (unassigned)
		return Qt::PartiallyChecked;
	else
		return Qt::Unchecked;
}

static inline bool IsSourceUnassigned(obs_source_t *source)
{
	uint32_t mixes = (obs_source_get_audio_mixers(source) & ((1 << MAX_AUDIO_MIXES) - 1));
	obs_monitoring_type mt = obs_source_get_monitoring_type(source);

	return mixes == 0 && mt != OBS_MONITORING_TYPE_MONITOR_ONLY;
}

static void ShowUnassignedWarning(const char *name)
{
	auto msgBox = [=]() {
		QMessageBox msgbox(App()->GetMainWindow());
		msgbox.setWindowTitle(QTStr("VolControl.UnassignedWarning.Title"));
		msgbox.setText(QTStr("VolControl.UnassignedWarning.Text").arg(name));
		msgbox.setIcon(QMessageBox::Icon::Information);
		msgbox.addButton(QMessageBox::Ok);

		QCheckBox *cb = new QCheckBox(QTStr("DoNotShowAgain"));
		msgbox.setCheckBox(cb);

		msgbox.exec();

		if (cb->isChecked()) {
			config_set_bool(App()->GetUserConfig(), "General", "WarnedAboutUnassignedSources", true);
			config_save_safe(App()->GetUserConfig(), "tmp", nullptr);
		}
	};

	QMetaObject::invokeMethod(App(), "Exec", Qt::QueuedConnection, Q_ARG(VoidFunc, msgBox));
}

void VolControl::OBSVolumeChanged(void *data, float db)
{
	Q_UNUSED(db);
	VolControl *volControl = static_cast<VolControl *>(data);

	QMetaObject::invokeMethod(volControl, "VolumeChanged");
}

void VolControl::OBSVolumeLevel(void *data, const float magnitude[MAX_AUDIO_CHANNELS],
				const float peak[MAX_AUDIO_CHANNELS], const float inputPeak[MAX_AUDIO_CHANNELS])
{
	VolControl *volControl = static_cast<VolControl *>(data);

	volControl->volMeter->setLevels(magnitude, peak, inputPeak);
}

void VolControl::OBSVolumeMuted(void *data, calldata_t *calldata)
{
	VolControl *volControl = static_cast<VolControl *>(data);
	bool muted = calldata_bool(calldata, "muted");

	QMetaObject::invokeMethod(volControl, "VolumeMuted", Q_ARG(bool, muted));
}

void VolControl::VolumeChanged()
{
	slider->blockSignals(true);
	slider->setValue((int)(obs_fader_get_deflection(obs_fader) * FADER_PRECISION));
	slider->blockSignals(false);

	updateText();
}

void VolControl::VolumeMuted(bool muted)
{
	bool unassigned = IsSourceUnassigned(source);

	auto newState = GetCheckState(muted, unassigned);
	if (mute->checkState() != newState)
		mute->setCheckState(newState);

	volMeter->muted = muted || unassigned;
}

void VolControl::OBSMixersOrMonitoringChanged(void *data, calldata_t *)
{

	VolControl *volControl = static_cast<VolControl *>(data);
	QMetaObject::invokeMethod(volControl, "MixersOrMonitoringChanged", Qt::QueuedConnection);
}

void VolControl::MixersOrMonitoringChanged()
{
	bool muted = obs_source_muted(source);
	bool unassigned = IsSourceUnassigned(source);

	auto newState = GetCheckState(muted, unassigned);
	if (mute->checkState() != newState)
		mute->setCheckState(newState);

	volMeter->muted = muted || unassigned;
}

void VolControl::SetMuted(bool)
{
	bool checked = mute->checkState() == Qt::Checked;
	bool prev = obs_source_muted(source);
	obs_source_set_muted(source, checked);
	bool unassigned = IsSourceUnassigned(source);

	if (!checked && unassigned) {
		mute->setCheckState(Qt::PartiallyChecked);
		/* Show notice about the source no being assigned to any tracks */
		bool has_shown_warning =
			config_get_bool(App()->GetUserConfig(), "General", "WarnedAboutUnassignedSources");
		if (!has_shown_warning)
			ShowUnassignedWarning(obs_source_get_name(source));
	}

	auto undo_redo = [](const std::string &uuid, bool val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		obs_source_set_muted(source, val);
	};

	QString text = QTStr(checked ? "Undo.Volume.Mute" : "Undo.Volume.Unmute");

	const char *name = obs_source_get_name(source);
	const char *uuid = obs_source_get_uuid(source);
	OBSBasic::Get()->undo_s.add_action(text.arg(name), std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, checked), uuid, uuid);
}

void VolControl::SliderChanged(int vol)
{
	float prev = obs_source_get_volume(source);

	obs_fader_set_deflection(obs_fader, float(vol) / FADER_PRECISION);
	updateText();

	auto undo_redo = [](const std::string &uuid, float val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		obs_source_set_volume(source, val);
	};

	float val = obs_source_get_volume(source);
	const char *name = obs_source_get_name(source);
	const char *uuid = obs_source_get_uuid(source);
	OBSBasic::Get()->undo_s.add_action(QTStr("Undo.Volume.Change").arg(name),
					   std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid, true);
}

void VolControl::updateText()
{
	QString text;
	float db = obs_fader_get_db(obs_fader);

	if (db < -96.0f)
		text = "-inf dB";
	else
		text = QString::number(db, 'f', 1).append(" dB");

	volLabel->setText(text);

	bool muted = obs_source_muted(source);
	const char *accTextLookup = muted ? "VolControl.SliderMuted" : "VolControl.SliderUnmuted";

	QString sourceName = obs_source_get_name(source);
	QString accText = QTStr(accTextLookup).arg(sourceName);

	slider->setAccessibleName(accText);
}

void VolControl::EmitConfigClicked()
{
	emit ConfigClicked();
}

void VolControl::SetMeterDecayRate(qreal q)
{
	volMeter->setPeakDecayRate(q);
}

void VolControl::setPeakMeterType(enum obs_peak_meter_type peakMeterType)
{
	volMeter->setPeakMeterType(peakMeterType);
}

VolControl::VolControl(OBSSource source_, bool showConfig, bool vertical)
	: source(std::move(source_)),
	  levelTotal(0.0f),
	  levelCount(0.0f),
	  obs_fader(obs_fader_create(OBS_FADER_LOG)),
	  obs_volmeter(obs_volmeter_create(OBS_FADER_LOG)),
	  vertical(vertical),
	  contextMenu(nullptr)
{
	nameLabel = new OBSSourceLabel(source);
	volLabel = new QLabel();
	mute = new MuteCheckBox();

	volLabel->setObjectName("volLabel");
	volLabel->setAlignment(Qt::AlignCenter);

#ifdef __APPLE__
	mute->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif

	QString sourceName = obs_source_get_name(source);
	setObjectName(sourceName);

	if (showConfig) {
		config = new QPushButton(this);
		config->setProperty("class", "icon-dots-vert");
		config->setAutoDefault(false);

		config->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

		config->setAccessibleName(QTStr("VolControl.Properties").arg(sourceName));

		connect(config, &QAbstractButton::clicked, this, &VolControl::EmitConfigClicked);
	}

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	if (vertical) {
		QHBoxLayout *nameLayout = new QHBoxLayout;
		QHBoxLayout *controlLayout = new QHBoxLayout;
		QHBoxLayout *volLayout = new QHBoxLayout;
		QFrame *meterFrame = new QFrame;
		QHBoxLayout *meterLayout = new QHBoxLayout;

		volMeter = new VolumeMeter(nullptr, obs_volmeter, true);
		slider = new VolumeSlider(obs_fader, Qt::Vertical);
		slider->setLayoutDirection(Qt::LeftToRight);
		slider->setDisplayTicks(true);

		nameLayout->setAlignment(Qt::AlignCenter);
		meterLayout->setAlignment(Qt::AlignCenter);
		controlLayout->setAlignment(Qt::AlignCenter);
		volLayout->setAlignment(Qt::AlignCenter);

		meterFrame->setObjectName("volMeterFrame");

		nameLayout->setContentsMargins(0, 0, 0, 0);
		nameLayout->setSpacing(0);
		nameLayout->addWidget(nameLabel);

		controlLayout->setContentsMargins(0, 0, 0, 0);
		controlLayout->setSpacing(0);

		// Add Headphone (audio monitoring) widget here
		controlLayout->addWidget(mute);

		if (showConfig) {
			controlLayout->addWidget(config);
		}

		meterLayout->setContentsMargins(0, 0, 0, 0);
		meterLayout->setSpacing(0);
		meterLayout->addWidget(slider);
		meterLayout->addWidget(volMeter);

		meterFrame->setLayout(meterLayout);

		volLayout->setContentsMargins(0, 0, 0, 0);
		volLayout->setSpacing(0);
		volLayout->addWidget(volLabel);
		volLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));

		mainLayout->addItem(nameLayout);
		mainLayout->addItem(volLayout);
		mainLayout->addWidget(meterFrame);
		mainLayout->addItem(controlLayout);

		volMeter->setFocusProxy(slider);

		// Default size can cause clipping of long names in vertical layout.
		QFont font = nameLabel->font();
		QFontInfo info(font);
		nameLabel->setFont(font);

		setMaximumWidth(110);
	} else {
		QHBoxLayout *textLayout = new QHBoxLayout;
		QHBoxLayout *controlLayout = new QHBoxLayout;
		QFrame *meterFrame = new QFrame;
		QVBoxLayout *meterLayout = new QVBoxLayout;
		QVBoxLayout *buttonLayout = new QVBoxLayout;

		volMeter = new VolumeMeter(nullptr, obs_volmeter, false);
		volMeter->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

		slider = new VolumeSlider(obs_fader, Qt::Horizontal);
		slider->setLayoutDirection(Qt::LeftToRight);
		slider->setDisplayTicks(true);

		textLayout->setContentsMargins(0, 0, 0, 0);
		textLayout->addWidget(nameLabel);
		textLayout->addWidget(volLabel);
		textLayout->setAlignment(nameLabel, Qt::AlignLeft);
		textLayout->setAlignment(volLabel, Qt::AlignRight);

		meterFrame->setObjectName("volMeterFrame");
		meterFrame->setLayout(meterLayout);

		meterLayout->setContentsMargins(0, 0, 0, 0);
		meterLayout->setSpacing(0);

		meterLayout->addWidget(volMeter);
		meterLayout->addWidget(slider);

		buttonLayout->setContentsMargins(0, 0, 0, 0);
		buttonLayout->setSpacing(0);

		if (showConfig) {
			buttonLayout->addWidget(config);
		}
		buttonLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding));
		buttonLayout->addWidget(mute);

		controlLayout->addItem(buttonLayout);
		controlLayout->addWidget(meterFrame);

		mainLayout->addItem(textLayout);
		mainLayout->addItem(controlLayout);

		volMeter->setFocusProxy(slider);
	}

	setLayout(mainLayout);

	nameLabel->setText(sourceName);

	slider->setMinimum(0);
	slider->setMaximum(int(FADER_PRECISION));

	bool muted = obs_source_muted(source);
	bool unassigned = IsSourceUnassigned(source);
	mute->setCheckState(GetCheckState(muted, unassigned));
	volMeter->muted = muted || unassigned;
	mute->setAccessibleName(QTStr("VolControl.Mute").arg(sourceName));
	obs_fader_add_callback(obs_fader, OBSVolumeChanged, this);
	obs_volmeter_add_callback(obs_volmeter, OBSVolumeLevel, this);

	sigs.emplace_back(obs_source_get_signal_handler(source), "mute", OBSVolumeMuted, this);
	sigs.emplace_back(obs_source_get_signal_handler(source), "audio_mixers", OBSMixersOrMonitoringChanged, this);
	sigs.emplace_back(obs_source_get_signal_handler(source), "audio_monitoring", OBSMixersOrMonitoringChanged,
			  this);

	QWidget::connect(slider, &VolumeSlider::valueChanged, this, &VolControl::SliderChanged);
	QWidget::connect(mute, &MuteCheckBox::clicked, this, &VolControl::SetMuted);

	obs_fader_attach_source(obs_fader, source);
	obs_volmeter_attach_source(obs_volmeter, source);

	/* Call volume changed once to init the slider position and label */
	VolumeChanged();
}

void VolControl::EnableSlider(bool enable)
{
	slider->setEnabled(enable);
}

VolControl::~VolControl()
{
	obs_fader_remove_callback(obs_fader, OBSVolumeChanged, this);
	obs_volmeter_remove_callback(obs_volmeter, OBSVolumeLevel, this);

	sigs.clear();

	if (contextMenu)
		contextMenu->close();
}

void VolControl::refreshColors()
{
	volMeter->setBackgroundNominalColor(volMeter->getBackgroundNominalColor());
	volMeter->setBackgroundWarningColor(volMeter->getBackgroundWarningColor());
	volMeter->setBackgroundErrorColor(volMeter->getBackgroundErrorColor());
	volMeter->setForegroundNominalColor(volMeter->getForegroundNominalColor());
	volMeter->setForegroundWarningColor(volMeter->getForegroundWarningColor());
	volMeter->setForegroundErrorColor(volMeter->getForegroundErrorColor());
}
