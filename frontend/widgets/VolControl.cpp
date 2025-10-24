#include "OBSBasic.hpp"

#include <components/MuteCheckBox.hpp>
#include <components/VolumeSlider.hpp>
#include <dialogs/NameDialog.hpp>
#include <widgets/VolControl.hpp>
#include <widgets/VolumeMeter.hpp>
#include <widgets/VolumeName.hpp>

#include <QMessageBox>

#include "moc_VolControl.cpp"
#include <QObjectCleanupHandler>

namespace {
bool isSourceUnassigned(obs_source_t *source)
{
	uint32_t mixes = (obs_source_get_audio_mixers(source) & ((1 << MAX_AUDIO_MIXES) - 1));
	obs_monitoring_type mt = obs_source_get_monitoring_type(source);

	return mixes == 0 && mt != OBS_MONITORING_TYPE_MONITOR_ONLY;
}

void showUnassignedWarning(const char *name)
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
} // namespace

VolControl::VolControl(obs_source_t *source, bool vertical, QWidget *parent)
	: weakSource_(OBSGetWeakRef(source)),
	  levelTotal(0.0f),
	  levelCount(0.0f),
	  obs_fader(obs_fader_create(OBS_FADER_LOG)),
	  vertical(vertical),
	  contextMenu(nullptr),
	  QFrame(parent)
{
	utils = new idian::Utils(this);

	uuid = obs_source_get_uuid(source);

	mainLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	setLayout(mainLayout);

	categoryLabel = new QLabel("Active");
	categoryLabel->setAlignment(Qt::AlignCenter);
	utils->addClass(categoryLabel, "mixer-category");
	utils->addClass(categoryLabel, "text-tiny");

	nameButton = new VolumeName(source, this);
	utils->addClass(nameButton, "text-small");
	utils->addClass(nameButton, "mixer-name");

	muteButton = new QPushButton(this);
	muteButton->setCheckable(true);
	utils->addClass(muteButton, "btn-mute");

	monitorButton = new QPushButton(this);
	monitorButton->setCheckable(true);
	utils->addClass(monitorButton, "btn-monitor");

	volLabel = new QLabel(this);
	volLabel->setObjectName("volLabel");

	slider = new VolumeSlider(obs_fader, Qt::Horizontal, this);
	slider->setMinimum(0);
	slider->setMaximum(int(FADER_PRECISION));

	sourceName = obs_source_get_name(source);
	setObjectName(sourceName);

	utils->applyStateStylingEventFilter(muteButton);
	utils->applyStateStylingEventFilter(monitorButton);

	volMeter = new VolumeMeter(this, source);

	bool muted = obs_source_muted(source);
	bool unassigned = isSourceUnassigned(source);

	volMeter->muted = muted || unassigned;

	setLayoutVertical(vertical);
	setName(sourceName);

	obs_fader_add_callback(obs_fader, obsVolumeChanged, this);

	sigs.emplace_back(obs_source_get_signal_handler(source), "mute", obsVolumeMuted, this);
	sigs.emplace_back(obs_source_get_signal_handler(source), "audio_mixers", obsMixersOrMonitoringChanged, this);
	sigs.emplace_back(obs_source_get_signal_handler(source), "audio_monitoring", obsMixersOrMonitoringChanged,
			  this);
	sigs.emplace_back(obs_source_get_signal_handler(source), "activate", VolControl::obsSourceActivated, this);
	sigs.emplace_back(obs_source_get_signal_handler(source), "deactivate", VolControl::obsSourceDeactivated, this);
	sigs.emplace_back(obs_source_get_signal_handler(source), "audio_activate", VolControl::obsSourceActivated,
			  this);
	sigs.emplace_back(obs_source_get_signal_handler(source), "audio_deactivate", VolControl::obsSourceDeactivated,
			  this);

	sigs.emplace_back(obs_source_get_signal_handler(source), "remove", VolControl::obsSourceDestroy, this);
	sigs.emplace_back(obs_source_get_signal_handler(source), "destroy", VolControl::obsSourceDestroy, this);

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QWidget::customContextMenuRequested, this, &VolControl::showVolumeControlMenu);

	connect(nameButton, &VolumeName::renamed, this, &VolControl::setName);
	connect(nameButton, &VolumeName::clicked, this, &VolControl::showVolumeControlMenu);

	connect(slider, &VolumeSlider::valueChanged, this, &VolControl::sliderChanged);
	connect(muteButton, &QPushButton::clicked, this, &VolControl::handleMuteButton);
	connect(monitorButton, &QPushButton::clicked, this, &VolControl::handleMonitorButton);

	OBSBasic *main = OBSBasic::Get();
	if (main) {
		connect(main, &OBSBasic::profileSettingChanged, this,
			[this](const std::string &category, const std::string &name) {
				if (category == "Audio" && name == "MeterDecayRate") {
					updateDecayRate();
				} else if (category == "Audio" && name == "PeakMeterType") {
					updatePeakMeterType();
				}
			});
	}

	obs_fader_attach_source(obs_fader, source);

	/* Call volume changed once to init the slider position and label */
	changeVolume();

	updateMixerState();
	updateCategoryLabel();
}

VolControl::~VolControl()
{
	obs_fader_remove_callback(obs_fader, obsVolumeChanged, this);

	sigs.clear();

	if (contextMenu) {
		contextMenu->close();
	}
}

void VolControl::obsVolumeChanged(void *data, float)
{
	VolControl *volControl = static_cast<VolControl *>(data);

	QMetaObject::invokeMethod(volControl, "changeVolume", Qt::QueuedConnection);
}

void VolControl::obsVolumeMuted(void *data, calldata_t *)
{
	VolControl *volControl = static_cast<VolControl *>(data);

	QMetaObject::invokeMethod(volControl, "updateMixerState", Qt::QueuedConnection);
}

void VolControl::obsMixersOrMonitoringChanged(void *data, calldata_t *)
{
	VolControl *volControl = static_cast<VolControl *>(data);
	QMetaObject::invokeMethod(volControl, "updateMixerState", Qt::QueuedConnection);
}

void VolControl::obsSourceActivated(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<VolControl *>(data), "sourceActiveChanged", Qt::QueuedConnection,
				  Q_ARG(bool, true));
}

void VolControl::obsSourceDeactivated(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<VolControl *>(data), "sourceActiveChanged", Qt::QueuedConnection,
				  Q_ARG(bool, false));
}

void VolControl::obsSourceDestroy(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<VolControl *>(data), "handleSourceDestroyed", Qt::QueuedConnection);
}

void VolControl::setLayoutVertical(bool vertical)
{
	QBoxLayout *newLayout = new QBoxLayout(vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
	newLayout->setContentsMargins(0, 0, 0, 0);
	newLayout->setSpacing(0);

	if (vertical) {
		setMaximumWidth(120);
		setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

		QHBoxLayout *categoryLayout = new QHBoxLayout;
		QHBoxLayout *nameLayout = new QHBoxLayout;
		QHBoxLayout *controlLayout = new QHBoxLayout;
		QHBoxLayout *volLayout = new QHBoxLayout;
		QFrame *meterFrame = new QFrame;
		QHBoxLayout *meterLayout = new QHBoxLayout;

		volMeter->setVertical(true);
		volMeter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

		slider->setOrientation(Qt::Vertical);
		slider->setLayoutDirection(Qt::LeftToRight);
		slider->setDisplayTicks(true);

		nameButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
		categoryLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
		volLabel->setAlignment(Qt::AlignLeft);

		categoryLayout->setAlignment(Qt::AlignCenter);
		nameLayout->setAlignment(Qt::AlignCenter);
		meterLayout->setAlignment(Qt::AlignCenter);
		controlLayout->setAlignment(Qt::AlignCenter);
		volLayout->setAlignment(Qt::AlignCenter);

		meterFrame->setObjectName("volMeterFrame");

		categoryLayout->setContentsMargins(0, 0, 0, 0);
		categoryLayout->setSpacing(0);
		categoryLayout->addWidget(categoryLabel);

		nameLayout->setContentsMargins(0, 0, 0, 0);
		nameLayout->setSpacing(0);
		nameLayout->addWidget(nameButton);

		controlLayout->setContentsMargins(0, 0, 0, 0);
		controlLayout->setSpacing(0);

		// Add Headphone (audio monitoring) widget here
		controlLayout->addWidget(muteButton);
		controlLayout->addWidget(monitorButton);

		meterLayout->setContentsMargins(0, 0, 0, 0);
		meterLayout->setSpacing(0);
		meterLayout->addWidget(slider);
		meterLayout->addWidget(volMeter);

		meterFrame->setLayout(meterLayout);

		volLayout->setContentsMargins(0, 0, 0, 0);
		volLayout->setSpacing(0);
		volLayout->addWidget(volLabel);
		volLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Maximum));

		newLayout->addItem(categoryLayout);
		newLayout->addItem(nameLayout);
		newLayout->addItem(volLayout);
		newLayout->addWidget(meterFrame);
		newLayout->addItem(controlLayout);

		newLayout->setStretch(0, 0);
		newLayout->setStretch(1, 0);
		newLayout->setStretch(2, 0);
		newLayout->setStretch(3, 1);
		newLayout->setStretch(4, 0);

		volMeter->setFocusProxy(slider);
	} else {
		setMaximumWidth(QWIDGETSIZE_MAX);
		setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

		QVBoxLayout *textLayout = new QVBoxLayout;
		QHBoxLayout *controlLayout = new QHBoxLayout;
		QFrame *meterFrame = new QFrame;
		QVBoxLayout *meterLayout = new QVBoxLayout;
		QVBoxLayout *buttonLayout = new QVBoxLayout;

		volMeter->setVertical(false);
		volMeter->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

		slider->setOrientation(Qt::Horizontal);
		slider->setLayoutDirection(Qt::LeftToRight);
		slider->setDisplayTicks(true);

		nameButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		categoryLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
		volLabel->setAlignment(Qt::AlignRight);

		QHBoxLayout *textTopLayout = new QHBoxLayout;
		textTopLayout->setContentsMargins(0, 0, 0, 0);
		textTopLayout->addWidget(categoryLabel);
		textTopLayout->addWidget(volLabel);

		textLayout->setContentsMargins(0, 0, 0, 0);
		textLayout->addItem(textTopLayout);
		textLayout->addWidget(nameButton);

		meterFrame->setObjectName("volMeterFrame");
		meterFrame->setLayout(meterLayout);

		meterLayout->setContentsMargins(0, 0, 0, 0);
		meterLayout->setSpacing(0);

		meterLayout->addWidget(slider);
		meterLayout->addWidget(volMeter);
		meterLayout->setStretch(0, 2);
		meterLayout->setStretch(1, 2);

		buttonLayout->setContentsMargins(0, 0, 0, 0);
		buttonLayout->setSpacing(0);

		//buttonLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding));
		buttonLayout->addWidget(muteButton);
		buttonLayout->addWidget(monitorButton);

		controlLayout->addItem(buttonLayout);
		controlLayout->addWidget(meterFrame);

		newLayout->addItem(textLayout);
		newLayout->addItem(controlLayout);
		newLayout->setStretch(0, 3);
		newLayout->setStretch(1, 6);

		volMeter->setFocusProxy(slider);
	}

	QWidget().setLayout(mainLayout);

	setLayout(newLayout);
	mainLayout = newLayout;
	updateTabOrder();

	adjustSize();
}

void VolControl::showVolumeControlMenu()
{
	QMenu popup;

	/* ------------------- */

	QAction lockAction(QTStr("LockVolume"), &popup);
	lockAction.setCheckable(true);
	lockAction.setChecked(hasMixerFlag(OBS::MixerStatus::Locked));

	bool isGlobal = hasMixerFlag(OBS::MixerStatus::Global);

	QAction pinAction(QTStr("Basic.AudioMixer.Pin"), &popup);
	bool isPinned = hasMixerFlag(OBS::MixerStatus::Pinned);
	if (isPinned) {
		pinAction.setText(QTStr("Basic.AudioMixer.Unpin"));
	}

	QAction hideAction(QTStr("Basic.AudioMixer.Hide"), &popup);
	bool isHidden = hasMixerFlag(OBS::MixerStatus::Hidden);
	if (isHidden && !isGlobal) {
		hideAction.setText(QTStr("Basic.AudioMixer.Unhide"));
	}

	QAction unhideAllAction(QTStr("UnhideAll"), &popup);
	QAction mixerRenameAction(QTStr("Rename"), &popup);

	QAction copyFiltersAction(QTStr("Copy.Filters"), &popup);
	QAction pasteFiltersAction(QTStr("Paste.Filters"), &popup);

	QAction filtersAction(QTStr("Filters"), &popup);
	QAction propertiesAction(QTStr("Properties"), &popup);

	QAction toggleControlLayoutAction(QTStr("VerticalLayout"), &popup);
	toggleControlLayoutAction.setCheckable(true);
	toggleControlLayoutAction.setChecked(
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl"));

	/* ------------------- */

	OBSSource source = OBSGetStrongRef(weakSource());
	if (!source) {
		return;
	}

	hideAction.setProperty("source", QVariant::fromValue<OBSSource>(source));
	pinAction.setProperty("source", QVariant::fromValue<OBSSource>(source));

	mixerRenameAction.setProperty("source", QVariant::fromValue<OBSSource>(source));

	copyFiltersAction.setProperty("source", QVariant::fromValue<OBSSource>(source));
	pasteFiltersAction.setProperty("source", QVariant::fromValue<OBSSource>(source));

	filtersAction.setProperty("source", QVariant::fromValue<OBSSource>(source));
	propertiesAction.setProperty("source", QVariant::fromValue<OBSSource>(source));

	/* ------------------- */

	OBSBasic *main = OBSBasic::Get();

	connect(&unhideAllAction, &QAction::triggered, this, [this]() { emit unhideAll(); });

	connect(&hideAction, &QAction::triggered, this, [this, isHidden]() { setHideInMixer(!isHidden); });
	connect(
		&pinAction, &QAction::triggered, this, [this, isPinned]() { setPinnedInMixer(!isPinned); },
		Qt::DirectConnection);
	connect(&lockAction, &QAction::toggled, this, &VolControl::setLocked);

	connect(&copyFiltersAction, &QAction::triggered, main, &OBSBasic::actionCopyFilters);
	connect(&pasteFiltersAction, &QAction::triggered, main, &OBSBasic::actionPasteFilters);

	connect(&mixerRenameAction, &QAction::triggered, this, &VolControl::renameSource);

	connect(&filtersAction, &QAction::triggered, main, &OBSBasic::actionOpenSourceFilters);
	connect(&propertiesAction, &QAction::triggered, main, &OBSBasic::actionOpenSourceProperties);

	/* ------------------- */

	connect(&toggleControlLayoutAction, &QAction::changed, main, &OBSBasic::toggleMixerLayout,
		Qt::DirectConnection);

	/* ------------------- */

	copyFiltersAction.setEnabled(obs_source_filter_count(source) > 0);
	pasteFiltersAction.setEnabled(!obs_weak_source_expired(main->copyFiltersSource));

	if (isGlobal) {
		pinAction.setDisabled(true);
		hideAction.setDisabled(true);
	}

	if (isPinned) {
		hideAction.setDisabled(true);
	}

	popup.addAction(&unhideAllAction);
	popup.addSeparator();
	popup.addAction(&pinAction);
	popup.addAction(&hideAction);
	popup.addAction(&lockAction);

	popup.addSeparator();
	popup.addAction(&copyFiltersAction);
	popup.addAction(&pasteFiltersAction);
	popup.addSeparator();
	popup.addAction(&mixerRenameAction);
	popup.addSeparator();
	popup.addAction(&filtersAction);
	popup.addAction(&propertiesAction);

	QPoint menuPos = nameButton->mapToGlobal(nameButton->rect().bottomLeft());
	QSize menuSize = popup.sizeHint();

	QRect available = QGuiApplication::screenAt(menuPos)->availableGeometry();
	int spaceBelow = available.bottom() - menuPos.y();
	int spaceAbove = menuPos.y() - available.top();

	QPoint popupPos = menuPos;

	if (menuSize.height() > spaceBelow && spaceAbove > spaceBelow) {
		popupPos = nameButton->mapToGlobal(nameButton->rect().topLeft());
		popupPos.ry() -= menuSize.height();
	}

	if (popupPos.x() + menuSize.width() > available.right()) {
		popupPos.rx() = available.right() - menuSize.width();
	}

	// toggleControlLayoutAction deletes and re-creates the volume controls
	// meaning that "vol" would be pointing to freed memory.
	if (popup.exec(popupPos) != &toggleControlLayoutAction) {
		//vol->SetContextMenu(nullptr);
	}
}

void VolControl::renameSource()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	obs_source_t *source = action->property("source").value<OBSSource>();

	const char *prevName = obs_source_get_name(source);

	for (;;) {
		std::string name;
		bool accepted = NameDialog::AskForName(this, QTStr("Basic.Main.MixerRename.Title"),
						       QTStr("Basic.Main.MixerRename.Text"), name, QT_UTF8(prevName));
		if (!accepted) {
			return;
		}

		if (name.empty()) {
			OBSMessageBox::warning(this, QTStr("NoNameEntered.Title"), QTStr("NoNameEntered.Text"));
			continue;
		}

		OBSSourceAutoRelease sourceTest = obs_get_source_by_name(name.c_str());

		if (sourceTest) {
			OBSMessageBox::warning(this, QTStr("NameExists.Title"), QTStr("NameExists.Text"));
			continue;
		}

		obs_source_set_name(source, name.c_str());
		break;
	}
}

void VolControl::changeVolume()
{
	QSignalBlocker blocker(slider);
	slider->setValue((int)(obs_fader_get_deflection(obs_fader) * FADER_PRECISION));

	updateText();
}

void VolControl::setLocked(bool locked)
{
	OBSSource source = OBSGetStrongRef(weakSource());
	if (!source) {
		return;
	}
	OBSDataAutoRelease priv_settings = obs_source_get_private_settings(source);
	obs_data_set_bool(priv_settings, "volume_locked", locked);

	enableSlider(!locked);
	setMixerFlag(OBS::MixerStatus::Locked, locked);

	OBSBasic *main = OBSBasic::Get();
	emit main->mixerStatusChanged(uuid);
}

void VolControl::updateCategoryLabel()
{
	QString labelText = QTStr("Basic.AudioMixer.Category.Active");

	if (hasMixerFlag(OBS::MixerStatus::Unassigned)) {
		labelText = QTStr("Basic.AudioMixer.Category.Unassigned");
	} else if (hasMixerFlag(OBS::MixerStatus::Global)) {
		labelText = QTStr("Basic.AudioMixer.Category.Global");
	} else if (hasMixerFlag(OBS::MixerStatus::Pinned)) {
		labelText = QTStr("Basic.AudioMixer.Category.Pinned");
	} else if (hasMixerFlag(OBS::MixerStatus::Hidden)) {
		labelText = QTStr("Basic.AudioMixer.Category.Hidden");
	} else if (!hasMixerFlag(OBS::MixerStatus::Active)) {
		labelText = QTStr("Basic.AudioMixer.Category.Inactive");

		if (hasMixerFlag(OBS::MixerStatus::Preview)) {
			labelText = QTStr("Basic.AudioMixer.Category.Preview");
		}
	}

	bool stylePinned = hasMixerFlag(OBS::MixerStatus::Global) || hasMixerFlag(OBS::MixerStatus::Pinned);
	bool styleInactive = hasMixerFlag(OBS::MixerStatus::Active) != true;
	bool styleHidden = hasMixerFlag(OBS::MixerStatus::Hidden);
	bool styleUnassigned = hasMixerFlag(OBS::MixerStatus::Unassigned);
	bool stylePreviewed = hasMixerFlag(OBS::MixerStatus::Preview);

	utils->toggleClass(this, "volume-pinned", stylePinned);
	utils->toggleClass(this, "volume-inactive", styleInactive);
	utils->toggleClass(this, "volume-preview", styleInactive && stylePreviewed);
	utils->toggleClass(this, "volume-hidden", styleHidden && !stylePinned);
	utils->toggleClass(this, "volume-unassigned", styleUnassigned);

	categoryLabel->setText(labelText);

	utils->polishChildren(this);
}

void VolControl::updateDecayRate()
{
	OBSBasic *main = OBSBasic::Get();
	double meterDecayRate = config_get_double(main->Config(), "Audio", "MeterDecayRate");

	setMeterDecayRate(meterDecayRate);
}

void VolControl::updatePeakMeterType()
{
	OBSBasic *main = OBSBasic::Get();
	uint32_t peakMeterTypeIdx = config_get_uint(main->Config(), "Audio", "PeakMeterType");

	enum obs_peak_meter_type peakMeterType;
	switch (peakMeterTypeIdx) {
	case 0:
		peakMeterType = SAMPLE_PEAK_METER;
		break;
	case 1:
		peakMeterType = TRUE_PEAK_METER;
		break;
	default:
		peakMeterType = SAMPLE_PEAK_METER;
		break;
	}

	setPeakMeterType(peakMeterType);
}

void VolControl::setMuted(bool mute)
{
	OBSSource source = OBSGetStrongRef(weakSource());
	if (!source) {
		return;
	}

	bool prev = obs_source_muted(source);
	bool unassigned = isSourceUnassigned(source);

	obs_source_set_muted(source, mute);

	if (!mute && unassigned) {
		// muteButton->setChecked(true);
		/* Show notice about the source no being assigned to any tracks */
		bool has_shown_warning =
			config_get_bool(App()->GetUserConfig(), "General", "WarnedAboutUnassignedSources");
		if (!has_shown_warning) {
			showUnassignedWarning(obs_source_get_name(source));
		}
	}

	auto undo_redo = [](const std::string &uuid, bool val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		obs_source_set_muted(source, val);
	};

	QString text = QTStr(mute ? "Undo.Volume.Mute" : "Undo.Volume.Unmute");

	const char *name = obs_source_get_name(source);
	OBSBasic::Get()->undo_s.add_action(text.arg(name), std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, mute), uuid, uuid);
}

void VolControl::setMonitoring(obs_monitoring_type type)
{
	OBSSource source = OBSGetStrongRef(weakSource());
	if (!source) {
		return;
	}

	obs_monitoring_type prevMonitoringType = obs_source_get_monitoring_type(source);
	obs_source_set_monitoring_type(source, type);

	auto undo_redo = [](const std::string &uuid, obs_monitoring_type val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		obs_source_set_monitoring_type(source, val);
	};

	QString text = QTStr("Undo.MonitoringType.Change");

	const char *name = obs_source_get_name(source);
	OBSBasic::Get()->undo_s.add_action(text.arg(name),
					   std::bind(undo_redo, std::placeholders::_1, prevMonitoringType),
					   std::bind(undo_redo, std::placeholders::_1, type), uuid, uuid);
}

void VolControl::sourceActiveChanged(bool active)
{
	setUseDisabledColors(!active);
	setMixerFlag(OBS::MixerStatus::Active, active);

	OBSBasic *main = OBSBasic::Get();
	emit main->mixerStatusChanged(uuid);
}

void VolControl::updateMixerState()
{
	OBSSource source = OBSGetStrongRef(weakSource());
	if (!source) {
		deleteLater();
		return;
	}

	bool muted = obs_source_muted(source);
	bool unassigned = isSourceUnassigned(source);
	obs_monitoring_type monitoringType = obs_source_get_monitoring_type(source);

	bool isActive = obs_source_active(source) && obs_source_audio_active(source);

	setMixerFlag(OBS::MixerStatus::Active, isActive);
	setUseDisabledColors(!isActive);
	setMixerFlag(OBS::MixerStatus::Unassigned, unassigned);

	QSignalBlocker blockMute(muteButton);
	QSignalBlocker blockMonitor(monitorButton);

	bool showAsMuted = muted || monitoringType == OBS_MONITORING_TYPE_MONITOR_ONLY;
	bool showAsMonitored = monitoringType != OBS_MONITORING_TYPE_NONE;
	bool showAsUnassigned = !muted && unassigned;

	volMeter->muted = (showAsMuted || showAsUnassigned) && !showAsMonitored;

	// Qt doesn't support overriding the QPushButton icon using pseudo state selectors like :checked
	// in QSS so we set a checked class selector on the button to be used instead.
	utils->toggleClass(muteButton, "checked", showAsMuted);
	utils->toggleClass(monitorButton, "checked", showAsMonitored);

	utils->toggleClass(muteButton, "mute-unassigned", showAsUnassigned);

	muteButton->setChecked(showAsMuted);
	monitorButton->setChecked(showAsMonitored);

	if (showAsUnassigned) {
		QIcon unassignedIcon;
		unassignedIcon.addFile(QString::fromUtf8(":/res/images/unassigned.svg"), QSize(16, 16),
				       QIcon::Mode::Normal, QIcon::State::Off);
		muteButton->setIcon(unassignedIcon);
	} else if (showAsMuted) {
		QIcon mutedIcon;
		mutedIcon.addFile(QString::fromUtf8(":/res/images/mute.svg"), QSize(16, 16), QIcon::Mode::Normal,
				  QIcon::State::Off);
		muteButton->setIcon(mutedIcon);
	} else {
		QIcon unmutedIcon;
		unmutedIcon.addFile(QString::fromUtf8(":/settings/images/settings/audio.svg"), QSize(16, 16),
				    QIcon::Mode::Normal, QIcon::State::Off);
		muteButton->setIcon(unmutedIcon);
	}

	if (showAsMonitored) {
		QIcon monitorOnIcon;
		monitorOnIcon.addFile(QString::fromUtf8(":/res/images/headphones.svg"), QSize(16, 16),
				      QIcon::Mode::Normal, QIcon::State::Off);
		monitorButton->setIcon(monitorOnIcon);
	} else {
		QIcon monitorOffIcon;
		monitorOffIcon.addFile(QString::fromUtf8(":/res/images/headphones-off.svg"), QSize(16, 16),
				       QIcon::Mode::Normal, QIcon::State::Off);
		monitorButton->setIcon(monitorOffIcon);
	}

	utils->repolish(muteButton);
	utils->repolish(monitorButton);

	updateCategoryLabel();
}

void VolControl::handleMuteButton(bool mute)
{
	OBSSource source = OBSGetStrongRef(weakSource());
	if (!source) {
		return;
	}

	// The Mute and Monitor buttons in the volume mixer work as a pseudo quad-state toggle.
	// Both buttons must be in their "off" state in order to actually process it as a mute.
	// Otherwise, clicking "Mute" with monitoring enabled will toggle the monitoring type.
	obs_monitoring_type monitoringType = obs_source_get_monitoring_type(source);

	if (mute && monitoringType == OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT) {
		setMonitoring(OBS_MONITORING_TYPE_MONITOR_ONLY);
	} else if (!mute && monitoringType == OBS_MONITORING_TYPE_MONITOR_ONLY) {
		setMonitoring(OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT);
	} else {
		setMuted(mute);
	}
}

void VolControl::handleMonitorButton(bool enableMonitoring)
{
	OBSSource source = OBSGetStrongRef(weakSource());
	if (!source) {
		return;
	}

	// The Mute and Monitor buttons in the volume mixer work as a pseudo quad-state toggle.
	// The source is only ever actually "Muted" if Monitoring is set to None.
	obs_monitoring_type monitoringType = obs_source_get_monitoring_type(source);

	bool muted = obs_source_muted(source);

	if (!enableMonitoring) {
		setMonitoring(OBS_MONITORING_TYPE_NONE);
		if (monitoringType == OBS_MONITORING_TYPE_MONITOR_ONLY) {
			setMuted(true);
		}
	} else if (enableMonitoring && muted) {
		setMonitoring(OBS_MONITORING_TYPE_MONITOR_ONLY);
		setMuted(false);
	} else if (enableMonitoring && !muted) {
		setMonitoring(OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT);
	}
}

void VolControl::sliderChanged(int vol)
{
	OBSSource source = OBSGetStrongRef(weakSource());
	if (!source) {
		return;
	}

	float prev = obs_source_get_volume(source);

	obs_fader_set_deflection(obs_fader, float(vol) / FADER_PRECISION);
	updateText();

	auto undo_redo = [](const std::string &uuid, float val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		obs_source_set_volume(source, val);
	};

	float val = obs_source_get_volume(source);
	const char *name = obs_source_get_name(source);
	OBSBasic::Get()->undo_s.add_action(QTStr("Undo.Volume.Change").arg(name),
					   std::bind(undo_redo, std::placeholders::_1, prev),
					   std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid, true);
}

void VolControl::updateText()
{
	QString text;
	float db = obs_fader_get_db(obs_fader);

	if (db < -96.0f) {
		text = "-inf dB";
	} else {
		text = QString::number(db, 'f', 1).append(" dB");
	}

	volLabel->setText(text);

	OBSSource source = OBSGetStrongRef(weakSource());
	if (!source) {
		return;
	}

	bool muted = obs_source_muted(source);
	const char *accTextLookup = muted ? "VolControl.SliderMuted" : "VolControl.SliderUnmuted";

	QString sourceName = obs_source_get_name(source);
	QString accText = QTStr(accTextLookup).arg(sourceName);

	slider->setAccessibleName(accText);
}

void VolControl::setVertical(bool vertical_)
{
	if (vertical == vertical_) {
		return;
	}

	vertical = vertical_;

	setLayoutVertical(vertical);
}

void VolControl::updateTabOrder()
{
	QWidget *prevFocus = firstWidget()->previousInFocusChain();
	QWidget *lastFocus = lastWidget()->nextInFocusChain();

	if (vertical) {
		setTabOrder(prevFocus, nameButton);
		setTabOrder(nameButton, slider);
		setTabOrder(slider, muteButton);
		setTabOrder(muteButton, monitorButton);
		setTabOrder(monitorButton, lastFocus);
	} else {
		setTabOrder(prevFocus, nameButton);
		setTabOrder(nameButton, muteButton);
		setTabOrder(muteButton, monitorButton);
		setTabOrder(monitorButton, slider);
		setTabOrder(slider, lastFocus);
	}
}

void VolControl::updateName()
{
	setName(sourceName);
}

void VolControl::setName(QString name)
{
	sourceName = name;

	muteButton->setAccessibleName(QTStr("VolControl.Mute").arg(name));
}

void VolControl::setMeterDecayRate(qreal q)
{
	volMeter->setPeakDecayRate(q);
}

void VolControl::setPeakMeterType(enum obs_peak_meter_type peakMeterType)
{
	volMeter->setPeakMeterType(peakMeterType);
}

void VolControl::enableSlider(bool enable)
{
	slider->setEnabled(enable);
}

void VolControl::setMixerFlag(OBS::MixerStatus category, bool enable)
{
	using T = std::underlying_type_t<OBS::MixerStatus>;
	T value = static_cast<T>(statusCategory);
	T bit = static_cast<T>(category);

	value = enable ? (value | bit) : (value & ~bit);

	statusCategory = static_cast<OBS::MixerStatus>(value);
}

bool VolControl::hasMixerFlag(OBS::MixerStatus category)
{
	using T = std::underlying_type_t<OBS::MixerStatus>;
	return (static_cast<T>(statusCategory) & static_cast<T>(category)) != 0;
}

void VolControl::setUseDisabledColors(bool greyscale)
{
	volMeter->setUseDisabledColors(greyscale);
}

void VolControl::setGlobalInMixer(bool global)
{
	if (hasMixerFlag(OBS::MixerStatus::Global) != global) {
		setMixerFlag(OBS::MixerStatus::Global, global);

		OBSBasic *main = OBSBasic::Get();
		emit main->mixerStatusChanged(uuid);
	}
}

void VolControl::setPinnedInMixer(bool pinned)
{
	if (hasMixerFlag(OBS::MixerStatus::Pinned) != pinned) {
		OBSSource source = OBSGetStrongRef(weakSource());
		if (!source) {
			return;
		}
		OBSDataAutoRelease priv_settings = obs_source_get_private_settings(source);
		obs_data_set_bool(priv_settings, "mixer_pinned", pinned);

		setMixerFlag(OBS::MixerStatus::Pinned, pinned);

		if (pinned) {
			// Unset hidden state when pinning controls
			setHideInMixer(false);
		}

		OBSBasic *main = OBSBasic::Get();
		emit main->mixerStatusChanged(uuid);
	}
}

void VolControl::setHideInMixer(bool hidden)
{
	if (hasMixerFlag(OBS::MixerStatus::Hidden) != hidden) {
		OBSSource source = OBSGetStrongRef(weakSource());
		if (!source) {
			return;
		}
		OBSDataAutoRelease priv_settings = obs_source_get_private_settings(source);
		obs_data_set_bool(priv_settings, "mixer_hidden", hidden);

		setMixerFlag(OBS::MixerStatus::Hidden, hidden);

		OBSBasic *main = OBSBasic::Get();
		emit main->mixerStatusChanged(uuid);
	}
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

void VolControl::debugHideControls()
{
	slider->hide();
	volLabel->hide();
	muteButton->hide();
}

void VolControl::setLevels(const float magnitude[MAX_AUDIO_CHANNELS], const float peak[MAX_AUDIO_CHANNELS],
			   const float inputPeak[MAX_AUDIO_CHANNELS])
{
	if (volMeter) {
		volMeter->setLevels(magnitude, peak, inputPeak);
	}
}
