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

#include <widgets/OBSBasic.hpp>
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

	mixerToolbar->addWidget(toggleHiddenButton);

	mixerToolbar->addSeparator();
	mixerToolbar->addWidget(spacer);
	mixerToolbar->addSeparator();

	mixerToolbar->addAction(layoutButton);
	mixerToolbar->addSeparator();
	mixerToolbar->addAction(advAudio);
	mixerToolbar->addSeparator();
	mixerToolbar->addWidget(optionsButton);

	// Setting this property on the QAction itself does not seem to work despite
	// the UI files doing exactly that, so we set it on the action widget directly
	QWidget *advAudioWidget = mixerToolbar->widgetForAction(advAudio);
	utils->addClass(advAudioWidget, "icon-cogs");

	// Connect to OBS signals
	signalHandlers.reserve(signalHandlers.size() + 8);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_create", AudioMixer::obsSourceCreate, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_remove", AudioMixer::obsSourceRemove, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_destroy", AudioMixer::obsSourceRemove, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_rename", AudioMixer::obsSourceRename, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_activate", AudioMixer::obsSourceActivated, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_deactivate", AudioMixer::obsSourceDeactivated,
				    this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_audio_activate",
				    AudioMixer::obsSourceAudioActivated, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_audio_deactivate",
				    AudioMixer::obsSourceAudioDeactivated, this);

	obs_frontend_add_event_callback(AudioMixer::onFrontendEvent, this);

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
				} else if (category == "BasicWindow" && name == "ShowListboxToolbars") {
					updateShowToolbar();
				} else if (category == "Accessibility" && name == "SettingsChanged") {
					refreshVolumeColors();
				}
			});

		connect(main, &OBSBasic::mixerStatusChanged, this, &AudioMixer::queueLayoutUpdate);

		connect(advAudio, &QAction::triggered, main, &OBSBasic::on_actionAdvAudioProperties_triggered,
			Qt::DirectConnection);
		connect(toggleHiddenButton, &QPushButton::clicked, this, &AudioMixer::toggleShowHidden);
		connect(layoutButton, &QAction::triggered, main, &OBSBasic::toggleMixerLayout);
	}

	updateShowToolbar();
	updatePreviewSources();
	updateGlobalSources();

	reloadVolumeControls();
}

AudioMixer::~AudioMixer()
{
	signalHandlers.clear();

	previewSources.clear();
	globalSources.clear();

	clearVolumeControls();

	obs_frontend_remove_event_callback(AudioMixer::onFrontendEvent, this);
}

void AudioMixer::toggleLayout()
{
	bool vertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl");
	setMixerLayoutVertical(vertical);

	queueLayoutUpdate();
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

VolControl *AudioMixer::createVolumeControl(obs_source_t *source)
{
	bool vertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl");
	VolControl *vol = new VolControl(source, vertical, this);

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

	return vol;
}

void AudioMixer::updateControlVisibility(std::string uuid)
{
	auto item = volumeList.find(uuid.c_str());
	if (item == volumeList.end()) {
		return;
	}

	VolControl *control = item->second;
	bool show = getMixerVisibilityForControl(control);

	if (show) {
		control->show();
	} else {
		control->hide();
	}

	queueLayoutUpdate();
}

void AudioMixer::sourceCreated(QString uuid)
{
	addControlForUuid(uuid);
	updateGlobalSources();
}

void AudioMixer::sourceRemoved(QString uuid)
{
	removeControlForUuid(uuid);
	updateGlobalSources();
}

void AudioMixer::updatePreviewSources()
{
	bool isStudioMode = obs_frontend_preview_program_mode_active();
	clearPreviewSources();

	if (isStudioMode) {
		OBSSourceAutoRelease previewSource = obs_frontend_get_current_preview_scene();
		if (!previewSource) {
			return;
		}

		obs_scene_t *previewScene = obs_scene_from_source(previewSource);
		if (!previewScene) {
			return;
		}

		if (!previewScene) {
			return;
		}

		auto getPreviewSources = [this](obs_scene_t *, obs_sceneitem_t *item) /* -- */
		{
			obs_source_t *source = obs_sceneitem_get_source(item);
			if (!source) {
				return true;
			}

			uint32_t flags = obs_source_get_output_flags(source);
			if ((flags & OBS_SOURCE_AUDIO) == 0) {
				return true;
			}

			const char *uuid = obs_source_get_uuid(source);
			if (uuid && *uuid) {
				previewSources.insert(QString::fromUtf8(uuid));
			}

			return true;
		};

		using getPreviewSources_t = decltype(getPreviewSources);

		auto previewEnum = [](obs_scene_t *scene, obs_sceneitem_t *item, void *data) -> bool /* -- */
		{
			return (*static_cast<getPreviewSources_t *>(data))(scene, item);
		};

		obs_scene_enum_items(previewScene, previewEnum, &getPreviewSources);
	}
}

void AudioMixer::updateGlobalSources()
{
	globalSources.clear();

	for (int i = 1; i <= 6; i++) {
		OBSSourceAutoRelease source = obs_get_output_source(i);
		if (source) {
			std::string uuid = obs_source_get_uuid(source);
			globalSources.insert(QString::fromStdString(uuid));
		}
	}

	queueLayoutUpdate();
}

QBoxLayout *AudioMixer::activeLayout() const
{
	bool vertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl");

	QBoxLayout *layout = vertical ? static_cast<QBoxLayout *>(vVolControlLayout)
				      : static_cast<QBoxLayout *>(hVolControlLayout);

	return layout;
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

		std::string uuid = obs_source_get_uuid(source);

		addControlForUuid(QString::fromStdString(uuid));
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

bool AudioMixer::getMixerVisibilityForControl(VolControl *control)
{
	OBSSource source = OBSGetStrongRef(control->weakSource());

	bool isPinned = isPinnedInMixer(source);
	bool isPreviewed = isSourcePreviewed(source);
	bool isHidden = isHideInMixer(source);
	bool isAudioActive = isSourceAudioActive(source);

	if (isPinned) {
		return true;
	}

	if (isHidden && showHidden) {
		return true;
	}

	if (!isAudioActive && showInactive) {
		return !isHidden;
	}

	if (isAudioActive) {
		return !isHidden;
	}

	if (isPreviewed) {
		return !isHidden;
	}

	return false;
}

void AudioMixer::clearPreviewSources()
{
	previewSources.clear();
}

bool AudioMixer::isSourcePreviewed(obs_source_t *source)
{
	if (!source) {
		return false;
	}

	const char *uuid = obs_source_get_uuid(source);
	if (!uuid) {
		return false;
	}

	if (previewSources.find(uuid) != previewSources.end()) {
		return true;
	}

	return false;
}

bool AudioMixer::isSourceGlobal(obs_source_t *source)
{
	if (!source) {
		return false;
	}

	const char *uuid = obs_source_get_uuid(source);
	if (!uuid) {
		return false;
	}

	if (globalSources.find(uuid) != globalSources.end()) {
		return true;
	}

	return false;
}

void AudioMixer::clearVolumeControls()
{
	for (const auto &[uuid, control] : volumeList) {
		if (control) {
			control->deleteLater();
		}
	}
	volumeList.clear();
}

void AudioMixer::refreshVolumeColors()
{
	for (const auto &[uuid, control] : volumeList) {
		control->refreshColors();
	}
}

void AudioMixer::unhideAllAudioControls()
{
	for (const auto &[uuid, control] : volumeList) {
		control->setHideInMixer(false);
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
	setUpdatesEnabled(false);

	hiddenCount = 0;
	bool vertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl");

	std::vector<RankedVol> rankedVolumes;
	rankedVolumes.reserve(volumeList.size());
	for (const auto &entry : volumeList) {
		VolControl *control = entry.second;
		if (control) {
			rankedVolumes.push_back({control, 0});
		}
	}

	std::sort(rankedVolumes.begin(), rankedVolumes.end(), [](const RankedVol &a, const RankedVol &b) {
		const QString &nameA = a.vol->getCachedName();
		const QString &nameB = b.vol->getCachedName();

		return nameA < nameB;
	});

	for (RankedVol &entry : rankedVolumes) {
		VolControl *volControl = entry.vol;

		int sortWeight = 0;

		OBSSource source = OBSGetStrongRef(volControl->weakSource());
		if (!source) {
			std::string cachedName = volControl->getCachedName().toStdString();
			const char *name = cachedName.c_str();
			blog(LOG_INFO, "Tried to sort VolControl for '%s' but source is null", name);
			continue;
		}

		bool isPreviewed = isSourcePreviewed(source);
		bool isGlobal = isSourceGlobal(source);
		bool isPinned = isPinnedInMixer(source);
		bool isHidden = isHideInMixer(source);
		bool isAudioActive = isSourceAudioActive(source);
		bool isLocked = isVolumeLocked(source);

		volControl->setMixerFlag(OBS::MixerStatus::Preview, isPreviewed);
		volControl->setMixerFlag(OBS::MixerStatus::Global, isGlobal);
		volControl->setMixerFlag(OBS::MixerStatus::Pinned, isPinned);
		volControl->setMixerFlag(OBS::MixerStatus::Hidden, isHidden);
		volControl->setMixerFlag(OBS::MixerStatus::Active, isAudioActive);
		volControl->setMixerFlag(OBS::MixerStatus::Locked, isLocked);

		if (isHidden) {
			hiddenCount += 1;
		}

		if (!isGlobal) {
			sortWeight += 20;
		}

		if (!isPinned) {
			sortWeight += 20;
		}

		if (isHidden && keepHiddenRight) {
			sortWeight += 20;

			if (isPreviewed) {
				sortWeight -= 10;
			}
		}

		if (!isAudioActive && keepInactiveRight) {
			sortWeight += 50;

			if (isPreviewed) {
				sortWeight -= 10;
			}
		}

		entry.sortWeight = sortWeight;
	}

	std::sort(rankedVolumes.begin(), rankedVolumes.end(),
		  [](const RankedVol &a, const RankedVol &b) { return a.sortWeight < b.sortWeight; });

	VolControl *prevControl = nullptr;
	for (const auto &entry : rankedVolumes) {
		QBoxLayout *layout = activeLayout();

		VolControl *volControl = entry.vol;

		layout->addWidget(volControl);
		volControl->setVertical(vertical);
		volControl->updateName();
		volControl->updateMixerState();

		bool showControl = getMixerVisibilityForControl(volControl);

		if (showControl) {
			volControl->show();
		} else {
			volControl->hide();
		}

		if (prevControl == nullptr) {
			setTabOrder(previousInFocusChain(), volControl->firstWidget());
		} else {
			setTabOrder(prevControl->lastWidget(), volControl->firstWidget());
		}
		volControl->updateTabOrder();

		prevControl = volControl;
	}

	toggleHiddenButton->setText(QTStr("%1 hidden").arg(hiddenCount));
	if (hiddenCount == 0) {
		toggleHiddenButton->setDisabled(true);
		utils->toggleClass(toggleHiddenButton, "text-muted", true);
	} else {
		toggleHiddenButton->setDisabled(false);
		utils->toggleClass(toggleHiddenButton, "text-muted", false);
	}

	setUpdatesEnabled(true);
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

void AudioMixer::addControlForUuid(QString uuid)
{
	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toStdString().c_str());

	QPointer<VolControl> newControl = createVolumeControl(source);
	activeLayout()->addWidget(newControl);

	volumeList.insert({uuid, newControl});
	queueLayoutUpdate();
}

void AudioMixer::removeControlForUuid(QString uuid)
{
	auto item = volumeList.find(uuid);
	if (item != volumeList.end()) {
		VolControl *widget = item->second;
		if (widget) {
			activeLayout()->removeWidget(widget);
			widget->deleteLater();
		}

		volumeList.erase(item);
	}

	previewSources.erase(uuid);
	globalSources.erase(uuid);
}

void AudioMixer::onFrontendEvent(obs_frontend_event event, void *data)
{
	AudioMixer *mixer = static_cast<AudioMixer *>(data);
	mixer->handleFrontendEvent(event);
}

void AudioMixer::handleFrontendEvent(obs_frontend_event event)
{
	switch (event) {
	case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
		updatePreviewSources();
		queueLayoutUpdate();
		break;
	case OBS_FRONTEND_EVENT_EXIT:
		obs_frontend_remove_event_callback(AudioMixer::onFrontendEvent, this);
		break;
	default:
		break;
	}
}

void AudioMixer::updateShowInactive()
{
	bool settingShowInactive = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerShowInactive");
	if (showInactive == settingShowInactive) {
		return;
	}

	showInactive = settingShowInactive;

	queueLayoutUpdate();
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

	queueLayoutUpdate();
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

void AudioMixer::updateShowToolbar()
{
	bool settingShowToolbar = config_get_bool(App()->GetUserConfig(), "BasicWindow", "ShowListboxToolbars");

	if (showToolbar == settingShowToolbar) {
		return;
	}

	showToolbar = settingShowToolbar;

	showToolbar ? mixerToolbar->show() : mixerToolbar->hide();
}

void AudioMixer::obsSourceActivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO) {
		std::string uuid = obs_source_get_uuid(source);
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "updateControlVisibility",
					  Qt::QueuedConnection, Q_ARG(std::string, uuid));
	}
}

void AudioMixer::obsSourceDeactivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO) {
		std::string uuid = obs_source_get_uuid(source);
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "updateControlVisibility",
					  Qt::QueuedConnection, Q_ARG(std::string, uuid));
	}
}

void AudioMixer::obsSourceAudioActivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");

	if (obs_source_active(source)) {
		std::string uuid = obs_source_get_uuid(source);
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "updateControlVisibility",
					  Qt::QueuedConnection, Q_ARG(std::string, uuid));
	}
}

void AudioMixer::obsSourceAudioDeactivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");

	std::string uuid = obs_source_get_uuid(source);
	QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "updateControlVisibility", Qt::QueuedConnection,
				  Q_ARG(std::string, uuid));
}

void AudioMixer::obsSourceCreate(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO) {
		std::string uuid = obs_source_get_uuid(source);
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "sourceCreated", Qt::QueuedConnection,
					  Q_ARG(QString, QString::fromStdString(uuid)));
	}
}

void AudioMixer::obsSourceRemove(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO) {
		std::string uuid = obs_source_get_uuid(source);
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "sourceRemoved", Qt::QueuedConnection,
					  Q_ARG(QString, QString::fromStdString(uuid)));
	}
}

void AudioMixer::obsSourceRename(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "queueLayoutUpdate", Qt::QueuedConnection);
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
