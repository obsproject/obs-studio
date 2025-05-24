#include "audio-encoders.hpp"

#include <OBSApp.hpp>
#include <widgets/OBSMainWindow.hpp>

#include <mutex>
#include <sstream>

using namespace std;

static const char *NullToEmpty(const char *str)
{
	return str ? str : "";
}

static const char *EncoderName(const std::string &id)
{
	return NullToEmpty(obs_encoder_get_display_name(id.c_str()));
}

static void HandleIntProperty(obs_property_t *prop, std::vector<int> &bitrates)
{
	const int max_ = obs_property_int_max(prop);
	const int step = obs_property_int_step(prop);

	for (int i = obs_property_int_min(prop); i <= max_; i += step)
		bitrates.push_back(i);
}

static void HandleListProperty(obs_property_t *prop, const char *id, std::vector<int> &bitrates)
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

		int bitrate = static_cast<int>(obs_property_list_item_int(prop, i));
		bitrates.push_back(bitrate);
	}
}

static void HandleSampleRate(obs_property_t *prop, const char *id)
{
	auto ReleaseData = [](obs_data_t *data) {
		obs_data_release(data);
	};
	std::unique_ptr<obs_data_t, decltype(ReleaseData)> data{obs_encoder_defaults(id), ReleaseData};

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

	uint32_t sampleRate = config_get_uint(main->Config(), "Audio", "SampleRate");

	obs_data_set_int(data.get(), "samplerate", sampleRate);

	obs_property_modified(prop, data.get());
}

static void HandleEncoderProperties(const char *id, std::vector<int> &bitrates)
{
	auto DestroyProperties = [](obs_properties_t *props) {
		obs_properties_destroy(props);
	};
	std::unique_ptr<obs_properties_t, decltype(DestroyProperties)> props{obs_get_encoder_properties(id),
									     DestroyProperties};

	if (!props) {
		blog(LOG_ERROR,
		     "Failed to get properties for encoder "
		     "'%s' (%s)",
		     EncoderName(id), id);
		return;
	}

	obs_property_t *samplerate = obs_properties_get(props.get(), "samplerate");
	if (samplerate)
		HandleSampleRate(samplerate, id);

	obs_property_t *bitrate = obs_properties_get(props.get(), "bitrate");

	obs_property_type type = obs_property_get_type(bitrate);
	switch (type) {
	case OBS_PROPERTY_INT:
		return HandleIntProperty(bitrate, bitrates);

	case OBS_PROPERTY_LIST:
		return HandleListProperty(bitrate, id, bitrates);

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

static std::vector<int> fallbackBitrates;
static map<std::string, std::vector<int>> encoderBitrates;

static void PopulateBitrateLists()
{
	static once_flag once;

	call_once(once, []() {
		struct obs_audio_info aoi;
		obs_get_audio_info(&aoi);

		/* NOTE: ffmpeg_aac and ffmpeg_opus have the same properties
		 * their bitrates will also be used as a fallback */
		HandleEncoderProperties("ffmpeg_aac", fallbackBitrates);

		if (fallbackBitrates.empty())
			blog(LOG_ERROR, "Could not enumerate fallback encoder "
					"bitrates");

		ostringstream ss;
		for (auto &bitrate : fallbackBitrates)
			ss << "\n	" << setw(3) << bitrate << " kbit/s:";

		blog(LOG_DEBUG, "Fallback encoder bitrates:%s", ss.str().c_str());

		const char *id = nullptr;
		for (size_t i = 0; obs_enum_encoder_types(i, &id); i++) {
			if (obs_get_encoder_type(id) != OBS_ENCODER_AUDIO)
				continue;

			if (strcmp(id, "ffmpeg_aac") == 0 || strcmp(id, "ffmpeg_opus") == 0)
				continue;

			std::string encoder = id;

			HandleEncoderProperties(id, encoderBitrates[encoder]);

			if (encoderBitrates[encoder].empty())
				blog(LOG_ERROR,
				     "Could not enumerate %s encoder "
				     "bitrates",
				     id);

			ostringstream ss;
			for (auto &bitrate : encoderBitrates[encoder])
				ss << "\n	" << setw(3) << bitrate << " kbit/s";

			blog(LOG_DEBUG, "%s (%s) encoder bitrates:%s", EncoderName(id), id, ss.str().c_str());
		}

		if (encoderBitrates.empty() && fallbackBitrates.empty())
			blog(LOG_ERROR, "Could not enumerate any audio encoder "
					"bitrates");
	});
}

static map<int, std::string> simpleAACBitrateMap;

static void PopulateSimpleAACBitrateMap()
{
	PopulateBitrateLists();

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

		for (auto &bitrate : fallbackBitrates)
			simpleAACBitrateMap[bitrate] = fallbackEncoder;

		const char *id = nullptr;
		for (size_t i = 0; obs_enum_encoder_types(i, &id); i++) {
			auto Compare = [=](const string &val) {
				return val == NullToEmpty(id);
			};

			if (find_if(begin(encoders), end(encoders), Compare) != end(encoders))
				continue;

			if (strcmp(GetCodec(id), "aac") != 0)
				continue;

			std::string encoder = id;
			if (encoderBitrates[encoder].empty())
				continue;

			for (auto &bitrate : encoderBitrates[encoder])
				simpleAACBitrateMap[bitrate] = encoder;
		}

		for (auto &encoder : encoders) {
			if (encoder == fallbackEncoder)
				continue;

			if (strcmp(GetCodec(encoder.c_str()), "aac") != 0)
				continue;

			for (auto &bitrate : encoderBitrates[encoder])
				simpleAACBitrateMap[bitrate] = encoder;
		}

		if (simpleAACBitrateMap.empty()) {
			blog(LOG_ERROR, "Could not enumerate any AAC encoder "
					"bitrates");
			return;
		}

		ostringstream ss;
		for (auto &entry : simpleAACBitrateMap)
			ss << "\n	" << setw(3) << entry.first << " kbit/s: '" << EncoderName(entry.second)
			   << "' (" << entry.second << ')';

		blog(LOG_DEBUG, "AAC simple encoder bitrate mapping:%s", ss.str().c_str());
	});
}

static map<int, std::string> simpleOpusBitrateMap;

static void PopulateSimpleOpusBitrateMap()
{
	PopulateBitrateLists();

	static once_flag once;

	call_once(once, []() {
		struct obs_audio_info aoi;
		obs_get_audio_info(&aoi);

		for (auto &bitrate : fallbackBitrates)
			simpleOpusBitrateMap[bitrate] = "ffmpeg_opus";

		const char *id = nullptr;
		for (size_t i = 0; obs_enum_encoder_types(i, &id); i++) {
			if (strcmp(GetCodec(id), "opus") != 0)
				continue;

			std::string encoder = id;
			if (encoderBitrates[encoder].empty())
				continue;

			for (auto &bitrate : encoderBitrates[encoder])
				simpleOpusBitrateMap[bitrate] = encoder;
		}

		if (simpleOpusBitrateMap.empty()) {
			blog(LOG_ERROR, "Could not enumerate any Opus encoder "
					"bitrates");
			return;
		}

		ostringstream ss;
		for (auto &entry : simpleOpusBitrateMap)
			ss << "\n	" << setw(3) << entry.first << " kbit/s: '" << EncoderName(entry.second)
			   << "' (" << entry.second << ')';

		blog(LOG_DEBUG, "Opus simple encoder bitrate mapping:%s", ss.str().c_str());
	});
}

const map<int, std::string> &GetSimpleAACEncoderBitrateMap()
{
	PopulateSimpleAACBitrateMap();
	return simpleAACBitrateMap;
}

const map<int, std::string> &GetSimpleOpusEncoderBitrateMap()
{
	PopulateSimpleOpusBitrateMap();
	return simpleOpusBitrateMap;
}

const char *GetSimpleAACEncoderForBitrate(int bitrate)
{
	auto &map_ = GetSimpleAACEncoderBitrateMap();
	auto res = map_.find(bitrate);
	if (res == end(map_))
		return NULL;
	return res->second.c_str();
}

const char *GetSimpleOpusEncoderForBitrate(int bitrate)
{
	auto &map_ = GetSimpleOpusEncoderBitrateMap();
	auto res = map_.find(bitrate);
	if (res == end(map_))
		return NULL;
	return res->second.c_str();
}

#define INVALID_BITRATE 10000

static int FindClosestAvailableSimpleBitrate(int bitrate, const map<int, std::string> &map)
{
	int prev = 0;
	int next = INVALID_BITRATE;

	for (auto val : map) {
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

int FindClosestAvailableSimpleAACBitrate(int bitrate)
{
	return FindClosestAvailableSimpleBitrate(bitrate, GetSimpleAACEncoderBitrateMap());
}

int FindClosestAvailableSimpleOpusBitrate(int bitrate)
{
	return FindClosestAvailableSimpleBitrate(bitrate, GetSimpleOpusEncoderBitrateMap());
}

const std::vector<int> &GetAudioEncoderBitrates(const char *id)
{
	std::string encoder = id;
	PopulateBitrateLists();
	if (encoderBitrates[encoder].empty())
		return fallbackBitrates;
	return encoderBitrates[encoder];
}

int FindClosestAvailableAudioBitrate(const char *id, int bitrate)
{
	PopulateBitrateLists();

	int prev = 0;
	int next = INVALID_BITRATE;
	std::string encoder = id;

	for (auto val : encoderBitrates[encoder].empty() ? fallbackBitrates : encoderBitrates[encoder]) {
		if (next > val) {
			if (val == bitrate)
				return bitrate;

			if (val < next && val > bitrate)
				next = val;
			if (val > prev && val < bitrate)
				prev = val;
		}
	}

	if (next != INVALID_BITRATE)
		return next;
	if (prev != 0)
		return prev;
	return 192;
}
