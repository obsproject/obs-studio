/******************************************************************************
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <widgets/AudioMixer.hpp>
#include <components/MenuCheckBox.hpp>

#include <QAction>
#include <QCheckBox>
#include <QWidgetAction>

#include <dialogs/NameDialog.hpp>
#include <utility/item-widget-helpers.hpp>

#include "moc_AudioMixer.cpp"

namespace {
bool isHideInMixer(obs_source_t *source)
{
	OBSDataAutoRelease priv_settings = obs_source_get_private_settings(source);
	bool hidden = obs_data_get_bool(priv_settings, "mixer_hidden");

	return hidden;
}

bool isPinnedInMixer(obs_source_t *source)
{
	OBSDataAutoRelease priv_settings = obs_source_get_private_settings(source);
	bool hidden = obs_data_get_bool(priv_settings, "mixer_pinned");

	return hidden;
}

bool isSourceAudioActive(obs_source_t *source)
{
	bool active = obs_source_active(source) && obs_source_audio_active(source);

	return active;
}

bool isVolumeLocked(obs_source_t *source)
{
	OBSDataAutoRelease priv_settings = obs_source_get_private_settings(source);
	bool lock = obs_data_get_bool(priv_settings, "volume_locked");

	return lock;
}

} // namespace

AudioMixer::AudioMixer(QWidget *parent) : QFrame(parent)
{
	showInactive = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerShowInactive");
	keepInactiveRight = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerKeepInactiveRight");
	showHidden = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerShowHidden");
	keepHiddenRight = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerKeepHiddenRight");

	mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	setLayout(mainLayout);
	setFrameShape(QFrame::NoFrame);
	setLineWidth(0);

	stackedMixerArea = new QStackedWidget(this);
	stackedMixerArea->setObjectName("stackedMixerArea");

	// Horizontal Widgets
	hMixerContainer = new QWidget(this);
	hMixerContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	hMainLayout = new QVBoxLayout();
	hMainLayout->setContentsMargins(0, 0, 0, 0);
	hMainLayout->setSpacing(0);
	hMixerContainer->setLayout(hMainLayout);

	hMixerScrollArea = new QScrollArea(this);
	hMixerScrollArea->setObjectName("hMixerScrollArea");
	hMixerScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	hMixerScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	hMixerScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	hMixerScrollArea->setWidgetResizable(true);
	hMixerScrollArea->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

	hVolumeWidgets = new QWidget(this);
	hVolumeWidgets->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	hVolumeWidgets->setObjectName("hVolumeWidgets");

	hVolControlLayout = new QVBoxLayout();
	hVolumeWidgets->setLayout(hVolControlLayout);
	hVolControlLayout->setContentsMargins(0, 0, 0, 0);
	hVolControlLayout->setSpacing(0);

	hMixerScrollArea->setWidget(hVolumeWidgets);

	hMainLayout->addWidget(hMixerScrollArea);

	// Vertical Widgets
	vMixerContainer = new QWidget(this);
	vMixerContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	vMainLayout = new QHBoxLayout();
	vMainLayout->setContentsMargins(0, 0, 0, 0);
	vMainLayout->setSpacing(0);
	vMixerContainer->setLayout(vMainLayout);

	vMixerScrollArea = new QScrollArea(this);
	vMixerScrollArea->setObjectName("vMixerScrollArea");
	vMixerScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	vMixerScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	vMixerScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	vMixerScrollArea->setWidgetResizable(true);
	vMixerScrollArea->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

	vVolumeWidgets = new QWidget(this);

	vVolumeWidgets->setObjectName("vVolumeWidgets");

	vVolControlLayout = new QHBoxLayout();
	vVolumeWidgets->setLayout(vVolControlLayout);
	vVolControlLayout->setContentsMargins(0, 0, 0, 0);
	vVolControlLayout->setSpacing(0);
	vVolControlLayout->setAlignment(Qt::AlignLeft);

	vMixerScrollArea->setWidget(vVolumeWidgets);

	vMainLayout->addWidget(vMixerScrollArea);

	stackedMixerArea->addWidget(hMixerContainer);
	stackedMixerArea->addWidget(vMixerContainer);

	//stackedMixerArea->setCurrentWidget(vMixerContainer);

	mixerToolbar = new QToolBar(this);
	mixerToolbar->setIconSize(QSize(16, 16));
	mixerToolbar->setFloatable(false);

	mainLayout->addWidget(stackedMixerArea);
	mainLayout->addWidget(mixerToolbar);

	advAudio = new QAction(this);
	advAudio->setText(QTStr("AdvAudioProps"));
	advAudio->setToolTip(QTStr("AdvAudioProps"));
	QIcon advIcon;
	advIcon.addFile(QString::fromUtf8(":/settings/images/settings/advanced.svg"), QSize(16, 16),
			QIcon::Mode::Normal, QIcon::State::Off);
	advAudio->setIcon(advIcon);
	advAudio->setObjectName("actionMixerToolbarAdvAudio");

	layoutButton = new QAction(this);
	layoutButton->setText("");
	layoutButton->setToolTip("");
	QIcon layoutIcon;
	layoutIcon.addFile(QString::fromUtf8(":/res/images/layout-horizontal.svg"), QSize(16, 16), QIcon::Mode::Normal,
			   QIcon::State::Off);
	layoutButton->setIcon(layoutIcon);
	layoutButton->setObjectName("actionMixerToolbarToggleLayout");

	QWidget *spacer = new QWidget(mixerToolbar);
	spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	QPushButton *optionsButton = new QPushButton(mixerToolbar);
	optionsButton->setText(QTStr("Basic.AudioMixer.Options"));
	utils->addClass(optionsButton, "toolbar-button");
	utils->addClass(optionsButton, "text-bold");
	optionsButton->setMenu(&mixerMenu);

	toggleHiddenButton = new QPushButton(mixerToolbar);
	toggleHiddenButton->setCheckable(true);
	toggleHiddenButton->setChecked(showHidden);
	toggleHiddenButton->setText(QTStr("Basic.AudioMixer.HiddenTotal").arg(0));
	toggleHiddenButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	QIcon hiddenIcon;
	hiddenIcon.addFile(QString::fromUtf8(":/res/images/hidden.svg"), QSize(16, 16), QIcon::Mode::Normal,
			   QIcon::State::Off);
	toggleHiddenButton->setIcon(hiddenIcon);
	utils->addClass(toggleHiddenButton, "toolbar-button");
	utils->addClass(toggleHiddenButton, "toggle-hidden");

	mixerToolbar->addWidget(optionsButton);
	mixerToolbar->addSeparator();
	mixerToolbar->addWidget(toggleHiddenButton);

	mixerToolbar->addSeparator();
	mixerToolbar->addWidget(spacer);
	mixerToolbar->addSeparator();

	mixerToolbar->addAction(layoutButton);
	mixerToolbar->addSeparator();
	mixerToolbar->addAction(advAudio);

	// Setting this property on the QAction itself does not seem to work despite
	// the UI files doing exactly that, so we set it on the action widget directly
	QWidget *advAudioWidget = mixerToolbar->widgetForAction(advAudio);
	utils->addClass(advAudioWidget, "icon-cogs");

	// Connect to OBS signals
	signalHandlers.reserve(signalHandlers.size() + 7);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_activate", AudioMixer::obsSourceActivated, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_deactivate", AudioMixer::obsSourceDeactivated,
				    this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_audio_activate",
				    AudioMixer::obsSourceAudioActivated, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_audio_deactivate",
				    AudioMixer::obsSourceAudioDeactivated, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_create", AudioMixer::obsSourceCreate, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_remove", AudioMixer::obsSourceRemove, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_rename", AudioMixer::obsSourceRename, this);

	// Connect to Qt signals
	connect(hMixerScrollArea, &QScrollArea::customContextMenuRequested, this,
		&AudioMixer::mixerContextMenuRequested);

	connect(vMixerScrollArea, &QScrollArea::customContextMenuRequested, this,
		&AudioMixer::mixerContextMenuRequested);

	connect(optionsButton, &QPushButton::clicked, this, &AudioMixer::showMixerContextMenu);

	connect(&updateTimer, &QTimer::timeout, this, &AudioMixer::updateVolumeLayouts);
	updateTimer.setSingleShot(true);

	OBSBasic *main = OBSBasic::Get();
	if (main) {
		connect(main, &OBSBasic::userSettingChanged, this,
			[this](const std::string &category, const std::string &name) {
				if (category == "BasicWindow" && name == "VerticalVolControl") {
					toggleLayout();
				} else if (category == "BasicWindow" && name == "MixerShowInactive") {
					updateShowInactive();
				} else if (category == "BasicWindow" && name == "MixerKeepInactiveRight") {
					updateKeepInactiveRight();
				} else if (category == "BasicWindow" && name == "MixerShowHidden") {
					updateShowHidden();
				} else if (category == "BasicWindow" && name == "MixerKeepHiddenRight") {
					updateKeepHiddenRight();
				}
			});

		connect(main, &OBSBasic::profileSettingChanged, this,
			[this](const std::string &category, const std::string &name) {
				if (category == "Audio" && name == "MeterDecayRate") {
					updateDecayRate();
				} else if (category == "Audio" && name == "PeakMeterType") {
					updatePeakMeterType();
				}
			});

		connect(main, &OBSBasic::mixerStatusChanged, this, &AudioMixer::queueLayoutUpdate);

		connect(advAudio, &QAction::triggered, main, &OBSBasic::on_actionAdvAudioProperties_triggered,
			Qt::DirectConnection);
		connect(toggleHiddenButton, &QPushButton::clicked, this, &AudioMixer::toggleShowHidden);
		connect(layoutButton, &QAction::triggered, main, &OBSBasic::toggleMixerLayout);
	}

	updateGlobalSources();
	reloadVolumeControls();
	queueLayoutUpdate();
}

AudioMixer::~AudioMixer()
{
	signalHandlers.clear();
	clearVolumeControls();
	globalSources.clear();
}

void AudioMixer::toggleLayout()
{
	bool vertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl");
	setMixerLayoutVertical(vertical);

	reloadVolumeControls();
}

void AudioMixer::toggleShowInactive(bool checked)
{
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "MixerShowInactive", checked);
	OBSBasic *main = OBSBasic::Get();
	if (main) {
		emit main->userSettingChanged("BasicWindow", "MixerShowInactive");
	}
}

void AudioMixer::toggleKeepInactiveRight(bool checked)
{
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "MixerKeepInactiveRight", checked);
	OBSBasic *main = OBSBasic::Get();
	if (main) {
		emit main->userSettingChanged("BasicWindow", "MixerKeepInactiveRight");
	}
}

void AudioMixer::toggleShowHidden(bool checked)
{
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "MixerShowHidden", checked);
	OBSBasic *main = OBSBasic::Get();
	if (main) {
		emit main->userSettingChanged("BasicWindow", "MixerShowHidden");
	}
}

void AudioMixer::toggleKeepHiddenRight(bool checked)
{
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "MixerKeepHiddenRight", checked);
	OBSBasic *main = OBSBasic::Get();
	if (main) {
		emit main->userSettingChanged("BasicWindow", "MixerKeepHiddenRight");
	}
}

void AudioMixer::createVolumeControl(OBSSource source)
{
	if (!obs_source_active(source) && !showInactive) {
		return;
	}
	if (!obs_source_audio_active(source) && !showInactive) {
		return;
	}

	bool vertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl");
	VolControl *vol = new VolControl(source, true, vertical);

	vol->setGlobalInMixer(isSourceGlobal(source));
	vol->setHideInMixer(isHideInMixer(source));
	vol->setPinnedInMixer(isPinnedInMixer(source));

	vol->enableSlider(!isVolumeLocked(source));

	OBSBasic *main = OBSBasic::Get();

	double meterDecayRate = config_get_double(main->Config(), "Audio", "MeterDecayRate");
	vol->setMeterDecayRate(meterDecayRate);

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

	vol->setPeakMeterType(peakMeterType);

	connect(vol, &VolControl::unhideAll, this, &AudioMixer::unhideAllAudioControls);

	InsertQObjectByName(volumes, vol);
}

void AudioMixer::deleteVolumeControl(OBSSource source)
{
	for (size_t i = 0; i < globalSources.size(); i++) {
		if (globalSources[i] == source) {
			globalSources.erase(globalSources.begin() + i);
			break;
		}
	}

	for (size_t i = 0; i < volumes.size(); i++) {
		if (volumes[i]->getSource() == source) {
			delete volumes[i];
			volumes.erase(volumes.begin() + i);
			break;
		}
	}
}

void AudioMixer::showVolumeControl(OBSSource source)
{
	for (size_t i = 0; i < volumes.size(); i++) {
		if (volumes[i]->getSource() == source) {
			queueLayoutUpdate();
			return;
		}
	}

	createVolumeControl(source);
	queueLayoutUpdate();
}

void AudioMixer::hideVolumeControl(OBSSource source)
{
	bool isHidden = isHideInMixer(source);
	bool isAudioActive = isSourceAudioActive(source);

	if (isAudioActive && showInactive) {
		return;
	}

	if (isHidden) {
		return;
	}

	deleteVolumeControl(source);
	queueLayoutUpdate();
}

void AudioMixer::updateGlobalSources()
{
	auto oldTotal = globalSources.size();

	globalSources.clear();

	for (int i = 1; i <= 6; i++) {
		obs_source_t *input = obs_get_output_source(i);
		if (!input) {
			globalSources.emplace_back(nullptr);
		} else {
			globalSources.emplace_back(input);
		}
		obs_source_release(input);
	}

	if (oldTotal != globalSources.size()) {
		queueLayoutUpdate();
	}
}

bool AudioMixer::isSourceGlobal(OBSSource source)
{
	for (const OBSSource &entry : globalSources) {
		if (entry == source) {
			return true;
		}
	}

	return false;
}

void AudioMixer::updateDecayRate()
{
	OBSBasic *main = OBSBasic::Get();
	double meterDecayRate = config_get_double(main->Config(), "Audio", "MeterDecayRate");

	for (size_t i = 0; i < volumes.size(); i++) {
		volumes[i]->setMeterDecayRate(meterDecayRate);
	}
}

void AudioMixer::updatePeakMeterType()
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

	for (size_t i = 0; i < volumes.size(); i++) {
		volumes[i]->setPeakMeterType(peakMeterType);
	}
}

void AudioMixer::reloadVolumeControls()
{
	setUpdatesEnabled(false);
	clearVolumeControls();

	auto ReloadAudioMixer = [this](obs_source_t *source) /* -- */
	{
		uint32_t flags = obs_source_get_output_flags(source);

		if ((flags & OBS_SOURCE_AUDIO) == 0) {
			return true;
		}

		if (isSourceGlobal(source) || isPinnedInMixer(source)) {
			createVolumeControl(source);
			return true;
		}

		if (!obs_source_active(source) && !showInactive) {
			return true;
		}

		if (!obs_source_audio_active(source) && !showInactive) {
			return true;
		}

		createVolumeControl(source);
		return true;
	};

	using ReloadAudioMixer_t = decltype(ReloadAudioMixer);

	auto PreEnum = [](void *data, obs_source_t *source) -> bool /* -- */
	{
		return (*static_cast<ReloadAudioMixer_t *>(data))(source);
	};

	obs_enum_sources(PreEnum, &ReloadAudioMixer);

	queueLayoutUpdate();
	setUpdatesEnabled(true);
}

void AudioMixer::clearVolumeControls()
{
	for (VolControl *vol : volumes) {
		delete vol;
	}

	volumes.clear();
}

void AudioMixer::refreshVolumeColors()
{
	for (VolControl *vol : volumes) {
		vol->refreshColors();
	}
}

void AudioMixer::unhideAllAudioControls()
{
	for (auto volControl : volumes) {
		QSignalBlocker block(volControl);
		volControl->setHideInMixer(false);
	}

	queueLayoutUpdate();
}

void AudioMixer::queueLayoutUpdate()
{
	if (!updateTimer.isActive()) {
		updateTimer.start(0);
	}
}

void AudioMixer::updateVolumeLayouts()
{
	// setUpdatesEnabled(false);

	hiddenCount = 0;
	bool vertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl");

	std::vector<RankedVol> rankedVolumes;

	std::sort(volumes.begin(), volumes.end(),
		  [](const VolControl *a, const VolControl *b) { return a->getCachedName() < b->getCachedName(); });

	for (auto volControl : volumes) {
		int sortWeight = 8;

		bool isGlobal = isSourceGlobal(volControl->getSource());
		bool isPinned = isPinnedInMixer(volControl->getSource());
		bool isHidden = isHideInMixer(volControl->getSource());
		bool isAudioActive = isSourceAudioActive(volControl->getSource());

		volControl->setMixerFlag(OBS::MixerStatus::Global, isGlobal);
		volControl->setMixerFlag(OBS::MixerStatus::Pinned, isPinned);
		volControl->setMixerFlag(OBS::MixerStatus::Hidden, isHidden);
		volControl->setMixerFlag(OBS::MixerStatus::Active, isAudioActive);

		if (isHidden) {
			hiddenCount += 1;
		}

		if (isGlobal) {
			sortWeight = 0;
		} else if (isPinned) {
			sortWeight = 4;
		} else if (isHidden && keepHiddenRight) {
			sortWeight += 1;
		}

		if (!isAudioActive && keepInactiveRight) {
			sortWeight += 2;
		}

		rankedVolumes.push_back({volControl, sortWeight});
	}

	std::sort(rankedVolumes.begin(), rankedVolumes.end(),
		  [](const RankedVol &a, const RankedVol &b) { return a.sortWeight < b.sortWeight; });

	for (auto entry : rankedVolumes) {
		QBoxLayout *layout = vertical ? static_cast<QBoxLayout *>(vVolControlLayout)
					      : static_cast<QBoxLayout *>(hVolControlLayout);

		auto volControl = entry.vol;

		layout->addWidget(volControl);
		volControl->updateName();
		volControl->updateMixerState();

		bool isHidden = volControl->hasMixerFlag(OBS::MixerStatus::Hidden);
		if (isHidden && !showHidden) {
			volControl->hide();
		} else {
			volControl->show();
		}
	}

	toggleHiddenButton->setText(QTStr("%1 hidden").arg(hiddenCount));
	if (hiddenCount == 0) {
		toggleHiddenButton->setDisabled(true);
		utils->toggleClass(toggleHiddenButton, "text-muted", true);
	} else {
		toggleHiddenButton->setDisabled(false);
		utils->toggleClass(toggleHiddenButton, "text-muted", false);
	}

	// setUpdatesEnabled(true);
}

void AudioMixer::mixerContextMenuRequested()
{
	showMixerContextMenu();
}

void AudioMixer::setMixerLayoutVertical(bool vertical)
{
	mixerVertical = vertical;

	if (vertical) {
		stackedMixerArea->setMinimumSize(180, 220);
		stackedMixerArea->setCurrentIndex(1);

		QIcon layoutIcon;
		layoutIcon.addFile(QString::fromUtf8(":/res/images/layout-vertical.svg"), QSize(16, 16),
				   QIcon::Mode::Normal, QIcon::State::Off);
		layoutButton->setIcon(layoutIcon);
	} else {
		stackedMixerArea->setMinimumSize(220, 0);
		stackedMixerArea->setCurrentIndex(0);

		QIcon layoutIcon;
		layoutIcon.addFile(QString::fromUtf8(":/res/images/layout-horizontal.svg"), QSize(16, 16),
				   QIcon::Mode::Normal, QIcon::State::Off);
		layoutButton->setIcon(layoutIcon);
	}

	QWidget *buttonWidget = mixerToolbar->widgetForAction(layoutButton);
	if (buttonWidget) {
		utils->toggleClass(buttonWidget, "icon-layout-vertical", vertical);
		utils->toggleClass(buttonWidget, "icon-layout-horizontal", !vertical);
	}
}

void AudioMixer::showMixerContextMenu()
{
	QAction unhideAllAction(QTStr("UnhideAll"), &mixerMenu);

	QWidgetAction showHiddenAction(&mixerMenu);
	MenuCheckBox showHiddenCheckBox(QTStr("Basic.AudioMixer.ShowHidden"), &mixerMenu);
	showHiddenCheckBox.setAction(&showHiddenAction);
	showHiddenCheckBox.setChecked(showHidden);
	showHiddenAction.setDefaultWidget(&showHiddenCheckBox);

	QWidgetAction showInactiveAction(&mixerMenu);
	MenuCheckBox showInactiveCheckBox(QTStr("Basic.AudioMixer.ShowInactive"), &mixerMenu);
	showInactiveCheckBox.setAction(&showInactiveAction);
	showInactiveCheckBox.setChecked(showInactive);
	showInactiveAction.setDefaultWidget(&showInactiveCheckBox);

	QWidgetAction hiddenOnRightAction(&mixerMenu);
	const char *hiddenShifted = mixerVertical ? "Basic.AudioMixer.KeepHiddenRight"
						  : "Basic.AudioMixer.KeepHiddenBottom";
	MenuCheckBox hiddenOnRightCheckBox(QTStr(hiddenShifted), &mixerMenu);
	hiddenOnRightCheckBox.setAction(&hiddenOnRightAction);
	hiddenOnRightCheckBox.setChecked(keepHiddenRight);
	hiddenOnRightAction.setDefaultWidget(&hiddenOnRightCheckBox);

	QWidgetAction inactiveOnRightAction(&mixerMenu);
	const char *inactiveShifted = mixerVertical ? "Basic.AudioMixer.KeepInactiveRight"
						    : "Basic.AudioMixer.KeepInactiveBottom";
	MenuCheckBox inactiveOnRightCheckBox(QTStr(inactiveShifted), &mixerMenu);
	inactiveOnRightCheckBox.setAction(&inactiveOnRightAction);
	inactiveOnRightCheckBox.setChecked(keepInactiveRight);
	inactiveOnRightAction.setDefaultWidget(&inactiveOnRightCheckBox);

	/* ------------------- */

	connect(&unhideAllAction, &QAction::triggered, this, &AudioMixer::unhideAllAudioControls, Qt::DirectConnection);

	connect(&showHiddenCheckBox, &QCheckBox::toggled, this, &AudioMixer::toggleShowHidden, Qt::DirectConnection);
	connect(&hiddenOnRightCheckBox, &QCheckBox::toggled, this, &AudioMixer::toggleKeepHiddenRight,
		Qt::DirectConnection);

	connect(&showInactiveCheckBox, &QCheckBox::toggled, this, &AudioMixer::toggleShowInactive,
		Qt::DirectConnection);
	connect(&inactiveOnRightCheckBox, &QCheckBox::toggled, this, &AudioMixer::toggleKeepInactiveRight,
		Qt::DirectConnection);

	/* ------------------- */

	mixerMenu.addAction(&unhideAllAction);
	mixerMenu.addSeparator();
	mixerMenu.addAction(&showHiddenAction);
	mixerMenu.addAction(&showInactiveAction);
	mixerMenu.addAction(&hiddenOnRightAction);
	mixerMenu.addAction(&inactiveOnRightAction);
	mixerMenu.exec(QCursor::pos());
}

void AudioMixer::updateShowInactive()
{
	bool settingShowInactive = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerShowInactive");
	if (showInactive == settingShowInactive) {
		return;
	}

	showInactive = settingShowInactive;
	reloadVolumeControls();
}

void AudioMixer::updateKeepInactiveRight()
{
	bool settingKeepInactiveRight =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerKeepInactiveRight");
	if (keepInactiveRight == settingKeepInactiveRight) {
		return;
	}

	keepInactiveRight = settingKeepInactiveRight;
	queueLayoutUpdate();
}

void AudioMixer::updateShowHidden()
{
	bool settingShowHidden = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerShowHidden");
	if (showHidden == settingShowHidden) {
		return;
	}

	showHidden = settingShowHidden;

	toggleHiddenButton->setText(QTStr("Basic.AudioMixer.HiddenTotal").arg(hiddenCount));
	toggleHiddenButton->setChecked(showHidden);

	setUpdatesEnabled(false);
	for (auto volControl : volumes) {
		bool isHidden = isHideInMixer(volControl->getSource());

		if (isHidden) {
			showHidden ? volControl->show() : volControl->hide();
		}
	}
	setUpdatesEnabled(true);
}

void AudioMixer::updateKeepHiddenRight()
{
	bool settingKeepHiddenRight = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerKeepHiddenRight");
	if (keepHiddenRight == settingKeepHiddenRight) {
		return;
	}

	keepHiddenRight = settingKeepHiddenRight;
	queueLayoutUpdate();
}

void AudioMixer::obsSourceActivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO) {
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "showVolumeControl",
					  Q_ARG(OBSSource, OBSSource(source)));
	}
}

void AudioMixer::obsSourceDeactivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO) {
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "hideVolumeControl",
					  Q_ARG(OBSSource, OBSSource(source)));
	}
}

void AudioMixer::obsSourceAudioActivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");

	if (obs_source_active(source)) {
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "showVolumeControl",
					  Q_ARG(OBSSource, OBSSource(source)));
	}
}

void AudioMixer::obsSourceAudioDeactivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "hideVolumeControl",
				  Q_ARG(OBSSource, OBSSource(source)));
}

void AudioMixer::obsSourceCreate(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "updateGlobalSources");
}

void AudioMixer::obsSourceRemove(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "deleteVolumeControl",
				  Q_ARG(OBSSource, OBSSource(source)));
}

void AudioMixer::obsSourceRename(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "queueLayoutUpdate");
}

void AudioMixer::on_actionMixerToolbarMenu_triggered()
{
	OBSBasic *main = OBSBasic::Get();

	QAction unhideAllAction(QTStr("UnhideAll"), this);
	connect(&unhideAllAction, &QAction::triggered, this, &AudioMixer::unhideAllAudioControls, Qt::DirectConnection);

	QAction toggleControlLayoutAction(QTStr("VerticalLayout"), this);
	toggleControlLayoutAction.setCheckable(true);
	toggleControlLayoutAction.setChecked(
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl"));
	connect(&toggleControlLayoutAction, &QAction::changed, main, &OBSBasic::toggleMixerLayout,
		Qt::DirectConnection);

	QMenu popup;
	popup.addAction(&unhideAllAction);
	popup.addSeparator();
	popup.addAction(&toggleControlLayoutAction);
	popup.exec(QCursor::pos());
}
