#include "multiviewAudioMeter.hpp"
#include "obs.h"
#include <string>

#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define NUMBER_OF_VOLUME_METER_RECTENGELS 48
#define DRAW_SCALE_NUMBERS_INCREMENT 5
MultiviewAudioMeter::MultiviewAudioMeter()
{
	minimumLevel = -60.0f;
	selectedAudio = 0;
	selectedTrackIndex = 0;
	CreateLabels();
	obs_remove_raw_audio_callback(selectedTrackIndex,
				      OBSOutputVolumeLevelChanged, this);
}

MultiviewAudioMeter::~MultiviewAudioMeter()
{
	obs_remove_raw_audio_callback(selectedTrackIndex,
				      OBSOutputVolumeLevelChanged, this);
}

void MultiviewAudioMeter::ConnectAudioOutput(int selectedNewAudio)
{
	if (selectedAudio != selectedNewAudio) {

		obs_remove_raw_audio_callback(
			selectedTrackIndex, OBSOutputVolumeLevelChanged, this);
		selectedAudio = selectedNewAudio;
		if (selectedAudio & (1 << 0)) {
			selectedTrackIndex = 0;
		} else if (selectedAudio & (1 << 1)) {
			selectedTrackIndex = 1;
		} else if (selectedAudio & (1 << 2)) {
			selectedTrackIndex = 2;
		} else if (selectedAudio & (1 << 3)) {
			selectedTrackIndex = 3;
		} else if (selectedAudio & (1 << 4)) {
			selectedTrackIndex = 4;
		} else if (selectedAudio & (1 << 5)) {
			selectedTrackIndex = 5;
		}
		struct audio_convert_info *arg2 =
			(struct audio_convert_info *)0;

		obs_add_raw_audio_callback(
			selectedTrackIndex,
			(struct audio_convert_info const *)arg2,
			OBSOutputVolumeLevelChanged, this);
	}
}

void MultiviewAudioMeter::RenderAudioMeter(uint32_t labelColor, float sourceX,
					   float sourceY, float ppiCX,
					   float ppiCY, float ppiScaleX,
					   float ppiScaleY)
{

	auto drawBox = [&](float cx, float cy, uint32_t colorVal) {
		gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
		gs_eparam_t *color =
			gs_effect_get_param_by_name(solid, "color");

		gs_effect_set_color(color, colorVal);
		while (gs_effect_loop(solid, "Solid"))
			gs_draw_sprite(nullptr, 0, (uint32_t)cx, (uint32_t)cy);
	};

	struct rectangleForDraw {
		float XPoint;
		float YPoint;
		float Width;
		float Height;
	};

	auto paintAreaWithColor = [&](rectangleForDraw rect, uint32_t color) {
		gs_matrix_push();
		gs_matrix_translate3f(rect.XPoint, rect.YPoint, 0.0f);
		drawBox(rect.Width, rect.Height, color);
		gs_matrix_pop();
	};

	int drawableChannels = 0;
	for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
		if (!isfinite(currentMagnitude[channelNr]))
			continue;
		drawableChannels++;
	}

	//fallback to stereo if no channel found
	if (drawableChannels == 0) {
		drawableChannels = 2;
	}

	float scale = NUMBER_OF_VOLUME_METER_RECTENGELS / minimumLevel;
	float sizeOfRectengles = NUMBER_OF_VOLUME_METER_RECTENGELS;

	//AddSpaceing between rectangles
	sizeOfRectengles += (NUMBER_OF_VOLUME_METER_RECTENGELS - 1) / 2;
	//Add padding
	sizeOfRectengles += 5;
	float yRectSize = ppiCY / sizeOfRectengles;
	float xRectSize = ppiCX / sizeOfRectengles;
	float xCoordinate = sourceX + xRectSize;
	float yCoordinate = sourceY + yRectSize;

	//Select the longest label in the scale
	//--------------------------------------
	int numberOfScaleLabel = convertToInt(
		(minimumLevel * -1 / DRAW_SCALE_NUMBERS_INCREMENT));
	size_t textvectorSize = scaleLabels.size();
	float labelWidth = 0;
	float labelHeight = 0;
	for (int j = numberOfScaleLabel; j >= 0; j--) {
		obs_source_t *decibelLabel =
			scaleLabels[textvectorSize - j - 1];
		if (labelWidth <
		    obs_source_get_width(decibelLabel) * ppiScaleX) {
			labelWidth =
				obs_source_get_width(decibelLabel) * ppiScaleX;
			labelHeight =
				obs_source_get_height(decibelLabel) * ppiScaleY;
		}
	}

	//Draw Background
	//---------------
	rectangleForDraw backRect;
	backRect.XPoint = xCoordinate;
	backRect.YPoint = yCoordinate;
	backRect.Width = xRectSize * drawableChannels +
			 labelWidth * (drawableChannels - 1) + xRectSize;
	backRect.Height = ppiCY - yRectSize * 2;
	gs_matrix_push();
	paintAreaWithColor(backRect, labelColor);
	gs_matrix_pop();

	//Add padding
	xCoordinate += xRectSize / 2;

	//Draw Scale by DRAW_SCALE_NUMBERS_INCREMENT dB -s
	//-------------------------------------------------
	for (int i = 1; i < drawableChannels; i++) {
		float lableX =
			xCoordinate + xRectSize * i + labelWidth * (i - 1);

		float yOffset = yRectSize / 2 + labelHeight / 2;
		float usableY =
			backRect.Height - yRectSize * 3 - labelHeight / 2;
		float pixelIncrementOfScale = usableY / numberOfScaleLabel;
		for (int j = numberOfScaleLabel; j >= 0; j--) {
			float labelY = yCoordinate + yOffset;
			yOffset += pixelIncrementOfScale;
			obs_source_t *curentDecibelLabel =
				scaleLabels[textvectorSize - j - 1];
			float currentLabelWidth =
				obs_source_get_width(curentDecibelLabel) *
				ppiScaleX;
			float xOffset = (labelWidth - currentLabelWidth) / 2;
			DrawScale(j, lableX + xOffset, labelY, ppiScaleX,
				  ppiScaleY);
		}
	}

	//Draw VU meter
	//-------------
	for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
		if (!isfinite(currentMagnitude[channelNr]) &&
		    channelNr >= drawableChannels)
			continue;
		float offsetY = yRectSize * 3;
		yCoordinate = sourceY + ppiCY - offsetY;

		float lastValue = currentMagnitude[channelNr];

		int drawBars = NUMBER_OF_VOLUME_METER_RECTENGELS -
			       convertToInt(lastValue * scale);
		if (drawBars < 0)
			drawBars = 0;

		int indexOfScale = 1;

		for (int i = 0; i < NUMBER_OF_VOLUME_METER_RECTENGELS; i++) {

			double sound = minimumLevel - (i + 1) / scale;
			rectangleForDraw vuRectangle;
			vuRectangle.XPoint = xCoordinate;
			vuRectangle.YPoint = yCoordinate;
			vuRectangle.Width = xRectSize;
			vuRectangle.Height = yRectSize;
			if (i < drawBars) {
				if (sound > -6) {
					paintAreaWithColor(
						vuRectangle,
						foregroundErrorColor);
				} else if (sound > -20) {
					paintAreaWithColor(
						vuRectangle,
						foregroundWarningColor);
				} else {
					paintAreaWithColor(
						vuRectangle,
						foregroundNominalColor);
				}
			} else {
				if (sound > -6) {
					paintAreaWithColor(
						vuRectangle,
						backgroundErrorColor);
				} else if (sound > -20) {
					paintAreaWithColor(
						vuRectangle,
						backgroundWarningColor);
				} else {
					paintAreaWithColor(
						vuRectangle,
						backgroundNominalColor);
				}
			}
			offsetY += yRectSize / 2;
			offsetY += yRectSize;
			yCoordinate = sourceY + ppiCY - offsetY;
		}
		xCoordinate += xRectSize + labelWidth;
	}
}

void MultiviewAudioMeter::DrawScale(int indexFromLast, float placeX,
				    float placeY, float ppiScaleX,
				    float ppiScaleY)
{
	obs_source_t *decibelLabel =
		scaleLabels[scaleLabels.size() - 1 - indexFromLast];
	gs_matrix_push();
	gs_matrix_translate3f(placeX, placeY, 0.0f);
	gs_matrix_scale3f(ppiScaleX, ppiScaleY, 1.0f);
	obs_source_video_render(decibelLabel);
	gs_matrix_pop();
}

void MultiviewAudioMeter::CreateLabels()
{

	//Create Scale label
	//-------------------
	struct obs_video_info ovi;
	obs_get_video_info(&ovi);
	uint32_t h = ovi.base_height;
	for (int deciBells = 0; deciBells >= minimumLevel;
	     deciBells -= DRAW_SCALE_NUMBERS_INCREMENT) {
		char integer_string[4];

		sprintf(integer_string, "%d", deciBells);
		scaleLabels.emplace_back(CreateLabel(integer_string, h / 4));
	}
}
//ListenForOutputLevelChange
void MultiviewAudioMeter::OBSOutputVolumeLevelChanged(void *param,
						      size_t mix_idx,
						      struct audio_data *data)
{
	MultiviewAudioMeter *volControl =
		static_cast<MultiviewAudioMeter *>(param);
	if (data) {

		int nr_channels = 0;
		for (int i = 0; i < MAX_AV_PLANES; i++) {
			if (data->data[i])
				nr_channels++;
		}
		nr_channels = CLAMP(nr_channels, 0, MAX_AUDIO_CHANNELS);
		size_t nr_samples = data->frames;

		int channel_nr = 0;
		for (int plane_nr = 0; channel_nr < nr_channels; plane_nr++) {
			if (channel_nr < nr_channels) {
				float *samples = (float *)data->data[plane_nr];
				if (!samples) {
					continue;
				}

				float sum = 0.0;
				for (size_t i = 0; i < nr_samples; i++) {
					float sample = samples[i];
					sum += sample * sample;
				}
				volControl->currentMagnitude[channel_nr] =
					sqrtf(sum / nr_samples);
			} else {
				volControl->currentMagnitude[channel_nr] = 0.0f;
			}
			channel_nr++;
		}
		for (int channel_nr = 0; channel_nr < MAX_AUDIO_CHANNELS;
		     channel_nr++) {
			volControl->currentMagnitude[channel_nr] = obs_mul_to_db(
				volControl->currentMagnitude[channel_nr]);
		}
	}
}

OBSSource MultiviewAudioMeter::CreateLabel(const char *name, size_t h)
{
	OBSDataAutoRelease settings = obs_data_create();
	OBSDataAutoRelease font = obs_data_create();

	std::string text;
	text += " ";
	text += name;
	text += " ";

#if defined(_WIN32)
	obs_data_set_string(font, "face", "Arial");
#elif defined(__APPLE__)
	obs_data_set_string(font, "face", "Helvetica");
#else
	obs_data_set_string(font, "face", "Monospace");
#endif
	obs_data_set_int(font, "flags", 1); // Bold text
	obs_data_set_int(font, "size", int(h / 9.81));

	obs_data_set_obj(settings, "font", font);
	obs_data_set_string(settings, "text", text.c_str());
	obs_data_set_bool(settings, "outline", false);

#ifdef _WIN32
	const char *text_source_id = "text_gdiplus";
#else
	const char *text_source_id = "text_ft2_source";
#endif

	OBSSourceAutoRelease txtSource =
		obs_source_create_private(text_source_id, name, settings);

	return txtSource.Get();
}

inline int MultiviewAudioMeter::convertToInt(float number)
{
	constexpr int min = std::numeric_limits<int>::min();
	constexpr int max = std::numeric_limits<int>::max();

	// NOTE: Conversion from 'const int' to 'float' changes max value from 2147483647 to 2147483648
	if (number >= (float)max)
		return max;
	else if (number < min)
		return min;
	else
		return int(number);
}
