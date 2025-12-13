#include "SettingsViewModel.h"
#include <QDebug>

SettingsViewModel::SettingsViewModel(QObject *parent) : QObject(parent)
{
	loadDefaults();
}

SettingsViewModel::~SettingsViewModel() {}

void SettingsViewModel::loadDefaults()
{
	// Mock Default Settings

	// Video
	m_video = {{"baseRenderingResolution", "1920x1080"},
		   {"outputScaledResolution", "1920x1080"},
		   {"fpsCommon", 60},
		   {"scaleFilter", "Bicubic"}};

	// Output
	m_output = {{"streamingBitrate", 6000},
		    {"encoder", "NVIDIA NVENC H.264"},
		    {"recordingPath", "/home/subtomic/Videos"},
		    {"recordingFormat", "mkv"}};

	// Stream
	m_stream = {{"service", "Twitch"}, {"server", "Auto"}, {"key", "live_xxxx_xxxx"}};

	// Audio
	m_audio = {{"sampleRate", "48khz"}, {"channels", "Stereo"}, {"micDevice", "Default"}};

	// General
	m_general = {{"language", "English"}, {"theme", "Dark"}};
}

void SettingsViewModel::updateSetting(const QString &category, const QString &key, const QVariant &value)
{
	if (category == "general") {
		m_general[key] = value;
		emit generalSettingsChanged();
	} else if (category == "stream") {
		m_stream[key] = value;
		emit streamSettingsChanged();
	} else if (category == "output") {
		m_output[key] = value;
		emit outputSettingsChanged();
	} else if (category == "audio") {
		m_audio[key] = value;
		emit audioSettingsChanged();
	} else if (category == "video") {
		m_video[key] = value;
		emit videoSettingsChanged();
	}
}

void SettingsViewModel::applyChanges()
{
	qDebug() << "Applying Settings Changes...";
	// Here we would call libobs API:
	// obs_data_set_string(video_settings, "base_resolution", ...);
	// obs_reset_video(obs_video_info);
}

void SettingsViewModel::reset()
{
	loadDefaults();
	emit generalSettingsChanged();
	emit streamSettingsChanged();
	emit outputSettingsChanged();
	emit audioSettingsChanged();
	emit videoSettingsChanged();
}
