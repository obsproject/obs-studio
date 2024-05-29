#pragma once
#include <vector>
#include <obs.hpp>

class MultiviewAudioMeter {
public:
	MultiviewAudioMeter();
	~MultiviewAudioMeter();
	// Volume printing
	float currentMagnitude[MAX_AUDIO_CHANNELS];
	void RenderAudioMeter(uint32_t labelColor, float sourceX, float sourceY,
			      float ppiCX, float ppiCY, float ppiScaleX,
			      float ppiScaleY);
	void ConnectAudioOutput(int selectedNewAudio);

private:
	float minimumLevel;
	int selectedAudio;
	int selectedTrackIndex;
	std::vector<OBSSource> scaleLabels;

	// argb colors
	static const uint32_t backgroundNominalColor = 0xFF267F26; // Dark green
	static const uint32_t backgroundWarningColor =
		0xFF7F7F26;                                      // Dark yellow
	static const uint32_t backgroundErrorColor = 0xFF7F2626; // Dark red
	static const uint32_t foregroundNominalColor =
		0xFF4CFF4C; // Bright green
	static const uint32_t foregroundWarningColor =
		0xFFFFFF4C; // Bright yellow
	static const uint32_t foregroundErrorColor =
		0xFFFF4C4C; // Bright red	// Volume printing

	static void OBSOutputVolumeLevelChanged(void *param, size_t mix_idx,
						struct audio_data *data);
	inline int convertToInt(float number);
	void CreateLabels();
	void DrawScale(int indexFromLast, float xCoordinate, float yCoordinate,
		       float ppiScaleX, float ppiScaleY);
	OBSSource CreateLabel(const char *name, size_t h);
};
