/*****************************************************************************
Copyright (C) 2025 by pkv@obsproject.com

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/
#include "mac-au.h"
#include "mac-au-plugin.h"
#include <vector>

#define MT_ obs_module_text
#define S_PLUGIN "au_uid"
#define S_EDITOR "au_open_gui"
#define S_SIDECHAIN_SOURCE "sidechain_source"
#define S_NOGUI "au_noview"
#define S_ERR "au_error"

#define TEXT_EDITOR MT_("AU.Button")
#define TEXT_PLUGIN MT_("AU.Plugin")
#define TEXT_SIDECHAIN_SOURCE  MT_("AU.SidechainSource")
#define TEXT_NOGUI MT_("AU.NOGUI")
#define TEXT_ERR MT_("AU.ERR")

/* -------------------------------------------------------- */
#define do_log(level, format, ...) \
	blog(level, "[AU filter: '%s'] " format, obs_source_get_name(ad->context), ## __VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ## __VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ## __VA_ARGS__)

#ifdef _DEBUG
#define debug(format, ...) do_log(LOG_DEBUG, format, ## __VA_ARGS__)
#endif
/* -------------------------------------------------------- */
struct AuPluginDeleter {
	void operator()(au_plugin *p) const noexcept
	{
		if (p)
			au_plugin_destroy(p);
	}
};

struct au_audio_info {
	uint32_t frames;
	uint64_t timestamp;
};

struct sidechain_prop_info {
	obs_property_t *sources;
	obs_source_t *parent;
};

static inline enum speaker_layout convert_speaker_layout(uint8_t channels)
{
	switch (channels) {
	case 0:
		return SPEAKERS_UNKNOWN;
	case 1:
		return SPEAKERS_MONO;
	case 2:
		return SPEAKERS_STEREO;
	case 3:
		return SPEAKERS_2POINT1;
	case 4:
		return SPEAKERS_4POINT0;
	case 5:
		return SPEAKERS_4POINT1;
	case 6:
		return SPEAKERS_5POINT1;
	case 8:
		return SPEAKERS_7POINT1;
	default:
		return SPEAKERS_UNKNOWN;
	}
}

static inline obs_source_t *get_sidechain(struct au_data *ad)
{
	if (ad->weak_sidechain)
		return obs_weak_source_get_source(ad->weak_sidechain);
	return NULL;
}
/* --------------------- deque mgt -------------------------- */

static inline void clear_deque(struct deque *buf)
{
	deque_pop_front(buf, NULL, buf->size);
}

static void reset_data(struct au_data *ad)
{
	for (size_t i = 0; i < ad->channels; i++) {
		clear_deque(&ad->input_buffers[i]);
		clear_deque(&ad->output_buffers[i]);
	}

	clear_deque(&ad->info_buffer);
}

static void reset_sidechain_data(struct au_data *ad)
{
	for (size_t i = 0; i < ad->channels; i++)
		clear_deque(&ad->sc_input_buffers[i]);
}

/* --------------------- main functions ------------------- */
static const char *au_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("AU.Filter");
}

void au_save_state(void *data, obs_data_t *settings)
{
	struct au_data *ad = static_cast<au_data *>(data);
	if (!ad)
		return;

	auto plugin = std::atomic_load(&ad->plugin);
	if (!plugin)
		return;

	CFDataRef state = au_plugin_save_state(plugin.get());
	if (!state)
		return;

	const UInt8 *bytes = CFDataGetBytePtr(state);
	CFIndex len = CFDataGetLength(state);

	size_t hex_len = (size_t)len * 2 + 1;
	char *hex = (char *)bmalloc(hex_len);
	if (!hex) {
		CFRelease(state);
		return;
	}

	static const char hexdigits[] = "0123456789ABCDEF";

	for (CFIndex i = 0; i < len; i++) {
		UInt8 b = bytes[i];
		hex[2 * i] = hexdigits[b >> 4];
		hex[2 * i + 1] = hexdigits[b & 0xF];
	}
	hex[2 * len] = '\0';

	obs_data_set_string(settings, "au_state", hex);
	obs_data_set_string(settings, "au_uid", ad->uid);

	bfree(hex);
	CFRelease(state);
}

static void sidechain_capture(void *data, obs_source_t *source, const struct audio_data *audio, bool muted);
static void au_destroy(void *data)
{
	struct au_data *ad = static_cast<au_data *>(data);
	if (!ad)
		return;

	ad->bypass.store(true, std::memory_order_relaxed);
	ad->sidechain_enabled.store(false, std::memory_order_relaxed);

	if (ad->weak_sidechain) {
		obs_source_t *sidechain = get_sidechain(ad);
		if (sidechain) {
			obs_source_remove_audio_capture_callback(sidechain, sidechain_capture, ad);
			obs_source_release(sidechain);
		}
		obs_weak_source_release(ad->weak_sidechain);
	}

	if (ad->sc_resampler)
		ad->sc_resampler.reset();

	for (size_t i = 0; i < ad->channels; i++) {
		deque_free(&ad->input_buffers[i]);
		deque_free(&ad->output_buffers[i]);
	}
	for (size_t i = 0; i < ad->channels; i++)
		deque_free(&ad->sc_input_buffers[i]);

	bfree(ad->copy_buffers[0]);
	bfree(ad->sc_copy_buffers[0]);
	deque_free(&ad->info_buffer);
	da_free(ad->output_data);

	auto plugin = std::atomic_load(&ad->plugin);
	if (plugin) {
		std::atomic_store(&ad->plugin, std::shared_ptr<au_plugin>{});
	}

	delete ad;
}

static bool create_new_au(au_data *ad, const char *uid)
{
	// Find new plugin descriptor
	const struct au_descriptor *desc = au_find_by_uid(uid);
	if (!desc) {
		info("AU '%s' was not found on this mac.", uid);
		ad->name[0] = 0;
		ad->has_sidechain.store(false, std::memory_order_relaxed);
		ad->sc_channels = 0;
		ad->bypass.store(true, std::memory_order_relaxed);
		ad->last_init_failed = true;

		return false;
	}

	// Create new AU instance
	struct au_plugin *raw =
		au_plugin_create(desc, (double)ad->sample_rate, (uint32_t)ad->frames, (uint32_t)ad->channels);

	if (!raw) {
		info("AU '%s' failed to initialize.", desc->name);
		ad->name[0] = 0;
		ad->has_sidechain.store(false, std::memory_order_relaxed);
		ad->sc_channels = 0;
		ad->bypass.store(true, std::memory_order_relaxed);
		ad->last_init_failed = true;
		return false;
	}

	std::shared_ptr<au_plugin> plugin(raw, AuPluginDeleter{});

	strncpy(ad->name, desc->name, sizeof(ad->name));
	ad->name[sizeof(ad->name) - 1] = '\0';

	strncpy(ad->uid, desc->uid, sizeof(ad->uid));
	ad->uid[sizeof(ad->uid) - 1] = '\0';

	uint32_t sc_channels = plugin->sidechain_num_channels;
	bool has_sc = (sc_channels == 1 || sc_channels == 2);
	if (!has_sc)
		sc_channels = 0;

	bool has_view = plugin->has_view;

	std::atomic_store(&ad->plugin, plugin);
	ad->sc_channels = sc_channels;
	ad->has_sidechain.store(has_sc, std::memory_order_relaxed);
	ad->has_view.store(has_view, std::memory_order_relaxed);
	ad->plugin->is_v3 = desc->is_v3;

	info("AU %s (vendor: %s, version : %s, %s) initialized.", desc->name, desc->vendor, desc->version,
	     desc->is_v3 ? "AUv3" : "AUv2");

	return true;
}

static void destroy_current_au(au_data *ad, obs_data_t *settings)
{
	if (!ad)
		return;

	ad->bypass.store(true, std::memory_order_relaxed);
	auto plugin = std::atomic_load(&ad->plugin);
	if (plugin)
		au_plugin_hide_editor(plugin.get());

	std::atomic_store(&ad->plugin, std::shared_ptr<au_plugin>{});

	ad->uid[0] = 0;
	ad->name[0] = 0;
	ad->has_sidechain.store(false, std::memory_order_relaxed);
	ad->sc_channels = 0;
	ad->has_view.store(false, std::memory_order_relaxed);

	// clear sc stuff
	obs_weak_source_t *old_weak = nullptr;
	{
		std::lock_guard<std::mutex> lock(ad->sidechain_update_mutex);
		if (ad->weak_sidechain) {
			old_weak = ad->weak_sidechain;
			ad->weak_sidechain = nullptr;
		}
		ad->sidechain_name.clear();
		ad->sidechain_enabled.store(false, std::memory_order_relaxed);
		obs_data_set_string(settings, S_SIDECHAIN_SOURCE, NULL);
	}

	if (old_weak) {
		obs_source_t *old_sidechain = obs_weak_source_get_source(old_weak);
		if (old_sidechain) {
			obs_source_remove_audio_capture_callback(old_sidechain, sidechain_capture, ad);
			obs_source_release(old_sidechain);
		}
		obs_weak_source_release(old_weak);
	}
}

static void au_load_state(au_data *ad, obs_data_t *settings)
{
	if (!ad)
		return;

	auto plugin = std::atomic_load(&ad->plugin);
	if (!plugin)
		return;

	const char *hex = obs_data_get_string(settings, "au_state");
	if (hex && *hex) {
		if ((strlen(hex) & 1) != 0)
			return;

		size_t len = strlen(hex) / 2;
		std::vector<uint8_t> bytes(len);
		for (size_t i = 0; i < len; i++) {
			char tmp[3] = {hex[2 * i], hex[2 * i + 1], 0};
			char *end = nullptr;
			unsigned long v = strtoul(tmp, &end, 16);
			if (!end || *end != '\0' || v > 0xFF) {
				return;
			}
			bytes.push_back((uint8_t)v);
		}

		CFDataRef d = CFDataCreate(kCFAllocatorDefault, bytes.data(), bytes.size());
		if (d) {
			au_plugin_load_state(plugin.get(), d);
			CFRelease(d);
		}
	}
}

// code ported from obs-filters/compressor.c
static void sidechain_swap(au_data *ad, obs_data *settings)
{
	if (!ad)
		return;

	if (!ad->has_sidechain.load(std::memory_order_relaxed))
		return;

	ad->sidechain_enabled.store(false, std::memory_order_relaxed);

	const char *sc_name_c = obs_data_get_string(settings, S_SIDECHAIN_SOURCE);
	std::string sc_name = sc_name_c ? sc_name_c : std::string();
	bool valid = !sc_name.empty() && sc_name != "none";

	obs_weak_source_t *old_weak = nullptr;

	{
		std::lock_guard<std::mutex> lk(ad->sidechain_update_mutex);

		if (!valid) {
			if (ad->weak_sidechain) {
				old_weak = ad->weak_sidechain;
				ad->weak_sidechain = nullptr;
			}

			if (!ad->sidechain_name.empty()) {
				ad->sidechain_name.clear();
			}
		} else {
			if (ad->sidechain_name.empty() || ad->sidechain_name != sc_name) {

				if (ad->weak_sidechain) {
					old_weak = ad->weak_sidechain;
					ad->weak_sidechain = nullptr;
				}

				ad->sidechain_name.clear();
				ad->sidechain_name = sc_name;
				ad->sidechain_check_time = os_gettime_ns() - 3000000000ULL;
			}
		}
	}
	ad->sidechain_enabled.store(valid, std::memory_order_relaxed);

	if (old_weak) {
		obs_source_t *s = obs_weak_source_get_source(old_weak);
		if (s) {
			obs_source_remove_audio_capture_callback(s, sidechain_capture, ad);
			obs_source_release(s);
		}
		obs_weak_source_release(old_weak);
	}
}

static void au_update(void *data, obs_data_t *settings)
{
	auto *ad = static_cast<au_data *>(data);
	if (!ad)
		return;

	const char *uid = obs_data_get_string(settings, S_PLUGIN);
	std::string uid_str = uid ? uid : "";

	// 1) No plugin selected → bypass and clear state
	if (uid_str.empty()) {
		destroy_current_au(ad, settings);
		return;
	}

	bool initial_load = (ad->uid[0] == 0);
	bool swapping = (strncmp(ad->uid, uid, sizeof(ad->uid)) != 0);
	// 2) Swap AUs
	if (swapping) {
		strncpy(ad->uid, uid, sizeof(ad->uid)); // we always store the uid even in failed state
		ad->last_init_failed = false;

		if (!initial_load) {
			destroy_current_au(ad, settings);
		}

		if (!create_new_au(ad, uid))
			return;

		if (initial_load) {
			au_load_state(ad, settings);
		}
		ad->bypass.store(false, std::memory_order_relaxed);
	}

	//  3) Sidechain swap logic (taken from obs-filters/compressor.c; quite tricky since part is done during video_tick)
	auto plugin = std::atomic_load(&ad->plugin);
	if (!plugin || !ad->has_sidechain.load(std::memory_order_relaxed))
		return;

	sidechain_swap(ad, settings);
}

static void *au_create(obs_data_t *settings, obs_source_t *filter)
{
	auto *ad = new au_data();

	ad->context = filter;
	std::atomic_store(&ad->plugin, std::shared_ptr<au_plugin>{});
	ad->uid[0] = 0;

	ad->frames = (size_t)FRAME_SIZE;
	ad->channels = audio_output_get_channels(obs_get_audio());
	ad->sample_rate = audio_output_get_sample_rate(obs_get_audio());
	ad->layout = audio_output_get_info(obs_get_audio())->speakers;

	ad->has_sidechain.store(false, std::memory_order_relaxed);
	ad->sidechain_enabled.store(false, std::memory_order_relaxed);
	ad->latency = 1000000000LL / (1000 / BUFFER_SIZE_MSEC);

	// allocate copy buffers(which are *contiguous* for the channels)
	size_t channels = ad->channels;
	size_t frames = (size_t)FRAME_SIZE;
	ad->copy_buffers[0] = (float *)bmalloc((size_t)FRAME_SIZE * channels * sizeof(float));
	ad->sc_copy_buffers[0] = (float *)bmalloc(FRAME_SIZE * MAX_AUDIO_CHANNELS * sizeof(float));

	for (size_t c = 1; c < channels; ++c)
		ad->copy_buffers[c] = ad->copy_buffers[c - 1] + frames;

	for (size_t c = 1; c < 2; ++c)
		ad->sc_copy_buffers[c] = ad->sc_copy_buffers[c - 1] + frames;

	// reserve deque buffers
	for (size_t i = 0; i < channels; i++) {
		deque_reserve(&ad->input_buffers[i], frames * sizeof(float));
		deque_reserve(&ad->output_buffers[i], frames * sizeof(float));
		deque_reserve(&ad->sc_input_buffers[i], frames * sizeof(float));
	}

	ad->bypass.store(true, std::memory_order_relaxed);
	au_update(ad, settings);
	return ad;
}

/* -------------- audio processing (incl. sc) --------------- */
static inline void preprocess_input(struct au_data *ad)
{
	if (!ad)
		return;

	int num_channels = (int)ad->channels;
	int sc_num_channels = (int)ad->sc_channels;
	int frames = (int)ad->frames;
	bool has_sc = ad->has_sidechain.load(std::memory_order_relaxed);
	bool sc_enabled = ad->sidechain_enabled.load(std::memory_order_relaxed);
	size_t segment_size = ad->frames * sizeof(float);

	// Ensure sidechain deque capacity
	if (has_sc && sc_enabled) {
		std::lock_guard<std::mutex> lock(ad->sidechain_mutex);
		for (int i = 0; i < sc_num_channels; i++) {
			if (ad->sc_input_buffers[i].size < segment_size)
				deque_push_back_zero(&ad->sc_input_buffers[i], segment_size);
		}
	}

	// Pop from input deque into our working buffers
	for (int i = 0; i < num_channels; i++)
		deque_pop_front(&ad->input_buffers[i], ad->copy_buffers[i], (size_t)frames * sizeof(float));

	// Pop sidechain into working buffers
	if (has_sc && sc_enabled) {
		{
			std::lock_guard<std::mutex> lock(ad->sidechain_mutex);
			for (int i = 0; i < sc_num_channels; i++)
				deque_pop_front(&ad->sc_input_buffers[i], ad->sc_copy_buffers[i],
						(size_t)frames * sizeof(float));
		}
		// Optional resampling if SC layout differs
		bool needs_resampling = (ad->channels != ad->sc_channels) &&
					(ad->sc_channels == 1 || ad->sc_channels == 2);
		auto sc_resampler = std::atomic_load(&ad->sc_resampler);
		if (needs_resampling && sc_resampler) {
			uint8_t *resampled[2] = {nullptr, nullptr};
			uint32_t out_frames = 0;
			uint64_t ts_offset = 0;

			if (audio_resampler_resample(sc_resampler.get(), resampled, &out_frames, &ts_offset,
						     (const uint8_t **)ad->sc_copy_buffers, (uint32_t)frames)) {
				// Copy the resampled data back into sc_copy_buffers
				for (int ch = 0; ch < sc_num_channels; ++ch) {
					memcpy(ad->sc_copy_buffers[ch], resampled[ch],
					       (size_t)out_frames * sizeof(float));
				}
			}
		}
	}
}

static inline void process(struct au_data *ad, const std::shared_ptr<au_plugin> &plugin)
{
	if (!ad || !plugin)
		return;

	int num_channels = (int)ad->channels;
	int frames = (int)ad->frames;
	bool bypass = ad->bypass.load(std::memory_order_relaxed);
	bool has_sc = ad->has_sidechain.load(std::memory_order_relaxed);
	bool sc_enabled = ad->sidechain_enabled.load(std::memory_order_relaxed);
	preprocess_input(ad);

	if (!bypass && plugin->num_enabled_out_audio_buses) {
		// Sidechain pointer array if enabled
		float *const *sc_ptrs = nullptr;
		if (has_sc && sc_enabled && ad->sc_channels > 0)
			sc_ptrs = ad->sc_copy_buffers;

		au_plugin_process(plugin.get(), ad->copy_buffers, sc_ptrs, sc_enabled, (uint32_t)frames,
				  (uint32_t)num_channels);
	}
	// Push processed segment to output
	size_t segment_size = ad->frames * sizeof(float);
	for (size_t i = 0; i < ad->channels; i++)
		deque_push_back(&ad->output_buffers[i], ad->copy_buffers[i], segment_size);
}

static struct obs_audio_data *au_filter_audio(void *data, struct obs_audio_data *audio)
{
	auto *ad = static_cast<au_data *>(data);
	if (!ad)
		return audio;

	auto plugin = std::atomic_load(&ad->plugin);
	if (!plugin || !plugin->num_enabled_out_audio_buses)
		return audio;

	bool bypass = ad->bypass.load(std::memory_order_relaxed);
	if (bypass)
		return audio;

	// If timestamp has dramatically changed, consider it a new stream of audio data. Clear all circular buffers to
	// prevent  old audio data from being processed as part of the new data.
	if (ad->last_timestamp) {
		int64_t diff = llabs((int64_t)ad->last_timestamp - (int64_t)audio->timestamp);
		if (diff > 1000000000LL)
			reset_data(ad);
	}
	ad->last_timestamp = audio->timestamp;

	struct au_audio_info info;
	size_t segment_size = ad->frames * sizeof(float);
	size_t out_size;
	// push audio packet info (timestamp/frame count) to info deque
	info.frames = audio->frames;
	info.timestamp = audio->timestamp;
	deque_push_back(&ad->info_buffer, &info, sizeof(info));

	// push back current audio data to input deque
	for (size_t i = 0; i < ad->channels; i++)
		deque_push_back(&ad->input_buffers[i], audio->data[i], audio->frames * sizeof(float));

	// pop/process each 10ms segments, push back to output deque
	while (ad->input_buffers[0].size >= segment_size)
		process(ad, plugin);

	// peek front of info deque, check to see if we have enough to pop the expected packet size, if not, return null
	memset(&info, 0, sizeof(info));
	deque_peek_front(&ad->info_buffer, &info, sizeof(info));
	out_size = info.frames * sizeof(float);

	if (ad->output_buffers[0].size < out_size)
		return NULL;

	// if there's enough audio data buffered in the output deque,pop and return a packet
	deque_pop_front(&ad->info_buffer, NULL, sizeof(info));
	da_resize(ad->output_data, out_size * ad->channels);

	for (size_t i = 0; i < ad->channels; i++) {
		ad->output_audio.data[i] = (uint8_t *)&ad->output_data.array[i * out_size];

		deque_pop_front(&ad->output_buffers[i], ad->output_audio.data[i], out_size);
	}

	ad->running_sample_count += info.frames;
	ad->system_time = os_gettime_ns();
	ad->output_audio.frames = info.frames;
	ad->output_audio.timestamp = info.timestamp - ad->latency;
	return &ad->output_audio;
}

static void sidechain_capture(void *data, obs_source_t *source, const struct audio_data *audio, bool muted)
{
	UNUSED_PARAMETER(source);
	UNUSED_PARAMETER(muted);
	auto *ad = static_cast<au_data *>(data);
	if (!ad)
		return;

	auto plugin = std::atomic_load(&ad->plugin);
	if (!plugin)
		return;

	bool bypass = ad->bypass.load(std::memory_order_relaxed);
	bool sc_enabled = ad->sidechain_enabled.load(std::memory_order_relaxed);

	if (bypass || !sc_enabled)
		return;

	if (ad->sc_channels != 1 && ad->sc_channels != 2)
		return;
	// If timestamp has dramatically changed, consider it a new stream of audio data. Clear all circular buffers to
	// prevent  old audio data from being processed as part of the new data.
	if (ad->sc_last_timestamp) {
		int64_t diff = llabs((int64_t)ad->sc_last_timestamp - (int64_t)audio->timestamp);

		if (diff > 1000000000LL) {
			std::lock_guard<std::mutex> lock(ad->sidechain_mutex);
			reset_sidechain_data(ad);
		}
	}

	ad->sc_last_timestamp = audio->timestamp;

	// push back current audio data to input deque
	{
		std::lock_guard<std::mutex> lock(ad->sidechain_mutex);
		for (size_t i = 0; i < ad->channels; i++)
			deque_push_back(&ad->sc_input_buffers[i], audio->data[i], audio->frames * sizeof(float));
	}
}

// written after obs-filters/compressor-filter.c for the sidechain logic
static void au_tick(void *data, float seconds)
{
	auto *ad = static_cast<au_data *>(data);
	if (!ad)
		return;

	bool has_sc = ad->has_sidechain.load(std::memory_order_relaxed);
	if (!has_sc)
		return;

	std::string new_name = {};

	{
		std::lock_guard<std::mutex> lock(ad->sidechain_update_mutex);
		if (!ad->sidechain_name.empty() && !ad->weak_sidechain) {
			uint64_t t = os_gettime_ns();

			if (t - ad->sidechain_check_time > 3000000000) {
				new_name = ad->sidechain_name;
				ad->sidechain_check_time = t;
			}
		}
	}

	if (!new_name.empty()) {
		obs_source_t *sidechain = obs_get_source_by_name(new_name.c_str());
		obs_weak_source_t *weak_sidechain = sidechain ? obs_source_get_weak_source(sidechain) : NULL;

		{
			std::lock_guard<std::mutex> lock(ad->sidechain_update_mutex);
			if (!ad->sidechain_name.empty() && ad->sidechain_name == new_name) {
				ad->weak_sidechain = weak_sidechain;
				weak_sidechain = NULL;
			}
		}

		if (sidechain) {
			// downmix or upmix if channel count is mismatched
			bool needs_resampling = ad->channels != ad->sc_channels;
			if (needs_resampling) {
				struct resample_info src, dst;
				src.samples_per_sec = ad->sample_rate;
				src.format = AUDIO_FORMAT_FLOAT_PLANAR;
				src.speakers = convert_speaker_layout((uint8_t)ad->channels);

				dst.samples_per_sec = ad->sample_rate;
				dst.format = AUDIO_FORMAT_FLOAT_PLANAR;
				dst.speakers = convert_speaker_layout((uint8_t)ad->sc_channels);

				audio_resampler *raw = audio_resampler_create(&dst, &src);
				if (!raw) {
					std::atomic_store(&ad->sc_resampler, std::shared_ptr<audio_resampler>{});
				} else {
					std::shared_ptr<audio_resampler> sp(raw, [](audio_resampler *r) {
						if (r)
							audio_resampler_destroy(r);
					});
					std::atomic_store(&ad->sc_resampler, sp);
				}
			} else {
				std::atomic_store(&ad->sc_resampler, std::shared_ptr<audio_resampler>{});
			}
			obs_source_add_audio_capture_callback(sidechain, sidechain_capture, ad);
			ad->sidechain_enabled.store(true, std::memory_order_relaxed);
			obs_weak_source_release(weak_sidechain);
			obs_source_release(sidechain);
		}
	}
	UNUSED_PARAMETER(seconds);
}

/* ---------------- properties functions --------------------- */

static bool au_show_gui_callback(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	auto *ad = static_cast<au_data *>(data);
	if (!ad)
		return false;

	auto plugin = std::atomic_load(&ad->plugin);

	if (!plugin)
		return false;

	if (!plugin->has_view)
		return false;

	if (!plugin->editor_is_visible)
		au_plugin_show_editor(plugin.get());
	else
		au_plugin_hide_editor(plugin.get());

	return true;
}

static bool add_sources(void *data, obs_source_t *source)
{
	struct sidechain_prop_info *info = (struct sidechain_prop_info *)data;
	uint32_t caps = obs_source_get_output_flags(source);

	if (source == info->parent)
		return true;
	if ((caps & OBS_SOURCE_AUDIO) == 0)
		return true;

	const char *name = obs_source_get_name(source);
	obs_property_list_add_string(info->sources, name, name);
	return true;
}

bool on_au_changed_cb(void *data, obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(settings);
	auto *ad = static_cast<au_data *>(data);
	if (!ad)
		return false;

	bool has_sc = ad->has_sidechain.load(std::memory_order_relaxed);

	obs_property_t *p = obs_properties_get(props, S_SIDECHAIN_SOURCE);

	if (has_sc) {
		obs_source_t *parent = obs_filter_get_parent(ad->context);
		obs_property_list_clear(p);
		obs_property_list_add_string(p, obs_module_text("None"), "none");
		struct sidechain_prop_info info = {p, parent};
		obs_enum_sources(add_sources, &info);
		obs_property_set_visible(p, true);
	} else {
		obs_property_set_visible(p, false);
	}

	obs_property_t *button = obs_properties_get(props, S_EDITOR);
	obs_property_set_visible(button, ad->has_view.load(std::memory_order_relaxed));

	obs_property_t *noview = obs_properties_get(props, S_NOGUI);
	obs_property_set_visible(noview, !ad->has_view && !ad->last_init_failed);

	obs_property_t *err = obs_properties_get(props, S_ERR);
	if (err)
		obs_properties_remove_by_name(props, S_ERR);

	if (ad->last_init_failed) {
		obs_property_t *err2 = obs_properties_add_text(props, S_ERR, TEXT_ERR, OBS_TEXT_INFO);
		obs_property_text_set_info_type(err2, OBS_TEXT_INFO_ERROR);
	}
	return true;
}

extern struct au_list g_au_list;
static obs_properties_t *au_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *sources;
	auto *ad = static_cast<au_data *>(data);
	if (!ad)
		return props;

	obs_property_t *aulist =
		obs_properties_add_list(props, S_PLUGIN, TEXT_PLUGIN, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(aulist, obs_module_text("AU.Select"), "");

	for (int i = 0; i < g_au_list.count; i++) {
		struct au_descriptor *d = &g_au_list.items[i];
		std::string name = d->name;
		std::string vendor = d->vendor;
		std::string label;

		if (!vendor.empty())
			label = name + " (" + vendor + ")";
		else
			label = name;

		const char *value = d->uid;
		obs_property_list_add_string(aulist, label.c_str(), value);
	}

	obs_property_t *button =
		obs_properties_add_button(props, S_EDITOR, obs_module_text(TEXT_EDITOR), au_show_gui_callback);
	obs_property_set_visible(button, ad->has_view.load(std::memory_order_relaxed));

	sources = obs_properties_add_list(props, S_SIDECHAIN_SOURCE, TEXT_SIDECHAIN_SOURCE, OBS_COMBO_TYPE_LIST,
					  OBS_COMBO_FORMAT_STRING);
	obs_property_set_modified_callback2(aulist, on_au_changed_cb, data);

	if (ad->last_init_failed) {
		obs_property_t *err = obs_properties_add_text(props, S_ERR, TEXT_ERR, OBS_TEXT_INFO);
		obs_property_text_set_info_type(err, OBS_TEXT_INFO_ERROR);
	}

	obs_property_t *noview = obs_properties_add_text(props, S_NOGUI, TEXT_NOGUI, OBS_TEXT_INFO);
	obs_property_text_set_info_type(noview, OBS_TEXT_INFO_WARNING);
	obs_property_set_visible(noview, !ad->has_view);

	return props;
}

void register_aufilter_source()
{
	struct obs_source_info au_filter = {};
	au_filter.id = "au_filter";
	au_filter.type = OBS_SOURCE_TYPE_FILTER;
	au_filter.output_flags = OBS_SOURCE_AUDIO;
	au_filter.get_name = au_name;
	au_filter.create = au_create;
	au_filter.destroy = au_destroy;
	au_filter.update = au_update;
	au_filter.filter_audio = au_filter_audio;
	au_filter.get_properties = au_properties;
	au_filter.save = au_save_state;
	au_filter.video_tick = au_tick;
	obs_register_source(&au_filter);
}
