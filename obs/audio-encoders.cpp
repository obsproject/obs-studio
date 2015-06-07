#include <algorithm>
#include <iomanip>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "audio-encoders.hpp"

using namespace std;

static const string encoders[] = {
	"ffmpeg_aac",
	"mf_aac",
	"libfdk_aac",
	"CoreAudio_AAC",
};

static const string &fallbackEncoder = encoders[0];

static const char *NullToEmpty(const char *str)
{
	return str ? str : "";
}

static const char *EncoderName(const char *id)
{
	return NullToEmpty(obs_encoder_get_display_name(id));
}

static map<int, const char*> bitrateMap;
static once_flag populateBitrateMap;

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
		blog(LOG_ERROR, "Encoder '%s' (%s) returned bitrate "
				"OBS_PROPERTY_LIST property of unhandled "
				"format %d",
				EncoderName(id), id, static_cast<int>(format));
		return;
	}

	const size_t count = obs_property_list_item_count(prop);
	for (size_t i = 0; i < count; i++) {
		int bitrate = static_cast<int>(
				obs_property_list_item_int(prop, i));
		bitrateMap[bitrate] = id;
	}
}

static void HandleEncoderProperties(const char *id)
{
	auto DestroyProperties = [](obs_properties_t *props)
	{
		obs_properties_destroy(props);
	};
	std::unique_ptr<obs_properties_t, decltype(DestroyProperties)> props{
			obs_get_encoder_properties(id),
			DestroyProperties};

	if (!props) {
		blog(LOG_ERROR, "Failed to get properties for encoder "
				"'%s' (%s)",
				EncoderName(id), id);
		return;
	}

	obs_property_t *bitrate = obs_properties_get(props.get(), "bitrate");

	obs_property_type type = obs_property_get_type(bitrate);
	switch (type) {
	case OBS_PROPERTY_INT:
		return HandleIntProperty(bitrate, id);

	case OBS_PROPERTY_LIST:
		return HandleListProperty(bitrate, id);

	default: break;
	}

	blog(LOG_ERROR, "Encoder '%s' (%s) returned bitrate property "
			"of unhandled type %d", EncoderName(id), id,
			static_cast<int>(type));
}

static const char *GetCodec(const char *id)
{
	return NullToEmpty(obs_get_encoder_codec(id));
}

static const string aac_ = "AAC";
static void PopulateBitrateMap()
{
	call_once(populateBitrateMap, []()
	{
		HandleEncoderProperties(fallbackEncoder.c_str());

		const char *id = nullptr;
		for (size_t i = 0; obs_enum_encoder_types(i, &id); i++) {
			auto Compare = [=](const string &val)
			{
				return val == NullToEmpty(id);
			};

			if (find_if(begin(encoders), end(encoders), Compare) !=
				end(encoders))
				continue;

			if (aac_ != GetCodec(id))
				continue;

			HandleEncoderProperties(id);
		}

		for (auto &encoder : encoders) {
			if (encoder == fallbackEncoder)
				continue;

			if (aac_ != GetCodec(encoder.c_str()))
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

		blog(LOG_INFO, "AAC encoder bitrate mapping:%s",
				ss.str().c_str());
	});
}

const map<int, const char*> &GetAACEncoderBitrateMap()
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
	return res->second;
}
