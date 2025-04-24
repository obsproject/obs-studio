#include "VolControl.hpp"
#include "VolumeMeter.hpp"
#include "OBSBasic.hpp"

#include <components/MuteCheckBox.hpp>
#include <components/OBSSourceLabel.hpp>
#include <components/VolumeSlider.hpp>
#include <components/VerticalLabel.hpp>

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

void VolControl::VolumeChanged()
{
	slider->blockSignals(true);
	slider->setValue((int)(obs_fader_get_deflection(obs_fader) * FADER_PRECISION));
	slider->blockSignals(false);

	updateText();
}

void VolControl::OBSAudioChanged(void *data, calldata_t *)
{
	VolControl *volControl = static_cast<VolControl *>(data);
	QMetaObject::invokeMethod(volControl, "AudioChanged", Qt::QueuedConnection);
}

void VolControl::AudioChanged()
{
	bool muted = obs_source_muted(source);
	bool unassigned = IsSourceUnassigned(source);

	auto newState = GetCheckState(muted, unassigned);
	if (mute && (mute->checkState() != newState))
		mute->setCheckState(newState);

	if (volMeter)
		volMeter->muted = muted || unassigned;

	nameLabel->setDisabled(muted || unassigned);
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
	peakDecayRate = q;

	if (volMeter)
		volMeter->setPeakDecayRate(peakDecayRate);
}

void VolControl::setPeakMeterType(enum obs_peak_meter_type peakMeterType_)
{
	peakMeterType = peakMeterType_;

	if (volMeter)
		volMeter->setPeakMeterType(peakMeterType);
}

VolControl::VolControl(OBSSource source_, bool vertical_, bool hidden)
	: source(std::move(source_)),
	  levelTotal(0.0f),
	  levelCount(0.0f),
	  obs_fader(obs_fader_create(OBS_FADER_LOG)),
	  obs_volmeter(obs_volmeter_create(OBS_FADER_LOG)),
	  vertical(vertical_),
	  contextMenu(nullptr)
{
	QString sourceName = obs_source_get_name(source);
	setObjectName(sourceName);

	nameLabel = new OBSSourceLabel(source);
	nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	if (vertical)
		nameLabel->ShowVertical();
	else
		nameLabel->ShowNormal();

	QCheckBox *expand = new QCheckBox();
	expand->setChecked(hidden);
	expand->setProperty("class", "checkbox-icon indicator-expand");
	connect(expand, &QCheckBox::clicked, this, &VolControl::ResetControls);

	controlLayout = new QGridLayout();
	controlLayout->setSpacing(2);
	controlLayout->setContentsMargins(0, 0, 0, 0);

	controlLayout->addWidget(expand, 0, 0);
	setLayout(controlLayout);

	if (vertical)
		setMaximumWidth(110);

	sigs.emplace_back(obs_source_get_signal_handler(source), "mute", OBSAudioChanged, this);
	sigs.emplace_back(obs_source_get_signal_handler(source), "audio_mixers", OBSAudioChanged, this);
	sigs.emplace_back(obs_source_get_signal_handler(source), "audio_monitoring", OBSAudioChanged, this);

	ResetControls(hidden);
}

void VolControl::ResetControls(bool hidden)
{
	SetSourceMixerHidden(source, hidden);

	if (hidden) {
		obs_fader_remove_callback(obs_fader, OBSVolumeChanged, this);
		obs_volmeter_remove_callback(obs_volmeter, OBSVolumeLevel, this);
		volLabel.reset();
		mute.reset();
		slider.reset();
		volMeter.reset();
		config.reset();

		if (vertical) {
			nameLabel->ShowVertical();
			controlLayout->addWidget(nameLabel, 1, 0);
			controlLayout->setAlignment(nameLabel, Qt::AlignTop);
		} else {
			nameLabel->ShowNormal();
			controlLayout->addWidget(nameLabel, 0, 1, 1, -1);
			controlLayout->setAlignment(nameLabel, Qt::AlignLeft);
		}
	} else {
		nameLabel->ShowNormal();
		controlLayout->addWidget(nameLabel, 0, 1, 1, -1);
		controlLayout->setAlignment(nameLabel, Qt::AlignLeft);
	}

	bool muted = obs_source_muted(source);
	bool unassigned = IsSourceUnassigned(source);
	nameLabel->setDisabled(muted || unassigned);

	if (hidden)
		return;

	// Volume label
	volLabel.reset(new QLabel());
	volLabel->setObjectName("volLabel");

	// Mute checkbox
	mute.reset(new MuteCheckBox());
	mute->setCheckState(GetCheckState(muted, unassigned));
	mute->setAccessibleName(QTStr("VolControl.Mute").arg(nameLabel->text()));
#ifdef __APPLE__
	mute->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif
	connect(mute.data(), &MuteCheckBox::clicked, this, &VolControl::SetMuted);

	// Fader
	slider.reset(new VolumeSlider(obs_fader, vertical ? Qt::Vertical : Qt::Horizontal));
	slider->setLayoutDirection(Qt::LeftToRight);
	slider->setDisplayTicks(true);
	slider->setMinimum(0);
	slider->setMaximum(int(FADER_PRECISION));
	obs_fader_add_callback(obs_fader, OBSVolumeChanged, this);
	obs_fader_attach_source(obs_fader, source);
	connect(slider.data(), &VolumeSlider::valueChanged, this, &VolControl::SliderChanged);

	// Config button
	config.reset(new QPushButton());
	config->setProperty("class", "icon-dots-vert");
	config->setAutoDefault(false);
	config->setAccessibleName(QTStr("VolControl.Properties").arg(nameLabel->text()));
	connect(config.data(), &QAbstractButton::clicked, this, &VolControl::EmitConfigClicked);

	// Volume meter
	volMeter.reset(new VolumeMeter(nullptr, obs_volmeter, vertical));
	volMeter.data()->muted = muted || unassigned;
	obs_volmeter_add_callback(obs_volmeter, OBSVolumeLevel, this);
	obs_volmeter_attach_source(obs_volmeter, source);

	if (vertical) {
		controlLayout->addWidget(volLabel.data(), 1, 0, 1, -1);
		controlLayout->addWidget(slider.data(), 2, 0, Qt::AlignHCenter);
		controlLayout->addWidget(volMeter.data(), 2, 1, Qt::AlignHCenter);
		controlLayout->addWidget(mute.data(), 3, 0, Qt::AlignHCenter);
		controlLayout->addWidget(config.data(), 3, 1, Qt::AlignHCenter);
		controlLayout->setColumnStretch(2, 1);
	} else {
		controlLayout->addWidget(volLabel.data(), 0, 2, Qt::AlignRight);
		controlLayout->addWidget(config.data(), 1, 0, Qt::AlignVCenter);
		controlLayout->addWidget(volMeter.data(), 1, 1, 1, -1, Qt::AlignVCenter);
		controlLayout->addWidget(mute.data(), 2, 0, Qt::AlignVCenter);
		controlLayout->addWidget(slider.data(), 2, 1, 1, -1, Qt::AlignVCenter);
	}

	SetMeterDecayRate(peakDecayRate);
	setPeakMeterType(peakMeterType);
	EnableSlider(sliderEnabled);

	/* Call volume changed once to init the slider position and label */
	VolumeChanged();
}

void VolControl::EnableSlider(bool enable)
{
	sliderEnabled = enable;

	if (slider)
		slider->setEnabled(enable);
}

VolControl::~VolControl()
{
	obs_fader_remove_callback(obs_fader, OBSVolumeChanged, this);
	obs_volmeter_remove_callback(obs_volmeter, OBSVolumeLevel, this);

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
