/*
  Helper to get common video and audio setting that are accessed from more than
  one file. 
*/
#pragma once

#include <obs.hpp>
#include "obs-app.hpp"

class CommonSettings {

public:
	/* Shared Utility Functions --------------------------*/
	static OBSData GetDataFromJsonFile(const char *jsonFile);

	/* Framerate ----------------------------------------*/
	static void GetConfigFPS(config_t *config, uint32_t &num,
				 uint32_t &den);
	static double GetConfigFPSDouble(config_t *config);

	/* Audio Data ---------------------------------------*/
	// Returns int of audio, sub (the .1 in 2.1) is a channel i.e., 2.1 -> 3ch
	static int GetAudioChannelCount(config_t *config);
	// Gets streaming track's bitrate, simple or advanced mode
	static int GetStreamingAudioBitrate(config_t *config);
	static int GetSimpleAudioBitrate(config_t *config);
	static int GetAdvancedAudioBitrate(config_t *config);
	// Advanced setting's streaming bitrate for track number (starts at 1)
	static int GetAdvancedAudioBitrateForTrack(config_t *config,
						   int trackIndex);

	/* Stream Encoder ——————————————————————————————————*/
	static int GetVideoBitrateInUse(config_t *config);

private:
	// Reused Strings
	static constexpr const char *videoSection = "Video";

	static void GetFPSCommon(config_t *config, uint32_t &num,
				 uint32_t &den);
	static void GetFPSInteger(config_t *config, uint32_t &num,
				  uint32_t &den);
	static void GetFPSFraction(config_t *config, uint32_t &num,
				   uint32_t &den);
	static void GetFPSNanoseconds(config_t *config, uint32_t &num,
				      uint32_t &den);
};
