/******************************************************************************
    Copyright (C) 2025-2026 pkv <pkv@obsproject.com>
    This file is part of obs-vst3.
    It uses the Steinberg VST3 SDK, which is licensed under MIT license.
    See https://github.com/steinbergmedia/vst3sdk for details.
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#include "obs-vst3.h"
// on linux Qt must be included before X11 headers due to a pesky redefinition
#include <QCoreApplication>
#include <QMetaObject>
#include "VST3Scanner.h"
#include "VST3Plugin.h"
#include "VST3HostApp.h"
#ifdef __linux__
#include "editor/linux/RunLoopImpl.h"
#endif

#include <sstream>
#include <thread>
#include <chrono>
#include <unordered_set>

#define MT_ obs_module_text
#define S_PLUGIN "vst3_plugin"
#define S_EDITOR "vst3_open_gui"
#define S_SIDECHAIN_SOURCE "sidechain_source"
#define S_NOGUI "vst3_noview"
#define S_ERR "vst3_error"
#define S_SCAN "vst3_scan"

#define TEXT_EDITOR MT_("VST3.Button")
#define TEXT_PLUGIN MT_("VST3.Plugin")
#define TEXT_SIDECHAIN_SOURCE  MT_("VST3.SidechainSource")
#define TEXT_NOGUI MT_("VST3.NOGUI")
#define TEXT_ERR MT_("VST3.Init.Fail")
#define TEXT_SCAN MT_("VST3.Scan.Ongoing")

/* -------------------------------------------------------- */
#define do_log(level, format, ...) \
	blog(level, "[VST3 filter ('%s')]: " format, obs_source_get_name(vd->context), ## __VA_ARGS__)

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
/* ----------------- global host & runloop ---------------- */

VST3HostApp *g_host_app = nullptr;
#ifdef __linux__
RunLoopImpl *g_run_loop = nullptr;
Display *g_display = nullptr;
#endif

void load_host()
{
	g_host_app = new VST3HostApp();
#ifdef __linux__
	g_display = XOpenDisplay(nullptr);
	g_run_loop = new RunLoopImpl(g_display);
	g_host_app->setRunLoop(g_run_loop);
#endif
}

void unload_host()
{
#ifdef __linux__
	if (g_run_loop)
		g_run_loop->stop();
	delete g_run_loop;
#endif
	delete g_host_app;
}
/* -------------------- initial scanning ------------------ */

// global VST3Scanner
VST3Scanner *list;
std::atomic<bool> vst3_scan_done;

static void vst3_cache_save()
{
	if (!list)
		return;

	char *path = obs_module_config_path(NULL);
	os_mkdirs(path);
	bfree(path);

	char *filepath = obs_module_get_config_path(obs_current_module(), "vst3list.json");

	obs_data_t *root = obs_data_create();
	obs_data_array_t *arr = obs_data_array_create();

	for (const auto &p : list->pluginList) {
		obs_data_t *obj = obs_data_create();
		obs_data_set_string(obj, "name", p.name.c_str());
		obs_data_set_string(obj, "id", p.id.c_str());
		obs_data_set_string(obj, "path", p.path.c_str());
		obs_data_set_string(obj, "pluginName", p.pluginName.c_str());
		obs_data_set_bool(obj, "discardable", p.discardable);
		obs_data_array_push_back(arr, obj);
		obs_data_release(obj);
	}

	obs_data_set_int(root, "version", 1);
	obs_data_set_array(root, "plugins", arr);
	obs_data_array_release(arr);

	obs_data_save_json_safe(root, filepath, "tmp", "bak");
	obs_data_release(root);
	bfree(filepath);
}

static bool vst3_cache_load()
{
	char *path = obs_module_config_path("vst3list.json");
	if (!path)
		return false;

	obs_data_t *root = obs_data_create_from_json_file_safe(path, "bak");
	bfree(path);
	if (!root)
		return false;

	obs_data_array_t *arr = obs_data_get_array(root, "plugins");
	if (!arr) {
		obs_data_release(root);
		return false;
	}

	list->pluginList.clear();
	list->classCount.clear();

	std::unordered_map<std::string, ModuleCache> modules;

	size_t count = obs_data_array_count(arr);
	for (size_t i = 0; i < count; ++i) {
		obs_data_t *obj = obs_data_array_item(arr, i);
		VST3ClassInfo ci;

		ci.name = obs_data_get_string(obj, "name");
		ci.id = obs_data_get_string(obj, "id");
		ci.path = obs_data_get_string(obj, "path");
		ci.pluginName = obs_data_get_string(obj, "pluginName");
		ci.discardable = obs_data_get_bool(obj, "discardable");

		obs_data_release(obj);

		if (ci.path.empty() || !std::filesystem::exists(ci.path))
			continue;

		auto &m = modules[ci.path];
		if (ci.discardable)
			m.discardable = true;

		m.classes.push_back(std::move(ci));
	}

	obs_data_array_release(arr);
	obs_data_release(root);

	// we need to update the json list in case VST3s have been removed or newly installed
	std::unordered_set<std::string> cachedPaths;
	cachedPaths.reserve(modules.size());

	for (auto &kv : modules)
		cachedPaths.insert(kv.first);

	list->updateModulesList(modules, cachedPaths);

	for (auto &[modulePath, m] : modules) {
		// if a module has the flag kClassesDiscardable, the SDK compels us to do a full load from binary, duh ...
		if (m.discardable) {
			list->addModuleClasses(modulePath);
		} else {
			for (auto &ci : m.classes) {
				list->pluginList.push_back(ci);
				++list->classCount[modulePath];
			}
		}
	}

	list->sort();

	return !list->pluginList.empty();
}

bool retrieve_vst3_list()
{
	vst3_scan_done.store(false, std::memory_order_relaxed);
	list = new VST3Scanner();
	if (!list->hasVST3()) {
		blog(LOG_INFO, "[VST3 Scanner] No VST3 were found");
		return false;
	}

	// The VST3 enumeration can take several seconds slowing down OBS startup. So we do it in its own thread.
	std::thread([] {
		using clock = std::chrono::steady_clock;
		auto start = clock::now();

		bool loaded_from_cache = vst3_cache_load();
		if (!loaded_from_cache) {
			if (!list->scanForVST3Plugins()) {
				blog(LOG_INFO, "[VST3 Scanner] Error when scanning for VST3. Module will be unloaded.");
			}
		}

		blog(LOG_INFO, "[VST3 Scanner] Available plugins:");
		for (const auto &plugin : list->pluginList) {
			blog(LOG_INFO, "[VST3 Scanner]   %s", plugin.name.c_str());
		}

		auto end = clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		blog(LOG_INFO, "[VST3 Scanner] %s in %lld ms, found %zu plugins",
		     loaded_from_cache ? "Loaded cache & non-cacheable VST3s" : "Completed scan", (long long)ms,
		     list->pluginList.size());

		vst3_cache_save();
		vst3_scan_done.store(true, std::memory_order_relaxed);
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

/* --------------------- deque mgt -------------------------- */

static inline void clear_deque(struct deque *buf)
{
	deque_pop_front(buf, NULL, buf->size);
}

static void reset_data(struct vst3_audio_data *vd)
{
	for (size_t i = 0; i < vd->channels; i++) {
		clear_deque(&vd->input_buffers[i]);
		clear_deque(&vd->output_buffers[i]);
	}

	clear_deque(&vd->info_buffer);
}

static void reset_sidechain_data(struct vst3_audio_data *vd)
{
	std::lock_guard<std::mutex> lock(vd->sidechain_mutex);
	for (size_t i = 0; i < vd->channels; i++)
		clear_deque(&vd->sc_input_buffers[i]);
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
	struct vst3_audio_data *vd = (struct vst3_audio_data *)data;
	vd->bypass.store(true, std::memory_order_relaxed);
	vd->sidechain_enabled.store(false, std::memory_order_relaxed);

	if (vd->weak_sidechain) {
		obs_source_t *sidechain = obs_weak_source_get_source(vd->weak_sidechain);
		if (sidechain) {
			obs_source_remove_audio_capture_callback(sidechain, sidechain_capture, vd);
			obs_source_release(sidechain);
		}
		obs_weak_source_release(vd->weak_sidechain);
	}

	std::atomic_store(&vd->sc_resampler, std::shared_ptr<audio_resampler>{});
	vd->sc_last_timestamp = 0;

	for (size_t i = 0; i < vd->channels; i++) {
		deque_free(&vd->input_buffers[i]);
		deque_free(&vd->output_buffers[i]);
		{
			std::lock_guard<std::mutex> lock(vd->sidechain_mutex);
			deque_free(&vd->sc_input_buffers[i]);
		}
	}
	bfree(vd->copy_buffers[0]);
	bfree(vd->sc_copy_buffers[0]);
	deque_free(&vd->info_buffer);
	da_free(vd->output_data);

	auto plugin = std::atomic_load(&vd->plugin);
	if (plugin) {
		plugin->setProcessing(false);
		std::atomic_store(&vd->plugin, std::shared_ptr<VST3Plugin>{});
	}

	delete vd;
}

static void teardown_sidechain(vst3_audio_data *vd, obs_data *settings)
{
	if (!vd)
		return;

	if (!vd->weak_sidechain || vd->sidechain_name.empty())
		return;
	vd->sidechain_enabled.store(false, std::memory_order_relaxed);

	obs_weak_source_t *old_weak = nullptr;
	{
		std::lock_guard<std::mutex> lock(vd->sidechain_update_mutex);
		if (vd->weak_sidechain) {
			old_weak = vd->weak_sidechain;
			vd->weak_sidechain = nullptr;
		}
		vd->sidechain_name.clear();
		obs_data_set_string(settings, S_SIDECHAIN_SOURCE, NULL);
	}

	if (old_weak) {
		obs_source_t *old_source = obs_weak_source_get_source(old_weak);

		if (old_source) {
			obs_source_remove_audio_capture_callback(old_source, sidechain_capture, vd);

			obs_source_release(old_source);
		}

		obs_weak_source_release(old_weak);
	}

	vd->sc_last_timestamp = 0;
	std::atomic_store(&vd->sc_resampler, std::shared_ptr<audio_resampler>{});
}

static void destroy_current_VST3Plugin(vst3_audio_data *vd, obs_data *settings)
{
	if (!vd)
		return;

	vd->bypass.store(true, std::memory_order_relaxed);
	auto plugin = std::atomic_load(&vd->plugin);
	if (!plugin)
		return;

	std::atomic_store(&vd->plugin, std::shared_ptr<VST3Plugin>{});

	plugin->setProcessing(false);
	plugin->hideEditor();
	plugin->deactivateComponent();
	vd->noview.store(true, std::memory_order_relaxed);

	if (vd->weak_sidechain)
		teardown_sidechain(vd, settings);
}

static bool create_VST3Plugin(vst3_audio_data *vd)
{
	if (!vd)
		return false;

	if (vd->vst3_id.empty() || vd->vst3_path.empty())
		return true;

	const std::string class_id = vd->vst3_id;
	const std::string vst3_path = vd->vst3_path;
	const int sample_rate = vd->sample_rate;
	const int max_block = FRAME_SIZE;

	Steinberg::Vst::SpeakerArrangement arr = obs_to_vst3_speaker_arrangement(vd->layout);

	VST3Plugin *raw = new VST3Plugin();
	raw->obsVst3Struct = vd;

	if (!raw->init(class_id, vst3_path, sample_rate, max_block, arr)) {
		infovst3("Failed to initialize VST3 plugin %s", raw->name.c_str());
		vd->last_init_failed = true;
		delete raw;
		return false;
	} else {
		infovst3("Plugin %s was successfully initialized.", raw->name.c_str());
	}

	// not all VST3s have a GUI
	if (!raw->createView()) {
		infovst3("Failed to create editor view for plugin at: %s", vst3_path.c_str());
		vd->noview.store(true, std::memory_order_relaxed);
	} else {
		vd->noview.store(false, std::memory_order_relaxed);
		infovst3("Plugin %s has a GUI.", raw->name.c_str());
	}

	// Wrap raw pointer in shared_ptr with deleteLater deleter
	auto plugin = std::shared_ptr<VST3Plugin>(raw, [](VST3Plugin *p) {
		if (p) {
			p->deleteLater();
		}
	});

	std::atomic_store(&vd->plugin, plugin);
	vd->bypass.store(false, std::memory_order_relaxed);

	return true;
}

/* main init function; in case of failure, the obs-vst3 filter is bypassed; if the new vst3 is empty, it just deletes
safely the previous vst3.*/
static bool init_VST3Plugin(void *data, obs_data *settings)
{
	auto *vd = static_cast<vst3_audio_data *>(data);
	if (!vd)
		return false;

	// Reentrancy guard
	if (vd->init_in_progress.test_and_set())
		return false;

	struct ClearFlag {
		std::atomic_flag &f;
		~ClearFlag() { f.clear(); }
	} _guard{vd->init_in_progress};

	destroy_current_VST3Plugin(vd, settings);

	return create_VST3Plugin(vd);
}

static void sidechain_swap(vst3_audio_data *vd, obs_data *settings)
{
	if (!vd)
		return;

	if (!vd->has_sidechain.load(std::memory_order_relaxed))
		return;

	vd->sidechain_enabled.store(false, std::memory_order_relaxed);

	std::string sidechain_name(obs_data_get_string(settings, S_SIDECHAIN_SOURCE));
	bool valid_sidechain = sidechain_name != "none" && !sidechain_name.empty();
	obs_weak_source_t *old_weak_sidechain = NULL;

	{
		std::lock_guard<std::mutex> lock(vd->sidechain_update_mutex);
		if (!valid_sidechain) {
			{
				if (vd->weak_sidechain) {
					old_weak_sidechain = vd->weak_sidechain;
					vd->weak_sidechain = NULL;
				}
				vd->sidechain_name = "";
			}
		} else {

			if (vd->sidechain_name.empty() || vd->sidechain_name != sidechain_name) {
				if (vd->weak_sidechain) {
					old_weak_sidechain = vd->weak_sidechain;
					vd->weak_sidechain = NULL;
				}
				vd->sidechain_name = sidechain_name;
				vd->sidechain_check_time = os_gettime_ns() - 3000000000;
			}
		}
	}
	vd->sidechain_enabled.store(true, std::memory_order_relaxed);

	if (old_weak_sidechain) {
		obs_source_t *old_sidechain = obs_weak_source_get_source(old_weak_sidechain);

		if (old_sidechain) {
			obs_source_remove_audio_capture_callback(old_sidechain, sidechain_capture, vd);
			obs_source_release(old_sidechain);
		}

		obs_weak_source_release(old_weak_sidechain);
	}
}

// Our logic differs significantly from a DAW. We indeed allow swapping of VST3s which may or may not have an sc.
// 2 or 3 threads are then involved (UI, audio and possibly video due to the trick of sidechain audio capture
// leveraging video_tick). We've taken great care to implement Ross Bencina's cardinal rule for audio programming.
// http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing but for sidechain there's
// still a mutex.
static void vst3_update(void *data, obs_data_t *settings)
{
	auto *vd = static_cast<struct vst3_audio_data *>(data);
	if (!vd)
		return;

	std::string vst3_plugin_id(obs_data_get_string(settings, S_PLUGIN));

	if (vst3_plugin_id.empty()) {
		vd->bypass.store(true, std::memory_order_relaxed);
		vd->vst3_id.clear();
		vd->vst3_path.clear();
		vd->vst3_name.clear();
		vd->has_sidechain.store(false, std::memory_order_relaxed);
		destroy_current_VST3Plugin(vd, settings);

		return;
	}

	auto plugin = std::atomic_load(&vd->plugin);
	bool initial_load = vd->vst3_id.empty() && !plugin;
	bool is_swap = (vd->vst3_id != vst3_plugin_id);

	if (is_swap) {
		if (!initial_load)
			destroy_current_VST3Plugin(vd, settings);

		if (vd->output_data.array) {
			da_free(vd->output_data);
		}
		vd->vst3_id = vst3_plugin_id;
		vd->last_init_failed = false;

		// retrieve path and name from VST3Scanner or from settings (required because vst3 scan can be long)
		if (!list->getPathById(vst3_plugin_id).empty()) {
			vd->vst3_path = list->getPathById(vst3_plugin_id);
			vd->vst3_name = list->getNameById(vst3_plugin_id);
		} else {
			vd->vst3_path = obs_data_get_string(settings, "vst3_path");
			vd->vst3_name = obs_data_get_string(settings, "vst3_name");
		}

		infovst3("filter applied: %s, path: %s", vd->vst3_name.c_str(), vd->vst3_path.c_str());

		if (init_VST3Plugin(vd, settings)) {
			auto plugin2 = std::atomic_load(&vd->plugin);
			if (plugin2) {
				plugin2->setProcessing(true);
				vd->sc_channels = plugin2->sidechainNumChannels;
			}
			// we support sidechain only for mono or stereo buses (sanity check)
			vd->has_sidechain.store(vd->sc_channels == 1 || vd->sc_channels == 2,
						std::memory_order_relaxed);
			vd->bypass.store(false, std::memory_order_relaxed);
			plugin = plugin2;
		} else {
			infovst3("VST3 failure; plugin deactivated.");
			vd->bypass.store(true, std::memory_order_relaxed);
			vd->has_sidechain.store(false, std::memory_order_relaxed);
			vd->sidechain_enabled.store(false, std::memory_order_relaxed);
		}
	}

	// Only load the state the first time the filter is loaded
	if (plugin && initial_load) {
		const char *hexComp = obs_data_get_string(settings, "vst3_state");
		const char *hexCtrl = obs_data_get_string(settings, "vst3_ctrl_state");
		if (hexComp && *hexComp) {
			std::vector<uint8_t> comp = fromHex(hexComp);
			std::vector<uint8_t> ctrl;
			if (hexCtrl && *hexCtrl)
				ctrl = fromHex(hexCtrl);
			plugin->loadStates(comp, ctrl);
		}
	}
	// Sidechain specific code starts here, cf obs-filters/compressor-filter.c for the logic. The sidechain swap is
	// done in 2 steps with the swapping proper in the video tick callback after a 3 sec wait.
	if (vd->has_sidechain.load(std::memory_order_relaxed))
		sidechain_swap(vd, settings);
}

static void *vst3_create(obs_data_t *settings, obs_source_t *filter)
{
	auto *vd = new vst3_audio_data();

	if (!vd)
		return NULL;

	vd->context = filter;
	vd->vst3_id = {};
	vd->vst3_name = {};
	vd->vst3_path = {};

	audio_t *audio = obs_get_audio();
	const struct audio_output_info *aoi = audio_output_get_info(audio);

	size_t frames = (size_t)FRAME_SIZE;
	vd->frames = frames;
	size_t channels = audio_output_get_channels(audio);
	vd->channels = channels;
	vd->sample_rate = audio_output_get_sample_rate(audio);
	vd->layout = aoi->speakers;
	vd->has_sidechain.store(false, std::memory_order_relaxed);
	vd->sidechain_enabled.store(false, std::memory_order_relaxed);
	vd->noview.store(true, std::memory_order_relaxed);

	vd->latency = 1000000000LL / (1000 / BUFFER_SIZE_MSEC);

	// allocate copy buffers(which are contiguous for the channels)
	vd->copy_buffers[0] = (float *)bmalloc((size_t)FRAME_SIZE * channels * sizeof(float));
	vd->sc_copy_buffers[0] = (float *)bmalloc(FRAME_SIZE * channels * sizeof(float));

	for (size_t c = 1; c < channels; ++c) {
		vd->copy_buffers[c] = vd->copy_buffers[c - 1] + frames;
		vd->sc_copy_buffers[c] = vd->sc_copy_buffers[c - 1] + frames;
	}

	// reserve deques (about 4 ticks, quite large but better safe than sorry)
	for (size_t i = 0; i < channels; i++) {
		deque_reserve(&vd->input_buffers[i], 8 * frames * sizeof(float));
		deque_reserve(&vd->output_buffers[i], 8 * frames * sizeof(float));
		deque_reserve(&vd->sc_input_buffers[i], 8 * frames * sizeof(float));
	}

	vd->bypass.store(true, std::memory_order_relaxed);

	vst3_update(vd, settings);
	return vd;
}

void vst3_save(void *data, obs_data_t *settings)
{
	vst3_audio_data *vd = (vst3_audio_data *)data;
	if (!vd)
		return;

	auto plugin = std::atomic_load(&vd->plugin);
	if (plugin) {
		std::vector<uint8_t> comp, ctrl;
		if (plugin->saveStates(comp, ctrl)) {
			obs_data_set_string(settings, "vst3_state", toHex(comp).c_str());
			if (!ctrl.empty())
				obs_data_set_string(settings, "vst3_ctrl_state", toHex(ctrl).c_str());
			else
				obs_data_set_string(settings, "vst3_ctrl_state", "");
		}
		// We store these because the filter might load before VST3s list has been populated with this info.
		obs_data_set_string(settings, "vst3_path", vd->vst3_path.c_str());
		obs_data_set_string(settings, "vst3_name", vd->vst3_name.c_str());
	}
}

/* -------------- audio processing (incl. sc) --------------- */
static inline void preprocess_input(struct vst3_audio_data *vd, const std::shared_ptr<VST3Plugin> &plugin)
{
	int num_channels = (int)vd->channels;
	int sc_num_channels = (int)vd->sc_channels;
	int frames = (int)vd->frames;
	size_t segment_size = vd->frames * sizeof(float);
	bool has_sc = vd->has_sidechain.load(std::memory_order_relaxed);
	bool sc_enabled = vd->sidechain_enabled.load(std::memory_order_relaxed);

	if (has_sc && sc_enabled) {
		std::lock_guard<std::mutex> lock(vd->sidechain_mutex);
		for (int i = 0; i < num_channels; i++) {
			if (vd->sc_input_buffers[i].size < segment_size)
				deque_push_back_zero(&vd->sc_input_buffers[i], segment_size);
		}
	}

	// Pop from input deque (main + sc)
	for (int i = 0; i < num_channels; i++)
		deque_pop_front(&vd->input_buffers[i], vd->copy_buffers[i], vd->frames * sizeof(float));

	if (has_sc && sc_enabled) {
		std::lock_guard<std::mutex> lock(vd->sidechain_mutex);
		for (int i = 0; i < num_channels; i++)
			deque_pop_front(&vd->sc_input_buffers[i], vd->sc_copy_buffers[i], vd->frames * sizeof(float));
	}

	// Copy input OBS buffer to VST input buffers
	for (int ch = 0; ch < num_channels; ++ch) {
		float *inBuf = (float *)vd->copy_buffers[ch];
		float *vstIn = plugin->channelBuffer32(Steinberg::Vst::kInput, ch);
		if (inBuf && vstIn)
			memcpy(vstIn, inBuf, frames * sizeof(float));
	}

	// Copy sidechain input OBS buffer to VST input buffers (upmix or downmix if necessary)
	if (has_sc && sc_enabled) {
		bool needs_resampling = vd->channels != vd->sc_channels &&
					(vd->sc_channels == 1 || vd->sc_channels == 2);
		auto sc_resampler = std::atomic_load(&vd->sc_resampler);
		if (needs_resampling && sc_resampler) {
			uint8_t *resampled[2] = {nullptr, nullptr};
			uint32_t out_frames;
			uint64_t ts_offset;

			if (audio_resampler_resample(sc_resampler.get(), resampled, &out_frames, &ts_offset,
						     (const uint8_t **)vd->sc_copy_buffers, (uint32_t)vd->frames)) {
				for (int ch = 0; ch < sc_num_channels; ++ch) {
					float *inBuf = (float *)resampled[ch];
					float *vstIn = plugin->sidechannelBuffer32(Steinberg::Vst::kInput, ch);
					if (inBuf && vstIn)
						memcpy(vstIn, inBuf, out_frames * sizeof(float));
				}
			}
		} else {
			for (int ch = 0; ch < sc_num_channels; ++ch) {
				float *inBuf = vd->sc_copy_buffers[ch];
				float *vstIn = plugin->sidechannelBuffer32(Steinberg::Vst::kInput, ch);
				if (inBuf && vstIn)
					memcpy(vstIn, inBuf, frames * sizeof(float));
			}
		}
	}
}

static inline void process(struct vst3_audio_data *vd, const std::shared_ptr<VST3Plugin> &plugin)
{
	int num_channels = (int)vd->channels;
	int frames = (int)vd->frames;

	preprocess_input(vd, plugin);
	plugin->process(frames);

	// Retrieve processed buffers from VST
	for (int ch = 0; ch < num_channels; ++ch) {
		uint8_t *outBuf = (uint8_t *)vd->copy_buffers[ch];
		float *vstOut = plugin->channelBuffer32(Steinberg::Vst::kOutput, ch);
		if (outBuf && vstOut)
			memcpy(outBuf, vstOut, frames * sizeof(float));
	}
	// Push to output deque
	for (size_t i = 0; i < vd->channels; i++)
		deque_push_back(&vd->output_buffers[i], vd->copy_buffers[i], vd->frames * sizeof(float));
}

// This re-uses the main logic from obs-filters/noise-suppress.c
static struct obs_audio_data *vst3_filter_audio(void *data, struct obs_audio_data *audio)
{
	vst3_audio_data *vd = (vst3_audio_data *)data;
	struct vst3_audio_info info;
	size_t segment_size = vd->frames * sizeof(float);
	size_t out_size;
	auto p = std::atomic_load(&vd->plugin);
	bool bypass = vd->bypass.load(std::memory_order_relaxed);

	if (bypass || !p)
		return audio;

	if (!p->numEnabledOutAudioBuses)
		return audio;

	/* If timestamp has dramatically changed, consider it a new stream of audio data. Clear all deques to prevent
	 * old audio data from being processed as part of the new data. */
	if (vd->last_timestamp) {
		int64_t diff = llabs((int64_t)vd->last_timestamp - (int64_t)audio->timestamp);

		if (diff > 1000000000LL)
			reset_data(vd);
	}

	vd->last_timestamp = audio->timestamp;

	/* push audio packet info (timestamp/frame count) to info deque */
	info.frames = audio->frames;
	info.timestamp = audio->timestamp;
	deque_push_back(&vd->info_buffer, &info, sizeof(info));

	/* push back current audio data to input deque */
	for (size_t i = 0; i < vd->channels; i++)
		deque_push_back(&vd->input_buffers[i], audio->data[i], audio->frames * sizeof(float));

	/* pop/process each 10ms segments, push back to output deque */
	while (vd->input_buffers[0].size >= segment_size)
		process(vd, p);

	/* peek front of info deque, check to see if we have enough to pop the expected packet size, if not, return null */
	memset(&info, 0, sizeof(info));
	deque_peek_front(&vd->info_buffer, &info, sizeof(info));
	out_size = info.frames * sizeof(float);

	if (vd->output_buffers[0].size < out_size)
		return NULL;

	/* if there's enough audio data buffered in the output deque, pop and return a packet */
	deque_pop_front(&vd->info_buffer, NULL, sizeof(info));
	da_resize(vd->output_data, out_size * vd->channels);

	for (size_t i = 0; i < vd->channels; i++) {
		vd->output_audio.data[i] = (uint8_t *)&vd->output_data.array[i * out_size];

		deque_pop_front(&vd->output_buffers[i], vd->output_audio.data[i], out_size);
	}

	vd->running_sample_count += info.frames;
	vd->system_time = os_gettime_ns();
	vd->output_audio.frames = info.frames;
	vd->output_audio.timestamp = info.timestamp - vd->latency;
	return &vd->output_audio;
}

static void sidechain_capture(void *data, obs_source_t *source, const struct audio_data *audio, bool muted)
{
	UNUSED_PARAMETER(source);
	UNUSED_PARAMETER(muted);
	struct vst3_audio_data *vd = (struct vst3_audio_data *)data;
	auto p = std::atomic_load(&vd->plugin);
	bool bypass = vd->bypass.load(std::memory_order_relaxed);
	bool sc_enabled = vd->sidechain_enabled.load(std::memory_order_relaxed);

	if (bypass || !p)
		return;

	if (!sc_enabled)
		return;

	if (vd->sc_channels != 1 && vd->sc_channels != 2)
		return;

	/* If timestamp has dramatically changed, consider it a new stream of audio data. Clear all deques to prevent old
	 * audio data from being processed as part of the new data. */
	if (vd->sc_last_timestamp) {
		int64_t diff = llabs((int64_t)vd->sc_last_timestamp - (int64_t)audio->timestamp);

		if (diff > 1000000000LL)
			reset_sidechain_data(vd);
	}

	vd->sc_last_timestamp = audio->timestamp;

	/* push back current audio data to input deque */
	{
		std::lock_guard<std::mutex> lock(vd->sidechain_mutex);
		for (size_t i = 0; i < vd->channels; i++)
			deque_push_back(&vd->sc_input_buffers[i], audio->data[i], audio->frames * sizeof(float));
	}
}

// written after obs-filters/compressor-filter.c for the sidechain logic
static void vst3_tick(void *data, float seconds)
{
	struct vst3_audio_data *vd = (struct vst3_audio_data *)data;
	if (!vd)
		return;

	bool has_sc = vd->has_sidechain.load(std::memory_order_relaxed);

	if (!has_sc)
		return;

	std::string new_name = {};
	{
		std::lock_guard<std::mutex> lock(vd->sidechain_update_mutex);
		if (!vd->sidechain_name.empty() && !vd->weak_sidechain) {
			uint64_t t = os_gettime_ns();

			if (t - vd->sidechain_check_time > 3000000000) {
				new_name = vd->sidechain_name;
				vd->sidechain_check_time = t;
			}
		}
	}

	if (!new_name.empty()) {
		obs_source_t *sidechain = obs_get_source_by_name(new_name.c_str());
		obs_weak_source_t *weak_sidechain = sidechain ? obs_source_get_weak_source(sidechain) : NULL;
		{
			std::lock_guard<std::mutex> lock(vd->sidechain_update_mutex);
			if (!vd->sidechain_name.empty() && vd->sidechain_name == new_name) {
				vd->weak_sidechain = weak_sidechain;
				weak_sidechain = NULL;
			}
		}
		if (sidechain) {
			// downmix or upmix if channel count is mismatched
			bool needs_resampling = vd->channels != vd->sc_channels;
			if (needs_resampling) {
				struct resample_info src, dst;
				src.samples_per_sec = vd->sample_rate;
				src.format = AUDIO_FORMAT_FLOAT_PLANAR;
				src.speakers = convert_speaker_layout((uint8_t)vd->channels);

				dst.samples_per_sec = vd->sample_rate;
				dst.format = AUDIO_FORMAT_FLOAT_PLANAR;
				dst.speakers = convert_speaker_layout((uint8_t)vd->sc_channels);

				audio_resampler *raw = audio_resampler_create(&dst, &src);
				if (!raw) {
					std::atomic_store(&vd->sc_resampler, std::shared_ptr<audio_resampler>{});
				} else {
					std::shared_ptr<audio_resampler> sp(raw, [](audio_resampler *r) {
						if (r)
							audio_resampler_destroy(r);
					});
					std::atomic_store(&vd->sc_resampler, sp);
				}
			} else {
				std::atomic_store(&vd->sc_resampler, std::shared_ptr<audio_resampler>{});
			}
			obs_source_add_audio_capture_callback(sidechain, sidechain_capture, vd);
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
	vst3_audio_data *vd = static_cast<vst3_audio_data *>(data);
	if (!vd)
		return false;

	auto plugin = std::atomic_load(&vd->plugin);
	if (!plugin)
		return false;

	bool noview = vd->noview.load(std::memory_order_relaxed);
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

	if (source == info->parent)
		return true;

	if ((caps & OBS_SOURCE_AUDIO) == 0)
		return true;

	const char *name = obs_source_get_name(source);
	obs_property_list_add_string(info->sources, name, name);
	return true;
}

bool on_vst3_changed_cb(void *priv, obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(settings);
	auto vd = (struct vst3_audio_data *)priv;
	if (!vd)
		return false;

	bool has_sc = vd->has_sidechain.load(std::memory_order_relaxed);

	obs_property_t *gui = obs_properties_get(props, S_EDITOR);
	obs_property_set_visible(gui, !vd->noview.load(std::memory_order_relaxed) && !vd->last_init_failed);

	obs_property_t *p = obs_properties_get(props, S_SIDECHAIN_SOURCE);
	if (has_sc && !vd->last_init_failed) {
		obs_source_t *parent = obs_filter_get_parent(vd->context);
		obs_property_list_clear(p);
		obs_property_list_add_string(p, obs_module_text("None"), "none");
		struct sidechain_prop_info info = {p, parent};
		obs_enum_sources(add_sources, &info);
		obs_property_set_visible(p, true);
	} else {
		obs_property_set_visible(p, false);
	}

	obs_property_t *noview = obs_properties_get(props, S_NOGUI);
	obs_property_set_visible(noview, vd->noview.load(std::memory_order_relaxed) && !vd->last_init_failed);

	obs_property_t *err = obs_properties_get(props, S_ERR);
	if (err) {
		obs_properties_remove_by_name(props, S_ERR);
	}
	if (vd->last_init_failed) {
		obs_property_t *err2 = obs_properties_add_text(props, S_ERR, TEXT_ERR, OBS_TEXT_INFO);
		obs_property_text_set_info_type(err2, OBS_TEXT_INFO_ERROR);
	}
	return true;
}

static obs_properties_t *vst3_properties(void *data)
{
	auto vd = (struct vst3_audio_data *)data;
	obs_properties_t *props = obs_properties_create();
	obs_property_t *sources;
	obs_property_t *vst3list =
		obs_properties_add_list(props, S_PLUGIN, TEXT_PLUGIN, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(vst3list, obs_module_text("VST3.Select"), "");
	for (const auto &plugin : list->pluginList) {
		const bool multi = list->ModWithMultipleClasses(plugin.path);
		std::string display = multi ? plugin.name + " (" + plugin.pluginName + ")" : plugin.name;
		std::string value = plugin.id;
		obs_property_list_add_string(vst3list, display.c_str(), value.c_str());
	}
	obs_property_t *gui =
		obs_properties_add_button(props, S_EDITOR, obs_module_text(TEXT_EDITOR), vst3_show_gui_callback);
	obs_property_set_visible(gui, !vd->noview.load(std::memory_order_relaxed) && !vd->last_init_failed);

	sources = obs_properties_add_list(props, S_SIDECHAIN_SOURCE, TEXT_SIDECHAIN_SOURCE, OBS_COMBO_TYPE_LIST,
					  OBS_COMBO_FORMAT_STRING);
	obs_property_set_visible(sources, !vd->last_init_failed);

	obs_property_set_modified_callback2(vst3list, on_vst3_changed_cb, data);

	obs_property_t *noview = obs_properties_add_text(props, S_NOGUI, TEXT_NOGUI, OBS_TEXT_INFO);
	obs_property_text_set_info_type(noview, OBS_TEXT_INFO_WARNING);
	obs_property_set_visible(noview, vd->noview.load(std::memory_order_relaxed) && !vd->last_init_failed);

	if (vd->last_init_failed) {
		obs_property_t *err = obs_properties_add_text(props, S_ERR, TEXT_ERR, OBS_TEXT_INFO);
		obs_property_text_set_info_type(err, OBS_TEXT_INFO_ERROR);
	}

	if (!vst3_scan_done.load(std::memory_order_relaxed)) {
		obs_property_t *scan_err = obs_properties_add_text(props, S_SCAN, TEXT_SCAN, OBS_TEXT_INFO);
		obs_property_text_set_info_type(scan_err, OBS_TEXT_INFO_ERROR);
	}

	return props;
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
