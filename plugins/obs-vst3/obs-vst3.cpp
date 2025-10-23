/* Copyright (c) 2025 pkv <pkv@obsproject.com>
 * This file is part of obs-vst3.
 *
 * This file uses the Steinberg VST3 SDK, which is licensed under MIT license.
 * See https://github.com/steinbergmedia/vst3sdk for details.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */

#include <sstream>
#include <thread>
#include <chrono>
#include <QCoreApplication>
#include <QMetaObject>
#include "VST3Scanner.h"
#include "VST3Plugin.h"
#ifdef __linux__
#include <obs-nix-platform.h>
#endif
#define MT_ obs_module_text
#define S_PLUGIN "vst3_plugin"
#define S_EDITOR "vst3_open_gui"
#define S_SIDECHAIN_SOURCE "sidechain_source"

#define TEXT_EDITOR MT_("VST3.Button")
#define TEXT_PLUGIN MT_("VST3.Plugin")
#define TEXT_SIDECHAIN_SOURCE  MT_("VST3.SidechainSource")

/* -------------------------------------------------------- */
#define do_log(level, format, ...) \
	blog(level, "[VST3: '%s'] " format, obs_source_get_name(ng->context), ## __VA_ARGS__)

#define warnvst3(format, ...) do_log(LOG_WARNING, format, ## __VA_ARGS__)
#define infovst3(format, ...) do_log(LOG_INFO, format, ## __VA_ARGS__)

#ifdef _DEBUG
#define debugvst3(format, ...) do_log(LOG_DEBUG, format, ## __VA_ARGS__)
#endif
/* -------------------------------------------------------- */

struct vst3_audio_info {
	uint32_t frames;
	uint64_t timestamp;
};

struct sidechain_prop_info {
	obs_property_t *sources;
	obs_source_t *parent;
};

/* -------------------- initial scanning ------------------ */

// global VST3Scanner
VST3Scanner *list;
static std::atomic<bool> vst3_scan_done = false;

bool retrieve_vst3_list()
{
	list = new VST3Scanner();
	if (!list->hasVST3()) {
		blog(LOG_INFO, "[VST3 Scanner] No VST3 were found");
		return false;
	}

	std::thread([=] {
		using clock = std::chrono::steady_clock;
		auto start = clock::now();
		if (list->scanForVST3Plugins()) {
			QMetaObject::invokeMethod(
				qApp,
				[=] {
					blog(LOG_INFO, "[VST3 Scanner] Available plugins:\n");
					for (const auto &plugin : list->pluginList) {
						blog(LOG_INFO, "   %s\n", plugin.name.c_str());
					}
				},
				Qt::QueuedConnection);
		} else {
			blog(LOG_INFO, "[VST3 Scanner] Error when scanning for VST3. Module will be unloaded.");
		}
		auto end = clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		blog(LOG_INFO, "[VST3 Scanner] Completed scan in %lld ms, found %zu plugins", (long long)ms,
		     list->pluginList.size());
		vst3_scan_done = true;
	}).detach();
	return true;
}

void free_vst3_list()
{
	delete list;
}

/* --------------------- utilities ----------------------- */
std::string toHex(const std::vector<uint8_t> &data)
{
	std::ostringstream oss;
	for (auto b : data)
		oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
	return oss.str();
}

std::vector<uint8_t> fromHex(const std::string &hex)
{
	std::vector<uint8_t> data;
	for (size_t i = 0; i + 1 < hex.size(); i += 2)
		data.push_back((uint8_t)std::stoi(hex.substr(i, 2), nullptr, 16));
	return data;
}

Steinberg::Vst::SpeakerArrangement obs_to_vst3_speaker_arrangement(speaker_layout layout)
{
	switch (layout) {
	case SPEAKERS_MONO:
		return Steinberg::Vst::SpeakerArr::kMono;
	case SPEAKERS_STEREO:
		return Steinberg::Vst::SpeakerArr::kStereo;
	case SPEAKERS_2POINT1: // Steinberg VST3 does not support 2.1 audio, so fallback to 3.0
		return Steinberg::Vst::SpeakerArr::k30Cine;
	case SPEAKERS_4POINT0:
		return Steinberg::Vst::SpeakerArr::k40Cine;
	case SPEAKERS_4POINT1:
		return Steinberg::Vst::SpeakerArr::k41Cine;
	case SPEAKERS_5POINT1:
		return Steinberg::Vst::SpeakerArr::k51;
	case SPEAKERS_7POINT1:
		return Steinberg::Vst::SpeakerArr::k71Music;
	case SPEAKERS_UNKNOWN:
	default:
		return Steinberg::Vst::SpeakerArr::kEmpty;
	}
}

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

static inline obs_source_t *get_sidechain(struct vst3_audio_data *ng)
{
	if (ng->weak_sidechain)
		return obs_weak_source_get_source(ng->weak_sidechain);
	return NULL;
}

/* --------------------- deque mgt -------------------------- */

static inline void clear_deque(struct deque *buf)
{
	deque_pop_front(buf, NULL, buf->size);
}

static void reset_data(struct vst3_audio_data *ng)
{
	for (size_t i = 0; i < ng->channels; i++) {
		clear_deque(&ng->input_buffers[i]);
		clear_deque(&ng->output_buffers[i]);
	}

	clear_deque(&ng->info_buffer);
}

static void reset_sidechain_data(struct vst3_audio_data *ng)
{
	for (size_t i = 0; i < ng->channels; i++)
		clear_deque(&ng->sc_input_buffers[i]);
}

/* --------------------- main functions ------------------- */

static const char *vst3_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return TEXT_PLUGIN;
}

static void sidechain_capture(void *data, obs_source_t *source, const struct audio_data *audio, bool muted);

static void vst3_destroy(void *data)
{
	struct vst3_audio_data *ng = (struct vst3_audio_data *)data;

	// stop any processing
	os_atomic_set_bool(&ng->bypass, true);
	ng->sidechain_enabled = false;

	/* sidechain stuff */
	if (ng->weak_sidechain) {
		obs_source_t *sidechain = get_sidechain(ng);
		if (sidechain) {
			obs_source_remove_audio_capture_callback(sidechain, sidechain_capture, ng);
			obs_source_release(sidechain);
		}
		obs_weak_source_release(ng->weak_sidechain);
	}

	if (ng->sc_resampler)
		audio_resampler_destroy(ng->sc_resampler);

	/* free buffers */
	for (size_t i = 0; i < ng->channels; i++) {
		deque_free(&ng->input_buffers[i]);
		deque_free(&ng->output_buffers[i]);
		deque_free(&ng->sc_input_buffers[i]);
	}
	bfree(ng->copy_buffers[0]);
	bfree(ng->sc_copy_buffers[0]);
	deque_free(&ng->info_buffer);
	da_free(ng->output_data);

	if (ng->plugin) {
		VST3Plugin *toDelete = nullptr;
		{
			std::lock_guard<std::mutex> lk(ng->plugin_state_mutex);
			toDelete = std::exchange(ng->plugin, nullptr);
		}
		if (toDelete) {
			toDelete->setProcessing(false);
			toDelete->deleteLater();
		}
		ng->plugin = nullptr;
	}

	delete ng;
}

/* main init function; in case of failure, the obs-vst3 filter is bypassed; if the new vst3 is empty, it just deletes
safely the previous vst3.*/
static bool init_VST3Plugin(void *data)
{
	auto *ng = static_cast<vst3_audio_data *>(data);

	// Re-entrancy guard
	if (ng->init_in_progress.test_and_set())
		return false; // already running

	struct ClearFlag {
		std::atomic_flag &f;
		~ClearFlag() { f.clear(); }
	} _guard{ng->init_in_progress};

	const std::string vst3_path = ng->vst3_path;
	const int sample_rate = ng->sample_rate;
	const int max_block =
		FRAME_SIZE; // 10 ms of audio @48 kHz, a little less than half the samples provided at each tick
	const auto class_id = ng->vst3_id;

	// Swap pointer under lock
	{
		std::unique_lock<std::mutex> lk(ng->plugin_state_mutex);
		os_atomic_set_bool(&ng->bypass, true);
		ng->noview = true;
		VST3Plugin *old = std::exchange(ng->plugin, nullptr);
		lk.unlock();

		if (old)
			delete old;
	}

	if (ng->vst3_id.empty() || ng->vst3_path.empty()) {
		infovst3("VST3 teardown: no plugin selected.");
		return true;
	}

	// Create + init the new instance
	std::unique_ptr<VST3Plugin> p(new VST3Plugin());
	// link base struct and vst class
	p->obsVst3Struct = ng;

	Steinberg::Vst::SpeakerArrangement arr = obs_to_vst3_speaker_arrangement(ng->layout);

	if (!p->init(class_id, vst3_path, sample_rate, max_block, arr)) {
		infovst3("Failed to initialize VST3 plugin at: %s\n", vst3_path.c_str());
		return false; // failed plugin deleted by RAII
	}

	// Create the editor view if the VST3 has a GUI
	if (!p->createView()) {
		infovst3("Failed to create editor view for VST3 plugin at: %s\n", vst3_path.c_str());
		ng->noview = true;
	} else {
		ng->noview = false;
	}

	// Publish the new instance + re-enable processing
	{
		std::unique_lock<std::mutex> lk(ng->plugin_state_mutex);
		ng->plugin = p.release();
		os_atomic_set_bool(&ng->bypass, false);
	}

	return true;
}

static void vst3_update(void *data, obs_data_t *settings)
{
	struct vst3_audio_data *ng = (struct vst3_audio_data *)data;

	if (!ng)
		return;

	std::string vst3_plugin_id(obs_data_get_string(settings, S_PLUGIN));
	if (vst3_plugin_id.empty()) {
		os_atomic_set_bool(&ng->bypass, true);
		if (ng->plugin) {
			ng->plugin->setProcessing(false);
			ng->plugin->hideEditor();
			ng->plugin->deactivateComponent();
#ifdef __linux__
			ng->plugin->stopRunLoop();
#endif
			init_VST3Plugin(ng);
		}
		ng->vst3_id.clear();
		ng->vst3_path.clear();
		ng->vst3_name.clear();
		ng->has_sidechain = false;
		return;
	}

	bool initial_load = ng->vst3_id.empty();

	if (ng->vst3_id != vst3_plugin_id && !vst3_plugin_id.empty()) {
		if (!initial_load) {
			// we are swapping VST3s so stop processing
			if (ng->plugin) {
				ng->plugin->setProcessing(false);
				ng->plugin->hideEditor();
				ng->plugin->deactivateComponent();
#ifdef __linux__
				ng->plugin->stopRunLoop();
#endif
				os_atomic_set_bool(&ng->bypass, true);
				if (ng->output_data.array) {
					da_free(ng->output_data);
				}
			}
		}
		// retrieve path and name from VST3Scanner or from settings
		ng->vst3_id = vst3_plugin_id;
		if (!list->get_path_by_id(vst3_plugin_id).empty()) {
			ng->vst3_path = list->get_path_by_id(vst3_plugin_id);
			ng->vst3_name = list->get_name_by_id(vst3_plugin_id);
		} else {
			ng->vst3_path = obs_data_get_string(settings, "vst3_path");
			ng->vst3_name = obs_data_get_string(settings, "vst3_name");
		}

		infovst3("filter applied: %s, path: %s", ng->vst3_name.c_str(), ng->vst3_path.c_str());

		if (init_VST3Plugin(ng)) {
			ng->plugin->setProcessing(true);
			os_atomic_set_bool(&ng->bypass, false);
			ng->has_sidechain = ng->plugin->sidechainNumChannels != 0;
		} else {
			infovst3("VST3 failure; plugin deactivated.");
			os_atomic_set_bool(&ng->bypass, true);
		}
	}

	// Only load the state the first time the filter is loaded
	if (ng && ng->plugin && initial_load) {
		const char *hexComp = obs_data_get_string(settings, "vst3_state");
		const char *hexCtrl = obs_data_get_string(settings, "vst3_ctrl_state");
		if (hexComp && *hexComp) {
			std::vector<uint8_t> comp = fromHex(hexComp);
			std::vector<uint8_t> ctrl;
			if (hexCtrl && *hexCtrl)
				ctrl = fromHex(hexCtrl);
			ng->plugin->loadStates(comp, ctrl);
		}
	}
	// Sidechain specific code starts here, cf obs-filters/compressor-filter.c for the logic. The sidechain swap is
	// done in 2 steps with the swapping proper in the video tick callback after a 3 sec wait.
	if (ng && !ng->has_sidechain)
		return;

	if (ng && ng->plugin) {
		// we support sidechain only for mono or stereo buses (sanity check)
		ng->sc_channels = ng->plugin->sidechainNumChannels;
		ng->sidechain_enabled = !(ng->sc_channels > 2 || ng->sc_channels == 0);
	}

	std::string sidechain_name(obs_data_get_string(settings, S_SIDECHAIN_SOURCE));
	bool valid_sidechain = sidechain_name != "none" && !sidechain_name.empty();
	obs_weak_source_t *old_weak_sidechain = NULL;

	{
		std::lock_guard<std::mutex> lock(ng->sidechain_update_mutex);
		if (!valid_sidechain) {
			if (ng->weak_sidechain) {
				old_weak_sidechain = ng->weak_sidechain;
				ng->weak_sidechain = NULL;
			}

			ng->sidechain_name = "";
			ng->sidechain_enabled = false;
		} else {
			if (ng->sidechain_name.empty() || ng->sidechain_name != sidechain_name) {
				if (ng->weak_sidechain) {
					old_weak_sidechain = ng->weak_sidechain;
					ng->weak_sidechain = NULL;
				}

				ng->sidechain_name = sidechain_name;
				ng->sidechain_check_time = os_gettime_ns() - 3000000000;
			}
			ng->sidechain_enabled = true;
		}
	}

	if (old_weak_sidechain) {
		obs_source_t *old_sidechain = obs_weak_source_get_source(old_weak_sidechain);

		if (old_sidechain) {
			obs_source_remove_audio_capture_callback(old_sidechain, sidechain_capture, ng);
			obs_source_release(old_sidechain);
		}

		obs_weak_source_release(old_weak_sidechain);
	}
}

static void *vst3_create(obs_data_t *settings, obs_source_t *filter)
{
	auto *ng = new vst3_audio_data();

	if (!ng)
		return NULL;

	ng->context = filter;
	ng->vst3_id = {};
	ng->vst3_name = {};
	ng->vst3_path = {};

	audio_t *audio = obs_get_audio();
	const struct audio_output_info *aoi = audio_output_get_info(audio);

	size_t frames = (size_t)FRAME_SIZE;
	ng->frames = frames;
	size_t channels = audio_output_get_channels(audio);
	ng->channels = channels;
	ng->sample_rate = audio_output_get_sample_rate(audio);
	ng->layout = aoi->speakers;
	ng->has_sidechain = false;
	ng->sidechain_enabled = false;
	ng->noview = true;

	ng->latency = 1000000000LL / (1000 / BUFFER_SIZE_MSEC);

	/* allocate copy buffers(which are contiguous for the channels) */
	ng->copy_buffers[0] = (float *)bmalloc((size_t)FRAME_SIZE * channels * sizeof(float));
	ng->sc_copy_buffers[0] = (float *)bmalloc(FRAME_SIZE * channels * sizeof(float));

	for (size_t c = 1; c < channels; ++c) {
		ng->copy_buffers[c] = ng->copy_buffers[c - 1] + frames;
		ng->sc_copy_buffers[c] = ng->sc_copy_buffers[c - 1] + frames;
	}

	/* reserve circular buffers */
	for (size_t i = 0; i < channels; i++) {
		deque_reserve(&ng->input_buffers[i], frames * sizeof(float));
		deque_reserve(&ng->output_buffers[i], frames * sizeof(float));
		deque_reserve(&ng->sc_input_buffers[i], frames * sizeof(float));
	}

	os_atomic_set_bool(&ng->bypass, true);

	vst3_update(ng, settings);
	return ng;
}

void vst3_save(void *data, obs_data_t *settings)
{
	vst3_audio_data *ng = (vst3_audio_data *)data;
	if (ng && ng->plugin) {
		std::vector<uint8_t> comp, ctrl;
		if (ng->plugin->saveStates(comp, ctrl)) {
			obs_data_set_string(settings, "vst3_state", toHex(comp).c_str());
			if (!ctrl.empty())
				obs_data_set_string(settings, "vst3_ctrl_state", toHex(ctrl).c_str());
			else
				obs_data_set_string(settings, "vst3_ctrl_state", "");
		}
		// We store these because the filter might load before VST3s list has been populated with this info.
		obs_data_set_string(settings, "vst3_path", ng->vst3_path.c_str());
		obs_data_set_string(settings, "vst3_name", ng->vst3_name.c_str());
	}
}

/* -------------- audio processing (incl. sc) --------------- */
static inline void preprocess_input(struct vst3_audio_data *ng)
{
	std::unique_lock<std::mutex> lk(ng->plugin_state_mutex);
	int num_channels = (int)ng->channels;
	int sc_num_channels = (int)ng->sc_channels;
	int frames = (int)ng->frames;
	VST3Plugin *plugin = ng->plugin;
	size_t segment_size = ng->frames * sizeof(float);
	bool has_sc = ng->has_sidechain;
	bool sc_enabled = ng->sidechain_enabled;
	lk.unlock();

	if (has_sc && sc_enabled) {
		for (int i = 0; i < num_channels; i++) {
			if (ng->sc_input_buffers[i].size < segment_size)
				deque_push_back_zero(&ng->sc_input_buffers[i], segment_size);
		}
	}

	/* Pop from input deque */
	for (int i = 0; i < num_channels; i++)
		deque_pop_front(&ng->input_buffers[i], ng->copy_buffers[i], ng->frames * sizeof(float));

	if (has_sc && sc_enabled) {
		for (int i = 0; i < num_channels; i++)
			deque_pop_front(&ng->sc_input_buffers[i], ng->sc_copy_buffers[i], ng->frames * sizeof(float));
	}

	// Copy input OBS buffer to VST input buffers
	for (int ch = 0; ch < num_channels; ++ch) {
		float *inBuf = (float *)ng->copy_buffers[ch];
		float *vstIn = plugin->channelBuffer32(Steinberg::Vst::kInput, ch);
		if (inBuf && vstIn)
			memcpy(vstIn, inBuf, frames * sizeof(float));
	}

	// Copy sidechain input OBS buffer to VST input buffers
	if (has_sc && sc_enabled) {
		bool needs_resampling = ng->channels != ng->sc_channels &&
					(ng->sc_channels == 1 || ng->sc_channels == 2);
		if (needs_resampling && ng->sc_resampler) {
			uint8_t *resampled[2] = {nullptr, nullptr};
			uint32_t out_frames;
			uint64_t ts_offset;

			if (audio_resampler_resample(ng->sc_resampler, resampled, &out_frames, &ts_offset,
						     (const uint8_t **)ng->sc_copy_buffers, (uint32_t)ng->frames)) {
				for (int ch = 0; ch < sc_num_channels; ++ch) {
					float *inBuf = (float *)resampled[ch];
					float *vstIn = plugin->sidechannelBuffer32(Steinberg::Vst::kInput, ch);
					if (inBuf && vstIn)
						memcpy(vstIn, inBuf, out_frames * sizeof(float));
				}
			}
		} else {
			for (int ch = 0; ch < sc_num_channels; ++ch) {
				float *inBuf = ng->sc_copy_buffers[ch];
				float *vstIn = plugin->sidechannelBuffer32(Steinberg::Vst::kInput, ch);
				if (inBuf && vstIn)
					memcpy(vstIn, inBuf, frames * sizeof(float));
			}
		}
	}
}

static inline void process(struct vst3_audio_data *ng)
{
	int num_channels = (int)ng->channels;
	int frames = (int)ng->frames;
	std::unique_lock<std::mutex> lk(ng->plugin_state_mutex);
	VST3Plugin *plugin = ng->plugin;
	bool bypass = os_atomic_load_bool(&ng->bypass);
	lk.unlock();

	if (bypass)
		return;

	preprocess_input(ng);
	plugin->process(frames);

	/* Retrieve processed buffers */
	for (int ch = 0; ch < num_channels; ++ch) {
		uint8_t *outBuf = (uint8_t *)ng->copy_buffers[ch];
		float *vstOut = plugin->channelBuffer32(Steinberg::Vst::kOutput, ch);
		if (outBuf && vstOut)
			memcpy(outBuf, vstOut, frames * sizeof(float));
	}
	/* Push to output deque */
	for (size_t i = 0; i < ng->channels; i++)
		deque_push_back(&ng->output_buffers[i], ng->copy_buffers[i], ng->frames * sizeof(float));
}

static struct obs_audio_data *vst3_filter_audio(void *data, struct obs_audio_data *audio)
{
	vst3_audio_data *ng = (vst3_audio_data *)data;
	std::unique_lock<std::mutex> lk(ng->plugin_state_mutex);
	struct vst3_audio_info info;
	size_t segment_size = ng->frames * sizeof(float);
	size_t out_size;
	VST3Plugin *p = ng->plugin;
	bool bypass = os_atomic_load_bool(&ng->bypass);
	lk.unlock();

	if (bypass || !p)
		return audio;

	if (!p->numEnabledOutAudioBuses)
		return audio;

	/* -----------------------------------------------
	 * If timestamp has dramatically changed, consider it a new stream of
	 * audio data. Clear all circular buffers to prevent old audio data
	 * from being processed as part of the new data. */
	if (ng->last_timestamp) {
		int64_t diff = llabs((int64_t)ng->last_timestamp - (int64_t)audio->timestamp);

		if (diff > 1000000000LL)
			reset_data(ng);
	}

	ng->last_timestamp = audio->timestamp;

	/* -----------------------------------------------
	 * push audio packet info (timestamp/frame count) to info deque */
	info.frames = audio->frames;
	info.timestamp = audio->timestamp;
	deque_push_back(&ng->info_buffer, &info, sizeof(info));

	/* -----------------------------------------------
	 * push back current audio data to input deque */
	for (size_t i = 0; i < ng->channels; i++)
		deque_push_back(&ng->input_buffers[i], audio->data[i], audio->frames * sizeof(float));

	/* -----------------------------------------------
	 * pop/process each 10ms segments, push back to output deque */
	while (ng->input_buffers[0].size >= segment_size)
		process(ng);

	/* -----------------------------------------------
	 * peek front of info deque, check to see if we have enough to
	 * pop the expected packet size, if not, return null */
	memset(&info, 0, sizeof(info));
	deque_peek_front(&ng->info_buffer, &info, sizeof(info));
	out_size = info.frames * sizeof(float);

	if (ng->output_buffers[0].size < out_size)
		return NULL;

	/* -----------------------------------------------
	 * if there's enough audio data buffered in the output deque,
	 * pop and return a packet */
	deque_pop_front(&ng->info_buffer, NULL, sizeof(info));
	da_resize(ng->output_data, out_size * ng->channels);

	for (size_t i = 0; i < ng->channels; i++) {
		ng->output_audio.data[i] = (uint8_t *)&ng->output_data.array[i * out_size];

		deque_pop_front(&ng->output_buffers[i], ng->output_audio.data[i], out_size);
	}

	ng->running_sample_count += info.frames;
	ng->system_time = os_gettime_ns();
	ng->output_audio.frames = info.frames;
	ng->output_audio.timestamp = info.timestamp - ng->latency;
	return &ng->output_audio;
}

static void sidechain_capture(void *data, obs_source_t *source, const struct audio_data *audio, bool muted)
{
	UNUSED_PARAMETER(source);
	UNUSED_PARAMETER(muted);
	struct vst3_audio_data *ng = (struct vst3_audio_data *)data;
	std::unique_lock<std::mutex> lk(ng->plugin_state_mutex);
	VST3Plugin *p = ng->plugin;
	bool bypass = os_atomic_load_bool(&ng->bypass);
	bool sc_enabled = ng->sidechain_enabled;
	lk.unlock();

	if (bypass || !p)
		return;

	if (!sc_enabled)
		return;
	if (ng->sc_channels != 1 && ng->sc_channels != 2)
		return;
	/* -----------------------------------------------
	 * If timestamp has dramatically changed, consider it a new stream of
	 * audio data. Clear all circular buffers to prevent old audio data
	 * from being processed as part of the new data. */
	if (ng->sc_last_timestamp) {
		int64_t diff = llabs((int64_t)ng->sc_last_timestamp - (int64_t)audio->timestamp);

		if (diff > 1000000000LL)
			reset_sidechain_data(ng);
	}

	ng->sc_last_timestamp = audio->timestamp;

	/* -----------------------------------------------
	 * push back current audio data to input deque */
	for (size_t i = 0; i < ng->channels; i++)
		deque_push_back(&ng->sc_input_buffers[i], audio->data[i], audio->frames * sizeof(float));
}

static void vst3_tick(void *data, float seconds)
{
	struct vst3_audio_data *ng = (struct vst3_audio_data *)data;
	std::unique_lock<std::mutex> lk(ng->plugin_state_mutex);
	bool has_sc = ng->has_sidechain;
	lk.unlock();

	if (!has_sc)
		return;

	// written after obs-filters/compressor-filter.c for the sidechain logic
	std::string new_name = {};

	{
		std::lock_guard<std::mutex> lock(ng->sidechain_update_mutex);
		if (!ng->sidechain_name.empty() && !ng->weak_sidechain) {
			uint64_t t = os_gettime_ns();

			if (t - ng->sidechain_check_time > 3000000000) {
				new_name = ng->sidechain_name;
				ng->sidechain_check_time = t;
			}
		}
	}

	if (!new_name.empty()) {
		obs_source_t *sidechain = obs_get_source_by_name(new_name.c_str());
		obs_weak_source_t *weak_sidechain = sidechain ? obs_source_get_weak_source(sidechain) : NULL;

		{
			std::lock_guard<std::mutex> lock(ng->sidechain_update_mutex);
			if (!ng->sidechain_name.empty() && ng->sidechain_name == new_name) {
				ng->weak_sidechain = weak_sidechain;
				weak_sidechain = NULL;
			}
		}

		if (sidechain) {
			ng->sidechain_enabled = true;
			// downmix or upmix if channel count is mismatched
			bool needs_resampling = ng->channels != ng->sc_channels;
			if (needs_resampling) {
				struct resample_info src, dst;
				src.samples_per_sec = ng->sample_rate;
				src.format = AUDIO_FORMAT_FLOAT_PLANAR;
				src.speakers = convert_speaker_layout((uint8_t)ng->channels);

				dst.samples_per_sec = ng->sample_rate;
				dst.format = AUDIO_FORMAT_FLOAT_PLANAR;
				dst.speakers = convert_speaker_layout((uint8_t)ng->sc_channels);
				if (ng->sc_resampler)
					audio_resampler_destroy(ng->sc_resampler);

				ng->sc_resampler = audio_resampler_create(&dst, &src);
			}
			obs_source_add_audio_capture_callback(sidechain, sidechain_capture, ng);

			obs_weak_source_release(weak_sidechain);
			obs_source_release(sidechain);
		}
	}
	UNUSED_PARAMETER(seconds);
}

/* ---------------- properties functions --------------------- */

static bool vst3_show_gui_callback(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	vst3_audio_data *ng = static_cast<vst3_audio_data *>(data);
	if (!ng)
		return false;

	VST3Plugin *plugin = ng->plugin;
	bool noview = ng->noview;

	if (!plugin)
		return false;

	if (noview)
		return false;

	if (!plugin->isEditorVisible())
		plugin->showEditor();
	else
		plugin->hideEditor();

	return true;
}

static bool add_sources(void *data, obs_source_t *source)
{
	struct sidechain_prop_info *info = (struct sidechain_prop_info *)data;
	uint32_t caps = obs_source_get_output_flags(source);

	// if not an audio source or if it is the parent, don't add
	if (source == info->parent)
		return true;
	if ((caps & OBS_SOURCE_AUDIO) == 0)
		return true;

	const char *name = obs_source_get_name(source);
	obs_property_list_add_string(info->sources, name, name);
	return true;
}

bool sidechain_properties_cb(void *priv, obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(settings);
	struct vst3_audio_data *ng = (struct vst3_audio_data *)priv;
	std::unique_lock<std::mutex> lk(ng->plugin_state_mutex);
	bool has_sc = ng->has_sidechain;
	lk.unlock();

	obs_property_t *p = obs_properties_get(props, S_SIDECHAIN_SOURCE);
	if (has_sc) {
		obs_source_t *parent = obs_filter_get_parent(ng->context);
		obs_property_list_clear(p);
		obs_property_list_add_string(p, obs_module_text("None"), "none");
		struct sidechain_prop_info info = {p, parent};
		obs_enum_sources(add_sources, &info);
		obs_property_set_visible(p, true);
	} else {
		obs_property_set_visible(p, false);
	}
	return true;
}

static obs_properties_t *vst3_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *sources;
	obs_property_t *vst3list =
		obs_properties_add_list(props, S_PLUGIN, TEXT_PLUGIN, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(vst3list, obs_module_text("VST3.Select"), "");
	for (const auto &plugin : list->pluginList) {
		const bool multi = list->bundle_has_multiple_classes(plugin.path);

		std::string display = multi ? plugin.name + " (" + plugin.pluginName + ")" : plugin.name;
		std::string value = plugin.id;
		obs_property_list_add_string(vst3list, display.c_str(), value.c_str());
	}
	obs_properties_add_button(props, S_EDITOR, obs_module_text(TEXT_EDITOR), vst3_show_gui_callback);

	sources = obs_properties_add_list(props, S_SIDECHAIN_SOURCE, TEXT_SIDECHAIN_SOURCE, OBS_COMBO_TYPE_LIST,
					  OBS_COMBO_FORMAT_STRING);
	obs_property_set_modified_callback2(vst3list, sidechain_properties_cb, data);

	return props;
}

/* ======================== module =========================*/
const char *PLUGIN_VERSION = "1.0.0";
OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-vst3", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "VST3 audio plugin";
}

void register_vst3_source()
{
	struct obs_source_info vst3_filter = {};
	vst3_filter.id = "vst3_filter";
	vst3_filter.type = OBS_SOURCE_TYPE_FILTER;
	vst3_filter.output_flags = OBS_SOURCE_AUDIO;
	vst3_filter.get_name = vst3_name;
	vst3_filter.create = vst3_create;
	vst3_filter.destroy = vst3_destroy;
	vst3_filter.update = vst3_update;
	vst3_filter.filter_audio = vst3_filter_audio;
	vst3_filter.get_properties = vst3_properties;
	vst3_filter.save = vst3_save;
	vst3_filter.video_tick = vst3_tick;
	obs_register_source(&vst3_filter);
}

bool obs_module_load(void)
{
#if defined(__linux__)
	if (obs_get_nix_platform() != OBS_NIX_PLATFORM_X11_EGL) {
		blog(LOG_INFO, "OBS-VST3 filter disabled due to X11 requirement.");
		return false;
	}
#endif
	if (!retrieve_vst3_list())
		blog(LOG_INFO, "OBS-VST3: you'll have to install VST3s in orer to use this filter.");

	register_vst3_source();
	blog(LOG_INFO, "OBS-VST3 filter loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_post_load(void)
{
	for (int i = 0; i < 100 && !vst3_scan_done; ++i)
		os_sleep_ms(100);
}

void obs_module_unload()
{
	free_vst3_list();
}
