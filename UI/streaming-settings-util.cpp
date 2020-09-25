#include <streaming-settings-util.hpp>

#include "common-settings.hpp"
#include <util/config-file.h>

/*
  We are detecting the stream settings so if they're using a large canvas but
  already have the rescaler on, they will be streaming correctly. 
*/
void UpdateStreamingResolution(int *resolutionXY, bool isRescaled,
			       config_t *config)
{

	// If scaled, that's the stream resolution sent to servers
	if (isRescaled) {
		// Only use if there is a resolution in text
		const char *rescaleRes =
			config_get_string(config, "AdvOut", "RescaleRes");
		if (rescaleRes && *rescaleRes) {
			int count = sscanf(rescaleRes, "%ux%u",
					   &resolutionXY[0], &resolutionXY[1]);
			// If text was valid, exit
			if (count == 2) {
				return;
			}
		}
	}

	// Resolution is not rescaled, use "output resolution" from video tab
	resolutionXY[0] = (int)config_get_uint(config, "Video", "OutputCX");
	resolutionXY[1] = (int)config_get_uint(config, "Video", "OutputCY");
};

QSharedPointer<StreamWizard::EncoderSettingsRequest>
StreamingSettingsUtility::makeEncoderSettingsFromCurrentState(
	config_t *config, obs_data_t *settings)
{
	QSharedPointer<StreamWizard::EncoderSettingsRequest> currentSettings(
		new StreamWizard::EncoderSettingsRequest());

	/* Stream info */
	currentSettings->videoType = StreamWizard::VideoType::live;
	// only live and rmpts is supported for now
	currentSettings->protocol = StreamWizard::StreamProtocol::rtmps;

	currentSettings->serverUrl =
		bstrdup(obs_data_get_string(settings, "server"));
	currentSettings->serviceName =
		bstrdup(obs_data_get_string(settings, "service"));

	/* Video */
	bool isRescaled = config_get_bool(config, "AdvOut", "Rescale");
	int resolutionXY[2] = {0, 0};
	UpdateStreamingResolution(resolutionXY, isRescaled, config);
	currentSettings->videoWidth = resolutionXY[0];
	currentSettings->videoHeight = resolutionXY[1];
	currentSettings->framerate = CommonSettings::GetConfigFPSDouble(config);

	// This ONLY works for simple output right now. If they're in advanced: no
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
