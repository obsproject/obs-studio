#include "common-settings.hpp"

#include "audio-encoders.hpp"

static bool IsAdvancedMode(config_t *config)
{
	const char *outputMode = config_get_string(config, "Output", "Mode");
	return (strcmp(outputMode, "Advanced") == 0);
}

OBSData CommonSettings::GetDataFromJsonFile(const char *jsonFile)
{
	char fullPath[512];
	obs_data_t *data = nullptr;

	int ret = GetProfilePath(fullPath, sizeof(fullPath), jsonFile);
	if (ret > 0) {
		BPtr<char> jsonData = os_quick_read_utf8_file(fullPath);
		if (jsonData) {
			data = obs_data_create_from_json(jsonData);
		}
	}

	if (!data)
		data = obs_data_create();
	OBSData dataRet(data);
	obs_data_release(data);
	return dataRet;
}

void CommonSettings::GetConfigFPS(config_t *config, uint32_t &num,
				  uint32_t &den)
{
	uint32_t type = config_get_uint(config, videoSection, "FPSType");

	if (type == 1) //"Integer"
		GetFPSInteger(config, num, den);
	else if (type == 2) //"Fraction"
		GetFPSFraction(config, num, den);
	else if (false) //"Nanoseconds", currently not implemented
		GetFPSNanoseconds(config, num, den);
	else
		GetFPSCommon(config, num, den);
}

double CommonSettings::GetConfigFPSDouble(config_t *config)
{
	uint32_t num = 0;
	uint32_t den = 0;
	CommonSettings::GetConfigFPS(config, num, den);
	return (double)num / (double)den;
}

void CommonSettings::GetFPSCommon(config_t *config, uint32_t &num,
				  uint32_t &den)
{
	const char *val = config_get_string(config, videoSection, "FPSCommon");

	if (strcmp(val, "10") == 0) {
		num = 10;
		den = 1;
	} else if (strcmp(val, "20") == 0) {
		num = 20;
		den = 1;
	} else if (strcmp(val, "24 NTSC") == 0) {
		num = 24000;
		den = 1001;
	} else if (strcmp(val, "25 PAL") == 0) {
		num = 25;
		den = 1;
	} else if (strcmp(val, "29.97") == 0) {
		num = 30000;
		den = 1001;
	} else if (strcmp(val, "48") == 0) {
		num = 48;
		den = 1;
	} else if (strcmp(val, "50 PAL") == 0) {
		num = 50;
		den = 1;
	} else if (strcmp(val, "59.94") == 0) {
		num = 60000;
		den = 1001;
	} else if (strcmp(val, "60") == 0) {
		num = 60;
		den = 1;
	} else {
		num = 30;
		den = 1;
	}
}

void CommonSettings::GetFPSInteger(config_t *config, uint32_t &num,
				   uint32_t &den)
{
	num = (uint32_t)config_get_uint(config, videoSection, "FPSInt");
	den = 1;
}

void CommonSettings::GetFPSFraction(config_t *config, uint32_t &num,
				    uint32_t &den)
{
	num = (uint32_t)config_get_uint(config, videoSection, "FPSNum");
	den = (uint32_t)config_get_uint(config, videoSection, "FPSDen");
}

void CommonSettings::GetFPSNanoseconds(config_t *config, uint32_t &num,
				       uint32_t &den)
{
	num = 1000000000;
	den = (uint32_t)config_get_uint(config, videoSection, "FPSNS");
}

int CommonSettings::GetAudioChannelCount(config_t *config)
{
	const char *channelSetup =
		config_get_string(config, "Audio", "ChannelSetup");

	if (strcmp(channelSetup, "Mono") == 0)
		return 1;
	if (strcmp(channelSetup, "Stereo") == 0)
		return 2;
	if (strcmp(channelSetup, "2.1") == 0)
		return 3;
	if (strcmp(channelSetup, "4.0") == 0)
		return 4;
	if (strcmp(channelSetup, "4.1") == 0)
		return 5;
	if (strcmp(channelSetup, "5.1") == 0)
		return 6;
	if (strcmp(channelSetup, "7.1") == 0)
		return 8;

	return 2;
}

int CommonSettings::GetStreamingAudioBitrate(config_t *config)
{
	bool isAdvancedMode = IsAdvancedMode(config);

	if (isAdvancedMode) {
		return GetAdvancedAudioBitrate(config);
	}
	return GetSimpleAudioBitrate(config);
}

int CommonSettings::GetSimpleAudioBitrate(config_t *config)
{
	int bitrate = config_get_uint(config, "SimpleOutput", "ABitrate");
	return FindClosestAvailableAACBitrate(bitrate);
}

int CommonSettings::GetAdvancedAudioBitrate(config_t *config)
{
	int track = config_get_int(config, "AdvOut", "TrackIndex");
	return GetAdvancedAudioBitrateForTrack(config, track - 1);
}

int CommonSettings::GetAdvancedAudioBitrateForTrack(config_t *config,
						    int trackIndex)
{
	static const char *names[] = {
		"Track1Bitrate", "Track2Bitrate", "Track3Bitrate",
		"Track4Bitrate", "Track5Bitrate", "Track6Bitrate",
	};

	// Sanity check for out of bounds, clamp to bounds
	if (trackIndex > 5)
		trackIndex = 5;
	if (trackIndex < 0)
		trackIndex = 0;

	int bitrate = (int)config_get_uint(config, "AdvOut", names[trackIndex]);
	return FindClosestAvailableAACBitrate(bitrate);
}

int CommonSettings::GetVideoBitrateInUse(config_t *config)
{
	if (!IsAdvancedMode(config)) {
		return config_get_int(config, "SimpleOutput", "VBitrate");
	}

	OBSData streamEncSettings = GetDataFromJsonFile("streamEncoder.json");
	int bitrate = obs_data_get_int(streamEncSettings, "bitrate");
	return bitrate;
}
