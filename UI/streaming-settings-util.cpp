#include <streaming-settings-util.hpp>

#include "common-settings.hpp"
#include <util/config-file.h>

#include <QPair>

QSharedPointer<StreamWizard::EncoderSettingsRequest>
StreamingSettingsUtility::makeEncoderSettingsFromCurrentState(config_t *config)
{
	QSharedPointer<StreamWizard::EncoderSettingsRequest> currentSettings(
		new StreamWizard::EncoderSettingsRequest());

	/* Stream info */

	currentSettings->videoType = StreamWizard::VideoType::live;
	// only live and rmpts is supported for now
	currentSettings->protocol = StreamWizard::StreamProtocol::rtmps;
	/* Video */
	currentSettings->videoWidth =
		(int)config_get_uint(config, "Video", "OutputCX");
	currentSettings->videoHeight =
		(int)config_get_uint(config, "Video", "OutputCY");
	currentSettings->framerate = CommonSettings::GetConfigFPSDouble(config);

	currentSettings->videoBitrate =
		CommonSettings::GetVideoBitrateInUse(config);
	currentSettings->videoCodec = StreamWizard::VideoCodec::h264;

	/* Audio */
	currentSettings->audioChannels =
		CommonSettings::GetAudioChannelCount(config);
	currentSettings->audioSamplerate =
		config_get_default_int(config, "Audio", "SampleRate");
	currentSettings->audioBitrate =
		CommonSettings::GetStreamingAudioBitrate(config);
	//For now, only uses AAC
	currentSettings->audioCodec = StreamWizard::AudioCodec::aac;

	return currentSettings;
};

/*
* StreamWizard::SettingsMap is sparse in that it wont have all keys 
* as well, the user can opt out of settings being applied. 
* The map looks like [ kSettingsResponseKeys : QPair<QVariant, bool>  ]
* AKA [KEY : SetttingPair<Setting_Value_Variant, User_Wants_Setting_Bool> ]
* if the key is available, it means the wizard provider added a value for it
* but the bool in the QPair is false if the user selected in the wizard not to 
* apply the setting. A case would be we suggest a 720p stream but the user 
* knows their account supports 1080 so disabled the setting from applying. 

* Returns TRUE is map contrains something for the key and it is marked true
*/
bool CheckInMapAndSelected(StreamWizard::SettingsMap *map, QString key)
{
	if (!map->contains(key)) {
		return false;
	}
	const QPair<QVariant, bool> settingPair = map->value(key);
	return settingPair.second;
}

// Helper Functions for ::applyWizardSettings
int intFromMap(StreamWizard::SettingsMap *map, QString key)
{
	QPair<QVariant, bool> settingPair = map->value(key);
	QVariant data = settingPair.first;
	return data.toInt();
}

QString stringFromMap(StreamWizard::SettingsMap *map, QString key)
{
	QPair<QVariant, bool> settingPair = map->value(key);
	QVariant data = settingPair.first;
	return data.toString();
}

double doubleFromMap(StreamWizard::SettingsMap *map, QString key)
{
	QPair<QVariant, bool> settingPair = map->value(key);
	QVariant data = settingPair.first;
	return data.toDouble();
}

bool boolFromMap(StreamWizard::SettingsMap *map, QString key)
{
	QPair<QVariant, bool> settingPair = map->value(key);
	QVariant data = settingPair.first;
	return data.toBool();
}

/*
* Given a settings map [ kSettingsResponseKeys : QPair<QVariant, bool> ] apply
* settings that are in the sparse map as well selected by the user (which is 
* marked by the bool in the pair).
* Apply to Basic encoder settings.
* Possible later goal: autoconfig advanced settings too
*/
void StreamingSettingsUtility::applyWizardSettings(
	QSharedPointer<StreamWizard::SettingsMap> newSettings, config_t *config)
{

	if (newSettings == nullptr || newSettings.isNull())
		return;

	// scope to function usage
	using namespace StreamWizard;

	SettingsMap *map = newSettings.data();

	QStringList x264SettingList;
	config_set_bool(config, "SimpleOutput", "UseAdvanced", true);

	// Resolution must have both
	if (CheckInMapAndSelected(map, kSettingsResponseKeys.videoHeight) &&
	    CheckInMapAndSelected(map, kSettingsResponseKeys.videoWidth)) {

		int canvasX = intFromMap(map, kSettingsResponseKeys.videoWidth);
		int canvasY =
			intFromMap(map, kSettingsResponseKeys.videoHeight);
		config_set_uint(config, "Video", "OutputCX", canvasX);
		config_set_uint(config, "Video", "OutputCY", canvasY);
	}

	//TODO: FPS is hacky but covers all integer and standard drop frame cases
	if (CheckInMapAndSelected(map, kSettingsResponseKeys.framerate)) {
		double currentFPS = CommonSettings::GetConfigFPSDouble(config);
		double newFPS =
			doubleFromMap(map, kSettingsResponseKeys.framerate);
		if (abs(currentFPS - newFPS) >
		    0.001) { // Only change if different
			if (abs(floor(newFPS) - newFPS) >
			    0.01) { // Is a drop-frame FPS
				int num = ceil(newFPS) * 1000;
				int den = 1001;
				config_set_uint(config, "Video", "FPSType",
						2); // Fraction
				config_set_uint(config, "Video", "FPSNum", num);
				config_set_uint(config, "Video", "FPSDen", den);
			} else { // Is integer FPS
				config_set_uint(config, "Video", "FPSType",
						1); // Integer
				config_set_uint(config, "Video", "FPSInt",
						(int)floor(newFPS));
			}
		}
	}

	if (CheckInMapAndSelected(map, kSettingsResponseKeys.videoBitrate)) {
		int newBitrate =
			intFromMap(map, kSettingsResponseKeys.videoBitrate);
		config_set_int(config, "SimpleOutput", "VBitrate", newBitrate);
	}

	if (CheckInMapAndSelected(map, kSettingsResponseKeys.h264Profile)) {
		QString profile =
			stringFromMap(map, kSettingsResponseKeys.h264Profile);
		profile = profile.toLower();
		x264SettingList.append("profile=" + profile);
	}

	if (CheckInMapAndSelected(map, kSettingsResponseKeys.h264Level)) {
		QString levelString =
			stringFromMap(map, kSettingsResponseKeys.h264Level);
		x264SettingList.append("level=" + levelString);
	}

	if (CheckInMapAndSelected(map, kSettingsResponseKeys.gopSizeInFrames)) {
		int gopSize =
			intFromMap(map, kSettingsResponseKeys.gopSizeInFrames);
		x264SettingList.append("keyint=" + QString::number(gopSize));
	}

	if (CheckInMapAndSelected(map, kSettingsResponseKeys.gopClosed)) {
		bool gopClose =
			boolFromMap(map, kSettingsResponseKeys.gopClosed);
		if (gopClose) {
			x264SettingList.append("open_gop=0");
		} else {
			x264SettingList.append("open_gop=1");
		}
	}

	if (CheckInMapAndSelected(map, kSettingsResponseKeys.gopBFrames)) {
		int bFrames = intFromMap(map, kSettingsResponseKeys.gopBFrames);
		x264SettingList.append("bframes=" + QString::number(bFrames));
	}

	if (CheckInMapAndSelected(map, kSettingsResponseKeys.gopRefFrames)) {
		int refFrames =
			intFromMap(map, kSettingsResponseKeys.gopRefFrames);
		x264SettingList.append("ref=" + QString::number(refFrames));
	}

	// kSettingsResponseKeys.streamRateControlMode defaults to CBR in Simple
	// encoder mode. Can add later for advanced panel.

	if (CheckInMapAndSelected(map,
				  kSettingsResponseKeys.streamBufferSize)) {
		int bufferSize =
			intFromMap(map, kSettingsResponseKeys.streamBufferSize);
		x264SettingList.append("vbv-bufsize=" +
				       QString::number(bufferSize));
	}

	std::string x264String =
		x264SettingList.join(" ").toUtf8().toStdString();
	const char *x264_c_String = x264String.c_str();
	config_set_string(config, "SimpleOutput", "x264Settings",
			  x264_c_String);
};
