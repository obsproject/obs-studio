#include <algorithm>
#include <iomanip>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "audio-encoders.hpp"
#include "obs-app.hpp"
#include "window-main.hpp"

using namespace std;

static const char *NullToEmpty(const char *str)
{
	return str ? str : "";
}

static const char *EncoderName(const std::string &id)
{
	return NullToEmpty(obs_encoder_get_display_name(id.c_str()));
}

static map<int, std::string> bitrateMap;

static void HandleIntProperty(obs_property_t *prop, const char *id)
{
	const int max_ = obs_property_int_max(prop);
	const int step = obs_property_int_step(prop);

	for (int i = obs_property_int_min(prop); i <= max_; i += step)
		bitrateMap[i] = id;
}

static void HandleListProperty(obs_property_t *prop, const char *id)
{
	obs_combo_format format = obs_property_list_format(prop);
	if (format != OBS_COMBO_FORMAT_INT) {
		blog(LOG_ERROR,
		     "Encoder '%s' (%s) returned bitrate "
		     "OBS_PROPERTY_LIST property of unhandled "
		     "format %d",
		     EncoderName(id), id, static_cast<int>(format));
		return;
	}

	const size_t count = obs_property_list_item_count(prop);
	for (size_t i = 0; i < count; i++) {
		if (obs_property_list_item_disabled(prop, i))
			continue;

		int bitrate =
			static_cast<int>(obs_property_list_item_int(prop, i));
		bitrateMap[bitrate] = id;
	}
}

static void HandleSampleRate(obs_property_t *prop, const char *id)
{
	auto ReleaseData = [](obs_data_t *data) { obs_data_release(data); };
	std::unique_ptr<obs_data_t, decltype(ReleaseData)> data{
		obs_encoder_defaults(id), ReleaseData};

	if (!data) {
		blog(LOG_ERROR,
		     "Failed to get defaults for encoder '%s' (%s) "
		     "while populating bitrate map",
		     EncoderName(id), id);
		return;
	}

	auto main = reinterpret_cast<OBSMainWindow *>(App()->GetMainWindow());
	if (!main) {
		blog(LOG_ERROR, "Failed to get main window while populating "
				"bitrate map");
		return;
	}

	uint32_t sampleRate =
		config_get_uint(main->Config(), "Audio", "SampleRate");

	obs_data_set_int(data.get(), "samplerate", sampleRate);

	obs_property_modified(prop, data.get());
}

static void HandleEncoderProperties(const char *id)
{
	auto DestroyProperties = [](obs_properties_t *props) {
		obs_properties_destroy(props);
	};
	std::unique_ptr<obs_properties_t, decltype(DestroyProperties)> props{
		obs_get_encoder_properties(id), DestroyProperties};

	if (!props) {
		blog(LOG_ERROR,
		     "Failed to get properties for encoder "
		     "'%s' (%s)",
		     EncoderName(id), id);
		return;
	}

	obs_property_t *samplerate =
		obs_properties_get(props.get(), "samplerate");
	if (samplerate)
		HandleSampleRate(samplerate, id);

	obs_property_t *bitrate = obs_properties_get(props.get(), "bitrate");

	obs_property_type type = obs_property_get_type(bitrate);
	switch (type) {
	case OBS_PROPERTY_INT:
		return HandleIntProperty(bitrate, id);

	case OBS_PROPERTY_LIST:
		return HandleListProperty(bitrate, id);

	default:
		break;
	}

	blog(LOG_ERROR,
	     "Encoder '%s' (%s) returned bitrate property "
	     "of unhandled type %d",
	     EncoderName(id), id, static_cast<int>(type));
}

static const char *GetCodec(const char *id)
{
	return NullToEmpty(obs_get_encoder_codec(id));
}

static void PopulateBitrateMap()
{
	static once_flag once;

	call_once(once, []() {
		const string encoders[] = {
			"ffmpeg_aac",
			"libfdk_aac",
			"CoreAudio_AAC",
		};

		const string fallbackEncoder = encoders[0];

		struct obs_audio_info aoi;
		obs_get_audio_info(&aoi);
		uint32_t output_channels = get_audio_channels(aoi.speakers);

		HandleEncoderProperties(fallbackEncoder.c_str());

		const char *id = nullptr;
		for (size_t i = 0; obs_enum_encoder_types(i, &id); i++) {
			auto Compare = [=](const string &val) {
				return val == NullToEmpty(id);
			};

			if (find_if(begin(encoders), end(encoders), Compare) !=
			    end(encoders))
				continue;

			if (strcmp(GetCodec(id), "AAC") != 0)
				continue;

			HandleEncoderProperties(id);
		}

		for (auto &encoder : encoders) {
			if (encoder == fallbackEncoder)
				continue;

			if (strcmp(GetCodec(encoder.c_str()), "AAC") != 0)
				continue;

			HandleEncoderProperties(encoder.c_str());
		}

		if (bitrateMap.empty()) {
			blog(LOG_ERROR, "Could not enumerate any AAC encoder "
					"bitrates");
			return;
		}

		ostringstream ss;
		for (auto &entry : bitrateMap)
			ss << "\n	" << setw(3) << entry.first
			   << " kbit/s: '" << EncoderName(entry.second) << "' ("
			   << entry.second << ')';

		blog(LOG_DEBUG, "AAC encoder bitrate mapping:%s",
		     ss.str().c_str());
	});
}

const map<int, std::string> &GetAACEncoderBitrateMap()
{
	PopulateBitrateMap();
	return bitrateMap;
}

const char *GetAACEncoderForBitrate(int bitrate)
{
	auto &map_ = GetAACEncoderBitrateMap();
	auto res = map_.find(bitrate);
	if (res == end(map_))
		return NULL;
	return res->second.c_str();
}

#define INVALID_BITRATE 10000

int FindClosestAvailableAACBitrate(int bitrate)
{
	auto &map_ = GetAACEncoderBitrateMap();
	int prev = 0;
	int next = INVALID_BITRATE;

	for (auto val : map_) {
		if (next > val.first) {
			if (val.first == bitrate)
				return bitrate;

			if (val.first < next && val.first > bitrate)
				next = val.first;
			if (val.first > prev && val.first < bitrate)
				prev = val.first;
		}
	}

	if (next != INVALID_BITRATE)
		return next;
	if (prev != 0)
		return prev;
	return 192;
}
