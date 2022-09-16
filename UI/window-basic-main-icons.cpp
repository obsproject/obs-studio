#include "window-basic-main.hpp"
#include "icons.hpp"

QIcon OBSBasic::GetSourceIcon(const char *id)
{
	bool colored = IconColorStyled();

	if (strcmp(id, "scene") == 0)
		return colored ? GetIcon(":/res/images/sources/scene.svg")
			       : GetSceneIcon();
	else if (strcmp(id, "group") == 0)
		return colored ? GetIcon(":/res/images/sources/group.svg")
			       : GetGroupIcon();

	obs_icon_type type = obs_source_get_icon_type(id);

	switch (type) {
	case OBS_ICON_TYPE_IMAGE:
		return colored ? GetIcon(":/res/images/sources/image.svg")
			       : GetImageIcon();
	case OBS_ICON_TYPE_COLOR:
		return colored ? GetIcon(":/res/images/sources/brush.svg")
			       : GetColorIcon();
	case OBS_ICON_TYPE_SLIDESHOW:
		return colored ? GetIcon(":/res/images/sources/slideshow.svg")
			       : GetSlideshowIcon();
	case OBS_ICON_TYPE_AUDIO_INPUT:
		return colored ? GetIcon(":/res/images/sources/microphone.svg")
			       : GetAudioInputIcon();
	case OBS_ICON_TYPE_AUDIO_OUTPUT:
		return colored ? GetIcon(":/settings/images/settings/audio.svg")
			       : GetAudioOutputIcon();
	case OBS_ICON_TYPE_DESKTOP_CAPTURE:
		return colored ? GetIcon(":/settings/images/settings/video.svg")
			       : GetDesktopCapIcon();
	case OBS_ICON_TYPE_WINDOW_CAPTURE:
		return colored ? GetIcon(":/res/images/sources/window.svg")
			       : GetWindowCapIcon();
	case OBS_ICON_TYPE_GAME_CAPTURE:
		return colored ? GetIcon(":/res/images/sources/gamepad.svg")
			       : GetGameCapIcon();
	case OBS_ICON_TYPE_CAMERA:
		return colored ? GetIcon(":/res/images/sources/camera.svg")
			       : GetCameraIcon();
	case OBS_ICON_TYPE_TEXT:
		return colored ? GetIcon(":/res/images/sources/text.svg")
			       : GetTextIcon();
	case OBS_ICON_TYPE_MEDIA:
		return colored ? GetIcon(":/res/images/sources/media.svg")
			       : GetMediaIcon();
	case OBS_ICON_TYPE_BROWSER:
		return colored ? GetIcon(":/res/images/sources/globe.svg")
			       : GetBrowserIcon();
	case OBS_ICON_TYPE_CUSTOM:
		//TODO: Add ability for sources to define custom icons
		return colored ? GetIcon(":/res/images/sources/default.svg")
			       : GetDefaultIcon();
	case OBS_ICON_TYPE_PROCESS_AUDIO_OUTPUT:
		return colored ? GetIcon(":/res/images/sources/windowaudio.svg")
			       : GetAudioProcessOutputIcon();
	default:
		return colored ? GetIcon(":/res/images/sources/default.svg")
			       : GetDefaultIcon();
	}
}

void OBSBasic::SetImageIcon(const QIcon &icon)
{
	imageIcon = icon;
}

void OBSBasic::SetColorIcon(const QIcon &icon)
{
	colorIcon = icon;
}

void OBSBasic::SetSlideshowIcon(const QIcon &icon)
{
	slideshowIcon = icon;
}

void OBSBasic::SetAudioInputIcon(const QIcon &icon)
{
	audioInputIcon = icon;
}

void OBSBasic::SetAudioOutputIcon(const QIcon &icon)
{
	audioOutputIcon = icon;
}

void OBSBasic::SetDesktopCapIcon(const QIcon &icon)
{
	desktopCapIcon = icon;
}

void OBSBasic::SetWindowCapIcon(const QIcon &icon)
{
	windowCapIcon = icon;
}

void OBSBasic::SetGameCapIcon(const QIcon &icon)
{
	gameCapIcon = icon;
}

void OBSBasic::SetCameraIcon(const QIcon &icon)
{
	cameraIcon = icon;
}

void OBSBasic::SetTextIcon(const QIcon &icon)
{
	textIcon = icon;
}

void OBSBasic::SetMediaIcon(const QIcon &icon)
{
	mediaIcon = icon;
}

void OBSBasic::SetBrowserIcon(const QIcon &icon)
{
	browserIcon = icon;
}

void OBSBasic::SetGroupIcon(const QIcon &icon)
{
	groupIcon = icon;
}

void OBSBasic::SetSceneIcon(const QIcon &icon)
{
	sceneIcon = icon;
}

void OBSBasic::SetDefaultIcon(const QIcon &icon)
{
	defaultIcon = icon;
}

void OBSBasic::SetAudioProcessOutputIcon(const QIcon &icon)
{
	audioProcessOutputIcon = icon;
}

void OBSBasic::SetIconColor(const QColor &color)
{
	iconColor = color;
	SetIconColorStyled(true);
	SetIconColorInternal(color.name());
}

void OBSBasic::SetIconDisabledColor(const QColor &color)
{
	iconColorDisabled = color;
	SetIconDisabledColorStyled(true);
	SetIconDisabledColorInternal(color.name());
}

QIcon OBSBasic::GetImageIcon() const
{
	return imageIcon;
}

QIcon OBSBasic::GetColorIcon() const
{
	return colorIcon;
}

QIcon OBSBasic::GetSlideshowIcon() const
{
	return slideshowIcon;
}

QIcon OBSBasic::GetAudioInputIcon() const
{
	return audioInputIcon;
}

QIcon OBSBasic::GetAudioOutputIcon() const
{
	return audioOutputIcon;
}

QIcon OBSBasic::GetDesktopCapIcon() const
{
	return desktopCapIcon;
}

QIcon OBSBasic::GetWindowCapIcon() const
{
	return windowCapIcon;
}

QIcon OBSBasic::GetGameCapIcon() const
{
	return gameCapIcon;
}

QIcon OBSBasic::GetCameraIcon() const
{
	return cameraIcon;
}

QIcon OBSBasic::GetTextIcon() const
{
	return textIcon;
}

QIcon OBSBasic::GetMediaIcon() const
{
	return mediaIcon;
}

QIcon OBSBasic::GetBrowserIcon() const
{
	return browserIcon;
}

QIcon OBSBasic::GetGroupIcon() const
{
	return groupIcon;
}

QIcon OBSBasic::GetSceneIcon() const
{
	return sceneIcon;
}

QIcon OBSBasic::GetDefaultIcon() const
{
	return defaultIcon;
}

QIcon OBSBasic::GetAudioProcessOutputIcon() const
{
	return audioProcessOutputIcon;
}

QColor OBSBasic::GetIconColor() const
{
	return iconColor;
}

QColor OBSBasic::GetIconDisabledColor() const
{
	return iconColorDisabled;
}

void OBSBasic::ResetIcons()
{
	LoadIcons();

	SetIcon(ui->sourceFiltersButton, ":/res/images/filter.svg");
	SetIcon(ui->sourcePropertiesButton,
		":/settings/images/settings/general.svg");
	SetIcon(ui->sourceInteractButton, ":/res/images/interact.svg");

	SetIcon(ui->actionAddScene, ":/res/images/plus.svg");
	SetIcon(ui->actionRemoveScene, ":/res/images/minus.svg");
	SetIcon(ui->actionSceneUp, ":/res/images/up.svg");
	SetIcon(ui->actionSceneDown, ":/res/images/down.svg");
	SetIcon(ui->actionSceneFilters, ":/res/images/filter.svg");

	SetIcon(ui->actionAddSource, ":/res/images/plus.svg");
	SetIcon(ui->actionRemoveSource, ":/res/images/minus.svg");
	SetIcon(ui->actionSourceProperties,
		":/settings/images/settings/general.svg");
	SetIcon(ui->actionSourceUp, ":/res/images/up.svg");
	SetIcon(ui->actionSourceDown, ":/res/images/down.svg");

	SetIcon(ui->actionMixerToolbarAdvAudio, ":/res/images/cogs.svg");
	SetIcon(ui->actionMixerToolbarMenu, ":/res/images/dots-vert.svg");

	SetIcon(ui->transitionAdd, ":/res/images/plus.svg");
	SetIcon(ui->transitionRemove, ":/res/images/minus.svg");
	SetIcon(ui->transitionProps, ":/settings/images/settings/general.svg");

	if (pause)
		SetIcon(pause.data(), ":/res/images/media/media_pause.svg",
			"pauseIconSmall");

	if (vcamButton)
		SetIcon(vcamButton.data()->second(),
			":/settings/images/settings/general.svg",
			"configIconSmall");

	if (replayBufferButton)
		SetIcon(replayBufferButton.data()->second(),
			":/res/images/save.svg", "replayIconSmall");

	ui->sources->UpdateIcons();
}
