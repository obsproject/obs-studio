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

#include "AudioMixer.hpp"

#include <components/MenuCheckBox.hpp>
#include <dialogs/NameDialog.hpp>
#include <utility/item-widget-helpers.hpp>
#include <widgets/OBSBasic.hpp>

#include <Idian/Utils.hpp>

#include <QAction>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidgetAction>

#include "moc_AudioMixer.cpp"

constexpr int GLOBAL_SOURCE_TOTAL = 6;

namespace {
bool isHiddenInMixer(obs_source_t *source)
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
	mixerVertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolumeControl");
	showInactive = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerShowInactive");
	keepInactiveLast = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerKeepInactiveLast");
	showHidden = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerShowHidden");
	keepHiddenLast = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerKeepHiddenLast");

	mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	setLayout(mainLayout);
	setFrameShape(QFrame::NoFrame);
	setLineWidth(0);

	stackedMixerArea = new QStackedWidget(this);
	stackedMixerArea->setObjectName("stackedMixerArea");

	// Horizontal Widgets
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

	hVolumeControlLayout = new QVBoxLayout(hVolumeWidgets);
	hVolumeWidgets->setLayout(hVolumeControlLayout);
	hVolumeControlLayout->setContentsMargins(0, 0, 0, 0);
	hVolumeControlLayout->setSpacing(0);
	hVolumeControlLayout->setAlignment(Qt::AlignTop);

	hMixerScrollArea->setWidget(hVolumeWidgets);

	// Vertical Widgets
	vMixerScrollArea = new QScrollArea(this);
	vMixerScrollArea->setObjectName("vMixerScrollArea");
	vMixerScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	vMixerScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	vMixerScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	vMixerScrollArea->setWidgetResizable(true);
	vMixerScrollArea->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

	vVolumeWidgets = new QWidget(this);

	vVolumeWidgets->setObjectName("vVolumeWidgets");

	vVolumeControlLayout = new QHBoxLayout(vVolumeWidgets);
	vVolumeWidgets->setLayout(vVolumeControlLayout);
	vVolumeControlLayout->setContentsMargins(0, 0, 0, 0);
	vVolumeControlLayout->setSpacing(0);
	vVolumeControlLayout->setAlignment(Qt::AlignLeft);

	vMixerScrollArea->setWidget(vVolumeWidgets);

	stackedMixerArea->addWidget(hMixerScrollArea);
	stackedMixerArea->addWidget(vMixerScrollArea);

	mixerToolbar = new QToolBar(this);
	mixerToolbar->setIconSize(QSize(16, 16));
	mixerToolbar->setFloatable(false);

	mainLayout->addWidget(stackedMixerArea);
	mainLayout->addWidget(mixerToolbar);

	advAudio = new QAction(this);
	advAudio->setText(QTStr("Basic.AdvAudio"));
	advAudio->setToolTip(QTStr("Basic.AdvAudio"));
	QIcon advIcon;
	advIcon.addFile(QString::fromUtf8(":/settings/images/settings/advanced.svg"), QSize(16, 16),
			QIcon::Mode::Normal, QIcon::State::Off);
	advAudio->setIcon(advIcon);
	advAudio->setObjectName("actionMixerToolbarAdvAudio");

	layoutButton = new QAction(this);
	layoutButton->setText("");
	layoutButton->setToolTip(QTStr("Basic.AudioMixer.Layout.Vertical"));
	QIcon layoutIcon;
	layoutIcon.addFile(QString::fromUtf8(":/res/images/layout-vertical.svg"), QSize(16, 16), QIcon::Mode::Normal,
			   QIcon::State::Off);
	layoutButton->setIcon(layoutIcon);
	layoutButton->setObjectName("actionMixerToolbarToggleLayout");

	QWidget *spacer = new QWidget(mixerToolbar);
	spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	optionsButton = new QPushButton(mixerToolbar);
	optionsButton->setText(QTStr("Basic.AudioMixer.Options"));
	idian::Utils::addClass(optionsButton, "toolbar-button");
	idian::Utils::addClass(optionsButton, "text-bold");

	createMixerContextMenu();
	optionsButton->setMenu(mixerMenu);

	toggleHiddenButton = new QPushButton(mixerToolbar);
	toggleHiddenButton->setCheckable(true);
	toggleHiddenButton->setChecked(showHidden);
	toggleHiddenButton->setText(QTStr("Basic.AudioMixer.HiddenTotal").arg(0));
	toggleHiddenButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	QIcon hiddenIcon;
	hiddenIcon.addFile(QString::fromUtf8(":/res/images/hidden.svg"), QSize(16, 16), QIcon::Mode::Normal,
			   QIcon::State::Off);
	toggleHiddenButton->setIcon(hiddenIcon);
	idian::Utils::addClass(toggleHiddenButton, "toolbar-button");
	idian::Utils::addClass(toggleHiddenButton, "toggle-hidden");

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
	idian::Utils::addClass(advAudioWidget, "icon-cogs");

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

	connect(&updateTimer, &QTimer::timeout, this, &AudioMixer::updateVolumeLayouts);
	updateTimer.setSingleShot(true);

	OBSBasic *main = OBSBasic::Get();
	if (main) {
		connect(main, &OBSBasic::userSettingChanged, this,
			[this](const std::string &category, const std::string &name) {
				if (category == "BasicWindow" && name == "VerticalVolumeControl") {
					updateLayout();
				} else if (category == "BasicWindow" && name == "MixerShowInactive") {
					updateShowInactive();
				} else if (category == "BasicWindow" && name == "MixerKeepInactiveLast") {
					updateKeepInactiveLast();
				} else if (category == "BasicWindow" && name == "MixerShowHidden") {
					updateShowHidden();
				} else if (category == "BasicWindow" && name == "MixerKeepHiddenLast") {
					updateKeepHiddenLast();
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
	updatePreviewHandlers();
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

void AudioMixer::updateLayout()
{
	bool vertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolumeControl");
	setMixerLayoutVertical(vertical);

	updateVolumeLayouts();
}

void AudioMixer::toggleShowInactive(bool checked)
{
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "MixerShowInactive", checked);
	OBSBasic *main = OBSBasic::Get();
	if (main) {
		emit main->userSettingChanged("BasicWindow", "MixerShowInactive");
	}
}

void AudioMixer::toggleKeepInactiveLast(bool checked)
{
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "MixerKeepInactiveLast", checked);
	OBSBasic *main = OBSBasic::Get();
	if (main) {
		emit main->userSettingChanged("BasicWindow", "MixerKeepInactiveLast");
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

void AudioMixer::toggleKeepHiddenLast(bool checked)
{
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "MixerKeepHiddenLast", checked);
	OBSBasic *main = OBSBasic::Get();
	if (main) {
		emit main->userSettingChanged("BasicWindow", "MixerKeepHiddenLast");
	}
}

VolumeControl *AudioMixer::createVolumeControl(obs_source_t *source)
{
	bool vertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolumeControl");
	VolumeControl *control = new VolumeControl(source, this, vertical);

	control->setGlobalInMixer(isSourceGlobal(source));
	control->setHiddenInMixer(isHiddenInMixer(source));
	control->setPinnedInMixer(isPinnedInMixer(source));

	control->enableSlider(!isVolumeLocked(source));

	OBSBasic *main = OBSBasic::Get();

	double meterDecayRate = config_get_double(main->Config(), "Audio", "MeterDecayRate");
	control->setMeterDecayRate(meterDecayRate);

	uint64_t peakMeterTypeIdx = config_get_uint(main->Config(), "Audio", "PeakMeterType");

	obs_peak_meter_type peakMeterType;
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

	control->setPeakMeterType(peakMeterType);

	connect(control, &VolumeControl::unhideAll, this, &AudioMixer::unhideAllAudioControls);

	return control;
}

void AudioMixer::updateControlVisibility(QString uuid)
{
	auto item = volumeList.find(uuid);
	if (item == volumeList.end()) {
		return;
	}

	VolumeControl *control = item->second;
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
	updatePreviewSources();
	updateGlobalSources();
}

void AudioMixer::sourceRemoved(QString uuid)
{
	removeControlForUuid(uuid);
	updatePreviewSources();
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

		auto getPreviewSources = [this](obs_scene_t *, obs_sceneitem_t *item) {
			if (!obs_sceneitem_visible(item)) {
				return true;
			}

			obs_source_t *source = obs_sceneitem_get_source(item);
			if (!source) {
				return true;
			}

			uint32_t flags = obs_source_get_output_flags(source);
			if ((flags & OBS_SOURCE_AUDIO) == 0) {
				return true;
			}

			auto uuidPointer = obs_source_get_uuid(source);
			if (uuidPointer && *uuidPointer) {
				previewSources.insert(QString::fromUtf8(uuidPointer));
			}

			return true;
		};

		using getPreviewSources_t = decltype(getPreviewSources);

		auto previewEnum = [](obs_scene_t *scene, obs_sceneitem_t *item, void *data) -> bool {
			return (*static_cast<getPreviewSources_t *>(data))(scene, item);
		};

		obs_scene_enum_items(previewScene, previewEnum, &getPreviewSources);
	}
}

void AudioMixer::updateGlobalSources()
{
	globalSources.clear();

	for (int i = 1; i <= GLOBAL_SOURCE_TOTAL; i++) {
		OBSSourceAutoRelease source = obs_get_output_source(i);
		if (source) {
			auto uuidPointer = obs_source_get_uuid(source);
			if (uuidPointer && *uuidPointer) {
				globalSources.insert(QString::fromUtf8(uuidPointer));
			}
		}
	}

	queueLayoutUpdate();
}

QBoxLayout *AudioMixer::activeLayout() const
{
	bool vertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolumeControl");

	QBoxLayout *layout = vertical ? static_cast<QBoxLayout *>(vVolumeControlLayout)
				      : static_cast<QBoxLayout *>(hVolumeControlLayout);

	return layout;
}

void AudioMixer::reloadVolumeControls()
{
	clearVolumeControls();

	auto createMixerControls = [](void *param, obs_source_t *source) -> bool {
		AudioMixer *mixer = static_cast<AudioMixer *>(param);

		uint32_t flags = obs_source_get_output_flags(source);

		if ((flags & OBS_SOURCE_AUDIO) == 0) {
			return true;
		}

		auto uuidPointer = obs_source_get_uuid(source);
		if (!uuidPointer || !*uuidPointer) {
			return true;
		}

		mixer->addControlForUuid(QString::fromUtf8(uuidPointer));
		return true;
	};

	obs_enum_sources(createMixerControls, this);

	queueLayoutUpdate();
}

bool AudioMixer::getMixerVisibilityForControl(VolumeControl *control)
{
	bool isPinned = control->mixerStatus().has(VolumeControl::MixerStatus::Pinned);
	bool isPreviewed = control->mixerStatus().has(VolumeControl::MixerStatus::Preview);
	bool isHidden = control->mixerStatus().has(VolumeControl::MixerStatus::Hidden);
	bool isAudioActive = control->mixerStatus().has(VolumeControl::MixerStatus::Active);

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

	auto uuidPointer = obs_source_get_uuid(source);
	if (!uuidPointer || !*uuidPointer) {
		return false;
	}

	if (previewSources.find(QString::fromUtf8(uuidPointer)) != previewSources.end()) {
		return true;
	}

	return false;
}

bool AudioMixer::isSourceGlobal(obs_source_t *source)
{
	if (!source) {
		return false;
	}

	auto uuidPointer = obs_source_get_uuid(source);
	if (!uuidPointer || !*uuidPointer) {
		return false;
	}

	if (globalSources.find(QString::fromUtf8(uuidPointer)) != globalSources.end()) {
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
		control->setHiddenInMixer(false);
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
	bool vertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolumeControl");

	std::vector<RankedVolume> rankedVolumes;
	rankedVolumes.reserve(volumeList.size());
	for (const auto &entry : volumeList) {
		VolumeControl *control = entry.second;
		if (control) {
			int sortingWeight = 0;

			OBSSource source = OBSGetStrongRef(control->weakSource());
			if (!source) {
				const char *cachedName = control->getCachedName().toUtf8().constData();
				blog(LOG_INFO, "Tried to sort VolumeControl for '%s' but source is null", cachedName);
				continue;
			}

			bool isPreviewed = isSourcePreviewed(source);
			bool isGlobal = isSourceGlobal(source);
			bool isPinned = isPinnedInMixer(source);
			bool isHidden = isHiddenInMixer(source);
			bool isAudioActive = isSourceAudioActive(source);
			bool isLocked = isVolumeLocked(source);

			control->mixerStatus().set(VolumeControl::MixerStatus::Preview, isPreviewed);
			control->mixerStatus().set(VolumeControl::MixerStatus::Global, isGlobal);
			control->mixerStatus().set(VolumeControl::MixerStatus::Pinned, isPinned);
			control->mixerStatus().set(VolumeControl::MixerStatus::Hidden, isHidden);
			control->mixerStatus().set(VolumeControl::MixerStatus::Active, isAudioActive);
			control->mixerStatus().set(VolumeControl::MixerStatus::Locked, isLocked);

			if (isHidden) {
				hiddenCount += 1;
			}

			if (!isGlobal) {
				sortingWeight += 20;
			}

			if (!isPinned) {
				sortingWeight += 20;
			}

			if (isHidden && keepHiddenLast) {
				sortingWeight += 20;

				if (isPreviewed) {
					sortingWeight -= 10;
				}
			}

			if (!isAudioActive && keepInactiveLast) {
				sortingWeight += 50;

				if (isPreviewed) {
					sortingWeight -= 10;
				}
			}

			rankedVolumes.push_back({control, sortingWeight});
		}
	}

	std::sort(rankedVolumes.begin(), rankedVolumes.end(), [](const RankedVolume &a, const RankedVolume &b) {
		const QString &nameA = a.control->getCachedName();
		const QString &nameB = b.control->getCachedName();

		if (a.sortingWeight == b.sortingWeight) {
			return nameA.toLower() < nameB.toLower();
		}

		return a.sortingWeight < b.sortingWeight;
	});

	VolumeControl *prevControl = nullptr;
	int index = 0;
	QBoxLayout *layout = activeLayout();

	vMixerScrollArea->setWidgetResizable(false);
	hMixerScrollArea->setWidgetResizable(false);

	QSize minimumSize{};
	for (const auto &entry : rankedVolumes) {
		VolumeControl *volControl = entry.control;
		if (!volControl) {
			continue;
		}

		layout->insertWidget(index, volControl);
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

		if (!minimumSize.isValid()) {
			minimumSize = volControl->minimumSizeHint();
		}

		++index;
	}

	toggleHiddenButton->setText(QTStr("Basic.AudioMixer.HiddenTotal").arg(hiddenCount));
	if (hiddenCount == 0) {
		toggleHiddenButton->setDisabled(true);
		idian::Utils::toggleClass(toggleHiddenButton, "text-muted", true);
	} else {
		toggleHiddenButton->setDisabled(false);
		idian::Utils::toggleClass(toggleHiddenButton, "text-muted", false);
	}

	vMixerScrollArea->setWidgetResizable(true);
	hMixerScrollArea->setWidgetResizable(true);

	int scrollBarSize = QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
	stackedMixerArea->setMinimumSize(minimumSize.width() + scrollBarSize, minimumSize.height() + scrollBarSize);

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
		stackedMixerArea->setCurrentIndex(1);

		QIcon layoutIcon;
		layoutIcon.addFile(QString::fromUtf8(":/res/images/layout-horizontal.svg"), QSize(16, 16),
				   QIcon::Mode::Normal, QIcon::State::Off);
		layoutButton->setIcon(layoutIcon);
		layoutButton->setToolTip(QTStr("Basic.AudioMixer.Layout.Horizontal"));
	} else {
		stackedMixerArea->setCurrentIndex(0);

		QIcon layoutIcon;
		layoutIcon.addFile(QString::fromUtf8(":/res/images/layout-vertical.svg"), QSize(16, 16),
				   QIcon::Mode::Normal, QIcon::State::Off);
		layoutButton->setIcon(layoutIcon);
		layoutButton->setToolTip(QTStr("Basic.AudioMixer.Layout.Vertical"));
	}

	// Qt caches the size of QWidgetActions so this is the simplest way to update the text
	// of the checkboxes in the menu and make sure the menu size is recalculated.
	createMixerContextMenu();

	QWidget *buttonWidget = mixerToolbar->widgetForAction(layoutButton);
	if (buttonWidget) {
		idian::Utils::toggleClass(buttonWidget, "icon-layout-horizontal", vertical);
		idian::Utils::toggleClass(buttonWidget, "icon-layout-vertical", !vertical);
	}
}

void AudioMixer::createMixerContextMenu()
{
	if (mixerMenu) {
		mixerMenu->deleteLater();
	}

	mixerMenu = new QMenu(this);

	// Create menu actions
	QAction *unhideAllAction = new QAction(QTStr("UnhideAll"), mixerMenu);

	showHiddenCheckBox = new MenuCheckBox(QTStr("Basic.AudioMixer.ShowHidden"), mixerMenu);
	QWidgetAction *showHiddenAction = new QWidgetAction(mixerMenu);
	showHiddenCheckBox->setAction(showHiddenAction);
	showHiddenCheckBox->setChecked(showHidden);
	showHiddenAction->setDefaultWidget(showHiddenCheckBox);

	QWidgetAction *showInactiveAction = new QWidgetAction(mixerMenu);
	MenuCheckBox *showInactiveCheckBox = new MenuCheckBox(QTStr("Basic.AudioMixer.ShowInactive"), mixerMenu);
	showInactiveCheckBox->setAction(showInactiveAction);
	showInactiveCheckBox->setChecked(showInactive);
	showInactiveAction->setDefaultWidget(showInactiveCheckBox);

	QWidgetAction *hiddenLastAction = new QWidgetAction(mixerMenu);
	const char *hiddenLastString = mixerVertical ? "Basic.AudioMixer.KeepHiddenRight"
						     : "Basic.AudioMixer.KeepHiddenBottom";
	MenuCheckBox *hiddenLastCheckBox = new MenuCheckBox(QTStr(hiddenLastString), mixerMenu);
	hiddenLastCheckBox->setAction(hiddenLastAction);
	hiddenLastCheckBox->setChecked(keepHiddenLast);
	hiddenLastAction->setDefaultWidget(hiddenLastCheckBox);

	QWidgetAction *inactiveLastAction = new QWidgetAction(mixerMenu);
	const char *inactiveLastString = mixerVertical ? "Basic.AudioMixer.KeepInactiveRight"
						       : "Basic.AudioMixer.KeepInactiveBottom";
	MenuCheckBox *inactiveLastCheckBox = new MenuCheckBox(QTStr(inactiveLastString), mixerMenu);
	inactiveLastCheckBox->setAction(inactiveLastAction);
	inactiveLastCheckBox->setChecked(keepInactiveLast);
	inactiveLastAction->setDefaultWidget(inactiveLastCheckBox);

	// Connect menu actions
	connect(unhideAllAction, &QAction::triggered, this, &AudioMixer::unhideAllAudioControls, Qt::DirectConnection);

	connect(showHiddenCheckBox, &QCheckBox::toggled, this, &AudioMixer::toggleShowHidden, Qt::DirectConnection);
	connect(hiddenLastCheckBox, &QCheckBox::toggled, this, &AudioMixer::toggleKeepHiddenLast, Qt::DirectConnection);

	connect(showInactiveCheckBox, &QCheckBox::toggled, this, &AudioMixer::toggleShowInactive, Qt::DirectConnection);
	connect(inactiveLastCheckBox, &QCheckBox::toggled, this, &AudioMixer::toggleKeepInactiveLast,
		Qt::DirectConnection);

	// Build menu and show
	mixerMenu->addAction(unhideAllAction);
	mixerMenu->addSeparator();
	mixerMenu->addAction(showHiddenAction);
	mixerMenu->addAction(showInactiveAction);
	mixerMenu->addAction(hiddenLastAction);
	mixerMenu->addAction(inactiveLastAction);

	optionsButton->setMenu(mixerMenu);
}

void AudioMixer::showMixerContextMenu()
{
	createMixerContextMenu();
	mixerMenu->popup(QCursor::pos());
}

void AudioMixer::addControlForUuid(QString uuid)
{
	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toUtf8().constData());

	QPointer<VolumeControl> newControl = createVolumeControl(source);

	volumeList.insert({uuid, newControl});
	queueLayoutUpdate();
}

void AudioMixer::removeControlForUuid(QString uuid)
{
	auto item = volumeList.find(uuid);
	if (item != volumeList.end()) {
		VolumeControl *widget = item->second;
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
	case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:
	case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:
		updatePreviewSources();
		updatePreviewHandlers();
		queueLayoutUpdate();
		break;
	case OBS_FRONTEND_EVENT_EXIT:
		obs_frontend_remove_event_callback(AudioMixer::onFrontendEvent, this);
		break;
	default:
		break;
	}
}

void AudioMixer::updatePreviewHandlers()
{
	previewSignals.clear();

	bool isStudioMode = obs_frontend_preview_program_mode_active();
	if (isStudioMode) {
		OBSSourceAutoRelease previewSource = obs_frontend_get_current_preview_scene();
		if (!previewSource) {
			return;
		}

		previewSignals.reserve(1);

		previewSignals.emplace_back(obs_source_get_signal_handler(previewSource), "item_visible",
					    AudioMixer::obsSceneItemVisibleChange, this);
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

void AudioMixer::updateKeepInactiveLast()
{
	bool settingKeepInactiveLast = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerKeepInactiveLast");
	if (keepInactiveLast == settingKeepInactiveLast) {
		return;
	}

	keepInactiveLast = settingKeepInactiveLast;

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
	showHiddenCheckBox->setChecked(showHidden);

	queueLayoutUpdate();
}

void AudioMixer::updateKeepHiddenLast()
{
	bool settingKeepHiddenLast = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MixerKeepHiddenLast");
	if (keepHiddenLast == settingKeepHiddenLast) {
		return;
	}

	keepHiddenLast = settingKeepHiddenLast;

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
	obs_source_t *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO) {
		auto uuidPointer = obs_source_get_uuid(source);
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "updateControlVisibility",
					  Qt::QueuedConnection, Q_ARG(QString, QString::fromUtf8(uuidPointer)));
	}
}

void AudioMixer::obsSourceDeactivated(void *data, calldata_t *params)
{
	obs_source_t *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO) {
		auto uuidPointer = obs_source_get_uuid(source);
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "updateControlVisibility",
					  Qt::QueuedConnection, Q_ARG(QString, QString::fromUtf8(uuidPointer)));
	}
}

void AudioMixer::obsSourceAudioActivated(void *data, calldata_t *params)
{
	obs_source_t *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));

	if (obs_source_active(source)) {
		auto uuidPointer = obs_source_get_uuid(source);
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "updateControlVisibility",
					  Qt::QueuedConnection, Q_ARG(QString, QString::fromUtf8(uuidPointer)));
	}
}

void AudioMixer::obsSourceAudioDeactivated(void *data, calldata_t *params)
{
	obs_source_t *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));

	auto uuidPointer = obs_source_get_uuid(source);
	QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "updateControlVisibility", Qt::QueuedConnection,
				  Q_ARG(QString, QString::fromUtf8(uuidPointer)));
}

void AudioMixer::obsSourceCreate(void *data, calldata_t *params)
{
	obs_source_t *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO) {
		auto uuidPointer = obs_source_get_uuid(source);
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "sourceCreated", Qt::QueuedConnection,
					  Q_ARG(QString, QString::fromUtf8(uuidPointer)));
	}
}

void AudioMixer::obsSourceRemove(void *data, calldata_t *params)
{
	obs_source_t *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO) {
		auto uuidPointer = obs_source_get_uuid(source);
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "sourceRemoved", Qt::QueuedConnection,
					  Q_ARG(QString, QString::fromUtf8(uuidPointer)));
	}
}

void AudioMixer::obsSourceRename(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "queueLayoutUpdate", Qt::QueuedConnection);
}

void AudioMixer::obsSceneItemVisibleChange(void *data, calldata_t *params)
{
	obs_sceneitem_t *sceneItem = static_cast<obs_sceneitem_t *>(calldata_ptr(params, "item"));
	if (!sceneItem) {
		return;
	}

	obs_source_t *source = obs_sceneitem_get_source(sceneItem);
	if (!source) {
		return;
	}

	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO) {
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "updatePreviewSources",
					  Qt::QueuedConnection);

		auto uuidPointer = obs_source_get_uuid(source);
		QMetaObject::invokeMethod(static_cast<AudioMixer *>(data), "updateControlVisibility",
					  Qt::QueuedConnection, Q_ARG(QString, QString::fromUtf8(uuidPointer)));
	}
}
