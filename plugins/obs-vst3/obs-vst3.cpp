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

#ifdef __linux__
#include "RunLoopImpl.h"
#endif
#include "VST3HostApp.h"
#include "VST3Plugin.h"
#include "VST3Scanner.h"

#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/vstspeaker.h>

#include <media-io/audio-io.h>
#include <media-io/audio-resampler.h>
#include <obs-module.h>
#ifdef __linux__
#include <obs-nix-platform.h>
#endif
#include <util/bmem.h>
#include <util/darray.h>
#include <util/deque.h>
#include <util/platform.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#define MT_ obs_module_text
constexpr auto S_PLUGIN = "vst3_plugin";
constexpr auto S_EDITOR = "vst3_open_gui";
constexpr auto S_SIDECHAIN_SOURCE = "sidechain_source";
constexpr auto S_NOGUI = "vst3_noview";
constexpr auto S_ERR = "vst3_error";
constexpr auto S_SCAN = "vst3_scan";
constexpr auto S_RESCAN = "vst3_rescan";

#define TEXT_EDITOR MT_("VST3.Button")
#define TEXT_PLUGIN MT_("VST3.Plugin")
#define TEXT_SIDECHAIN_SOURCE  MT_("VST3.SidechainSource")
#define TEXT_NOGUI MT_("VST3.NOGUI")
#define TEXT_ERR MT_("VST3.Init.Fail")
#define TEXT_SCAN MT_("VST3.Scan.Ongoing")
#define TEXT_RESCAN MT_("VST3.Rescan")

// --------------------------------------------------------
#define do_log(level, format, ...) \
	blog(level, "[VST3 filter ('%s')]: " format, obs_source_get_name(filterData->context), ## __VA_ARGS__)

#define warnvst3(format, ...) do_log(LOG_WARNING, format, ## __VA_ARGS__)
#define infovst3(format, ...) do_log(LOG_INFO, format, ## __VA_ARGS__)

#ifdef _DEBUG
#define debugvst3(format, ...) do_log(LOG_DEBUG, format, ## __VA_ARGS__)
#endif
// --------------------------------------------------------

struct vst3_audio_info {
	uint32_t frames;
	uint64_t timestamp;
};

struct sidechain_prop_info {
	obs_property_t *sources;
	obs_source_t *parent;
};
// ----------------- global host & runloop ----------------
namespace {
std::unique_ptr<VST3HostApp> gHostApp;
#ifdef __linux__
std::unique_ptr<RunLoopImpl> gRunLoop;
#endif
} // namespace

VST3HostApp *getHostApp() noexcept
{
	return gHostApp.get();
}

bool loadHost()
{
	VST3Backend backend = VST3Backend::Unknown;

#ifdef _WIN32
	backend = VST3Backend::Windows;
#elif defined(__APPLE__)
	backend = VST3Backend::MacOS;
#elif defined(__linux__)
	const auto platform = obs_get_nix_platform();

	if (platform == OBS_NIX_PLATFORM_X11_EGL) {
		backend = VST3Backend::X11;
	} else if (platform == OBS_NIX_PLATFORM_WAYLAND) {
		backend = VST3Backend::Wayland;
	}
#endif

	if (backend == VST3Backend::Unknown) {
		blog(LOG_WARNING, "[VST3 Host] Unsupported platform");
		return false;
	}

	auto hostApp = std::make_unique<VST3HostApp>(backend);

#ifdef __linux__
	auto runLoop = std::make_unique<RunLoopImpl>();
	hostApp->setRunLoop(runLoop.get());
#endif

	gHostApp = std::move(hostApp);

#ifdef __linux__
	gRunLoop = std::move(runLoop);
#endif

	return true;
}

void unloadHost()
{
#ifdef __linux__
	if (gHostApp) {
		gHostApp->setRunLoop(nullptr);
	}

	if (gRunLoop) {
		gRunLoop->stop();
	}

	gRunLoop.reset();
#endif

	gHostApp.reset();
}
// -------------------- initial scanning ------------------
VST3Scanner *list;
std::atomic<bool> vst3ScanDone;
static std::thread vst3ScanThread;

static void vst3CacheSave()
{
	if (!list) {
		return;
	}

	char *path = obs_module_config_path(nullptr);
	os_mkdirs(path);
	bfree(path);

	char *filepath = obs_module_config_path("vst3list.json");

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

static bool vst3CacheLoad()
{
	char *path = obs_module_config_path("vst3list.json");
	if (!path) {
		return false;
	}

	obs_data_t *root = obs_data_create_from_json_file_safe(path, "bak");
	bfree(path);
	if (!root) {
		return false;
	}

	obs_data_array_t *jsonPluginList = obs_data_get_array(root, "plugins");
	if (!jsonPluginList) {
		obs_data_release(root);
		return false;
	}

	list->pluginList.clear();
	list->classCount.clear();

	std::unordered_map<std::string, ModuleCache> modules;

	size_t jsonPluginNumber = obs_data_array_count(jsonPluginList);
	for (size_t i = 0; i < jsonPluginNumber; ++i) {
		obs_data_t *obj = obs_data_array_item(jsonPluginList, i);
		VST3ClassInfo classInfo;

		classInfo.name = obs_data_get_string(obj, "name");
		classInfo.id = obs_data_get_string(obj, "id");
		classInfo.path = obs_data_get_string(obj, "path");
		classInfo.pluginName = obs_data_get_string(obj, "pluginName");
		classInfo.discardable = obs_data_get_bool(obj, "discardable");

		obs_data_release(obj);

		if (classInfo.path.empty() || !std::filesystem::exists(classInfo.path)) {
			continue;
		}

		auto &m = modules[classInfo.path];
		if (classInfo.discardable) {
			m.discardable = true;
		}

		m.classes.push_back(std::move(classInfo));
	}

	obs_data_array_release(jsonPluginList);
	obs_data_release(root);

	list->updateModulesList(modules);

	for (auto &[modulePath, m] : modules) {
		// if a module has the flag kClassesDiscardable, the SDK compels us to do a full load from binary, duh ...
		if (m.discardable) {
			list->addModuleClasses(modulePath);
		} else {
			for (auto &classInfo : m.classes) {
				list->pluginList.push_back(classInfo);
				++list->classCount[modulePath];
			}
		}
	}

	list->sort();

	return !list->pluginList.empty();
}

static void scanVst3List(bool useCache)
{
	try {
		using clock = std::chrono::steady_clock;
		auto start = clock::now();

		bool loadedFromCache = useCache && vst3CacheLoad();
		if (!loadedFromCache) {
			if (!list->scanForVST3Plugins()) {
				blog(LOG_INFO, "[VST3 Scanner] Error when scanning for VST3.");
			}
		}

		blog(LOG_INFO, "[VST3 Scanner] Available plugins:");
		for (const auto &plugin : list->pluginList) {
			blog(LOG_INFO, "[VST3 Scanner]   %s", plugin.name.c_str());
		}
		auto end = clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		blog(LOG_INFO, "[VST3 Scanner] %s in %lld ms, found %zu plugins",
		     loadedFromCache ? "Loaded cache & non-cacheable VST3s" : "Completed scan",
		     static_cast<long long>(ms), list->pluginList.size());

		vst3CacheSave();
	} catch (const std::exception &e) {
		blog(LOG_ERROR, "[VST3 Scanner] Scan aborted by an exception: %s", e.what());
	} catch (...) {
		blog(LOG_ERROR, "[VST3 Scanner] Scan aborted by an unknown exception");
	}

	vst3ScanDone.store(true, std::memory_order_release);
}

bool retrieveVst3List()
{
	vst3ScanDone.store(false, std::memory_order_relaxed);
	list = new VST3Scanner();
	if (!list->hasVST3()) {
		vst3ScanDone.store(true, std::memory_order_release);
		blog(LOG_INFO, "[VST3 Scanner] No VST3 were found");
		return false;
	}

	vst3ScanThread = std::thread(scanVst3List, true);

	return true;
}

void freeVst3List()
{
	if (vst3ScanThread.joinable()) {
		vst3ScanThread.join();
	}
	delete list;
}

static bool isValidHex(const std::string &hex)
{
	if (hex.empty()) {
		return false;
	}

	if ((hex.size() & 1) != 0) {
		return false;
	}

	for (char c : hex) {
		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
			return false;
		}
	}
	return true;
}

std::string toHex(const std::vector<uint8_t> &data)
{
	std::ostringstream oss;
	for (auto b : data) {
		oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
	}

	return oss.str();
}

std::vector<uint8_t> fromHex(const std::string &hex)
{
	if (!isValidHex(hex)) {
		blog(LOG_INFO, "Corrupted VST3 settings.");
		return {};
	}

	std::vector<uint8_t> data;
	for (size_t i = 0; i + 1 < hex.size(); i += 2) {
		data.push_back(static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
	}

	return data;
}

Steinberg::Vst::SpeakerArrangement obsToVst3SpeakerArrangement(speaker_layout layout)
{
	switch (layout) {
	case SPEAKERS_MONO:
		return Steinberg::Vst::SpeakerArr::kMono;
	case SPEAKERS_STEREO:
		return Steinberg::Vst::SpeakerArr::kStereo;
	case SPEAKERS_2POINT1:
		return Steinberg::Vst::SpeakerArr::kStereo | kSpeakerLfe;
	case SPEAKERS_4POINT0:
		return Steinberg::Vst::SpeakerArr::k40Music;
	case SPEAKERS_4POINT1:
		return Steinberg::Vst::SpeakerArr::k41Music;
	case SPEAKERS_5POINT1:
		return Steinberg::Vst::SpeakerArr::k51;
	case SPEAKERS_7POINT1:
		return Steinberg::Vst::SpeakerArr::k71Music;
	case SPEAKERS_UNKNOWN:
	default:
		return Steinberg::Vst::SpeakerArr::kEmpty;
	}
}

static inline enum speaker_layout convertSpeakerLayout(uint8_t channels)
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

//--------------------- deque management --------------------------

static inline void clearDeque(struct deque *buf)
{
	deque_pop_front(buf, nullptr, buf->size);
}

static void resetDequeData(struct Vst3AudioData *filterData)
{
	for (size_t i = 0; i < filterData->channels; i++) {
		clearDeque(&filterData->inputBuffers[i]);
		clearDeque(&filterData->outputBuffers[i]);
	}

	clearDeque(&filterData->infoBuffer);
}

static void resetSidechainData(struct Vst3AudioData *filterData)
{
	std::lock_guard<std::mutex> lock(filterData->sidechainMutex);
	for (size_t i = 0; i < filterData->channels; i++) {
		clearDeque(&filterData->sidechainInputBuffers[i]);
	}
}

// -------------------- main functions -------------------

static const char *vst3Name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return TEXT_PLUGIN;
}

static void sidechainCapture(void *data, obs_source_t *source, const struct audio_data *audio, bool muted);

static void vst3Destroy(void *data)
{
	auto *filterData = static_cast<struct Vst3AudioData *>(data);
	filterData->bypass.store(true, std::memory_order_relaxed);
	filterData->sidechainEnabled.store(false, std::memory_order_relaxed);

	if (filterData->weakSidechain) {
		obs_source_t *sidechain = obs_weak_source_get_source(filterData->weakSidechain);
		if (sidechain) {
			obs_source_remove_audio_capture_callback(sidechain, sidechainCapture, filterData);
			obs_source_release(sidechain);
		}
		obs_weak_source_release(filterData->weakSidechain);
	}

	std::atomic_store(&filterData->sidechainResampler, std::shared_ptr<audio_resampler>{});
	filterData->sidechainLastTimestamp = 0;

	for (size_t i = 0; i < filterData->channels; i++) {
		deque_free(&filterData->inputBuffers[i]);
		deque_free(&filterData->outputBuffers[i]);
		{
			std::lock_guard<std::mutex> lock(filterData->sidechainMutex);
			deque_free(&filterData->sidechainInputBuffers[i]);
		}
	}
	bfree(filterData->copyBuffers[0]);
	bfree(filterData->sidechainCopyBuffers[0]);
	deque_free(&filterData->infoBuffer);
	da_free(filterData->outputData);

	auto plugin = std::atomic_exchange(&filterData->plugin, std::shared_ptr<VST3Plugin>{});

	if (!plugin) {
		delete filterData;
		return;
	}

	QObject::connect(plugin.get(), &QObject::destroyed, [filterData](QObject *) { delete filterData; });
}

static void teardownSidechain(Vst3AudioData *filterData, obs_data *settings)
{
	if (!filterData->weakSidechain || filterData->sidechainName.empty()) {
		return;
	}
	filterData->sidechainEnabled.store(false, std::memory_order_relaxed);

	obs_weak_source_t *oldWeak = nullptr;
	{
		std::lock_guard<std::mutex> lock(filterData->sidechainUpdateMutex);
		if (filterData->weakSidechain) {
			oldWeak = filterData->weakSidechain;
			filterData->weakSidechain = nullptr;
		}
		filterData->sidechainName.clear();
		obs_data_set_string(settings, S_SIDECHAIN_SOURCE, nullptr);
	}

	if (oldWeak) {
		obs_source_t *oldSource = obs_weak_source_get_source(oldWeak);

		if (oldSource) {
			obs_source_remove_audio_capture_callback(oldSource, sidechainCapture, filterData);

			obs_source_release(oldSource);
		}

		obs_weak_source_release(oldWeak);
	}

	filterData->sidechainLastTimestamp = 0;
	std::atomic_store(&filterData->sidechainResampler, std::shared_ptr<audio_resampler>{});
}

static void destroyCurrentVST3Plugin(Vst3AudioData *filterData, obs_data *settings)
{
	filterData->bypass.store(true, std::memory_order_relaxed);
	auto plugin = std::atomic_load(&filterData->plugin);
	if (!plugin) {
		return;
	}

	std::atomic_store(&filterData->plugin, std::shared_ptr<VST3Plugin>{});

	plugin->setProcessing(false);
	plugin->hideEditor();
	plugin->deactivateComponent();
	filterData->noview.store(true, std::memory_order_relaxed);

	if (filterData->weakSidechain) {
		teardownSidechain(filterData, settings);
	}
}

static bool createVST3Plugin(Vst3AudioData *filterData)
{
	if (filterData->vst3Id.empty() || filterData->vst3Path.empty()) {
		return true;
	}

	const std::string classId = filterData->vst3Id;
	const std::string vst3Path = filterData->vst3Path;
	const int sampleRate = filterData->sampleRate;
	int maxBlock = static_cast<int>(filterData->frames);

	Steinberg::Vst::SpeakerArrangement speakerArrangement = obsToVst3SpeakerArrangement(filterData->layout);

	auto *raw = new VST3Plugin();
	raw->filterData = filterData;

	if (!raw->init(classId, vst3Path, sampleRate, maxBlock, speakerArrangement)) {
		infovst3("Failed to initialize VST3 plugin %s", raw->name.c_str());
		filterData->lastInitFailed = true;
		delete raw;
		return false;
	} else {
		infovst3("Plugin %s was successfully initialized.", raw->name.c_str());
	}

	// not all VST3s have a GUI!
	if (!raw->createView()) {
		infovst3("Failed to create editor view for plugin at: %s", vst3Path.c_str());
		filterData->noview.store(true, std::memory_order_relaxed);
	} else {
		filterData->noview.store(false, std::memory_order_relaxed);
		infovst3("Plugin %s has a GUI.", raw->name.c_str());
	}

	auto plugin = std::shared_ptr<VST3Plugin>(raw, [](VST3Plugin *p) {
		if (p) {
			p->deleteLater();
		}
	});

	plugin->setProcessing(true);
	std::atomic_store(&filterData->plugin, plugin);
	filterData->bypass.store(false, std::memory_order_relaxed);

	return true;
}

// Main init function; in case of failure, the obs-vst3 filter is bypassed; if the new vst3 is empty, it just deletes
// safely the previous vst3.
static bool initVST3Plugin(void *data, obs_data *settings)
{
	auto *filterData = static_cast<Vst3AudioData *>(data);

	if (filterData->initInProgress.test_and_set()) {
		return false;
	}

	struct ClearFlag {
		std::atomic_flag &f;
		~ClearFlag() { f.clear(); }
	} guard_{filterData->initInProgress};

	destroyCurrentVST3Plugin(filterData, settings);

	return createVST3Plugin(filterData);
}

static void sidechainSwap(Vst3AudioData *filterData, obs_data *settings)
{
	if (!filterData->hasSidechain.load(std::memory_order_relaxed)) {
		return;
	}

	filterData->sidechainEnabled.store(false, std::memory_order_relaxed);

	std::string sidechainName(obs_data_get_string(settings, S_SIDECHAIN_SOURCE));
	bool validSidechain = sidechainName != "none" && !sidechainName.empty();
	obs_weak_source_t *oldWeakSidechain = nullptr;

	{
		std::lock_guard<std::mutex> lock(filterData->sidechainUpdateMutex);
		if (!validSidechain) {
			{
				if (filterData->weakSidechain) {
					oldWeakSidechain = filterData->weakSidechain;
					filterData->weakSidechain = nullptr;
				}
				filterData->sidechainName = "";
			}
		} else {

			if (filterData->sidechainName.empty() || filterData->sidechainName != sidechainName) {
				if (filterData->weakSidechain) {
					oldWeakSidechain = filterData->weakSidechain;
					filterData->weakSidechain = nullptr;
				}
				filterData->sidechainName = sidechainName;
				filterData->sidechainCheckTime = os_gettime_ns() - 3000000000;
			}
		}
	}
	filterData->sidechainEnabled.store(true, std::memory_order_relaxed);

	if (oldWeakSidechain) {
		obs_source_t *oldSidechain = obs_weak_source_get_source(oldWeakSidechain);

		if (oldSidechain) {
			obs_source_remove_audio_capture_callback(oldSidechain, sidechainCapture, filterData);
			obs_source_release(oldSidechain);
		}

		obs_weak_source_release(oldWeakSidechain);
	}
}

static void clear_vst3_state(obs_data_t *settings)
{
	obs_data_set_string(settings, "vst3_state", "");
	obs_data_set_string(settings, "vst3_ctrl_state", "");
}

// Our logic differs significantly from a DAW. We indeed allow swapping of VST3s which may or may not have a sc.
// 2 or 3 threads are then involved (UI, audio and possibly video due to the trick of sidechain audio capture
// leveraging video_tick). We've taken great care to implement Ross Bencina's cardinal rule for audio programming.
// http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing but for sidechain there's
// still a mutex that's inherited from the obs-filter compressor... TODO: revisit both filters later to improve that.
static void vst3Update(void *data, obs_data_t *settings)
{
	auto *filterData = static_cast<struct Vst3AudioData *>(data);
	if (!filterData) {
		return;
	}

	std::string vst3PluginId(obs_data_get_string(settings, S_PLUGIN));

	if (vst3PluginId.empty()) {
		filterData->bypass.store(true, std::memory_order_relaxed);
		filterData->vst3Id.clear();
		filterData->vst3Path.clear();
		filterData->vst3Name.clear();
		clear_vst3_state(settings);
		filterData->hasSidechain.store(false, std::memory_order_relaxed);
		destroyCurrentVST3Plugin(filterData, settings);

		return;
	}

	auto plugin = std::atomic_load(&filterData->plugin);
	bool initialLoad = filterData->vst3Id.empty() && !plugin;
	bool isSwap = (filterData->vst3Id != vst3PluginId);

	if (isSwap) {
		if (!initialLoad) {
			clear_vst3_state(settings);
			destroyCurrentVST3Plugin(filterData, settings);
		}

		filterData->vst3Id = vst3PluginId;
		filterData->lastInitFailed = false;

		if (!list->getPathById(vst3PluginId).empty()) {
			filterData->vst3Path = list->getPathById(vst3PluginId);
			filterData->vst3Name = list->getNameById(vst3PluginId);
		} else {
			filterData->vst3Path = obs_data_get_string(settings, "vst3_path");
			filterData->vst3Name = obs_data_get_string(settings, "vst3_name");
		}

		infovst3("filter applied: %s, path: %s", filterData->vst3Name.c_str(), filterData->vst3Path.c_str());

		if (initVST3Plugin(filterData, settings)) {
			auto plugin2 = std::atomic_load(&filterData->plugin);
			if (plugin2) {
				filterData->sidechainChannels = plugin2->sidechainNumberChannels;
			}
			// we support sidechain only for mono or stereo buses (sanity check)
			filterData->hasSidechain.store(filterData->sidechainChannels == 1 ||
							       filterData->sidechainChannels == 2,
						       std::memory_order_relaxed);
			filterData->bypass.store(false, std::memory_order_relaxed);
			plugin = plugin2;
		} else {
			infovst3("VST3 failure; plugin deactivated.");
			filterData->bypass.store(true, std::memory_order_relaxed);
			filterData->hasSidechain.store(false, std::memory_order_relaxed);
			filterData->sidechainEnabled.store(false, std::memory_order_relaxed);
		}
	}

	// Only load the state the first time the filter is loaded
	if (plugin && initialLoad) {
		const char *hexComp = obs_data_get_string(settings, "vst3_state");
		const char *hexCtrl = obs_data_get_string(settings, "vst3_ctrl_state");
		if (hexComp && *hexComp) {
			std::vector<uint8_t> comp = fromHex(hexComp);
			std::vector<uint8_t> ctrl;
			if (hexCtrl && *hexCtrl) {
				ctrl = fromHex(hexCtrl);
			}
			if (!plugin->loadStates(comp, ctrl)) {
				infovst3("VST3 failure; failed to load settings.");
			}
		}
	}
	// Sidechain specific code starts here, cf obs-filters/compressor-filter.c for the logic. The sidechain swap is
	// done in 2 steps with the swapping proper in the video tick callback after a 3 sec wait.
	if (filterData->hasSidechain.load(std::memory_order_relaxed)) {
		sidechainSwap(filterData, settings);
	}
}

static void *vst3Create(obs_data_t *settings, obs_source_t *filter)
{
	auto *filterData = new Vst3AudioData();

	filterData->context = filter;
	filterData->vst3Id = {};
	filterData->vst3Name = {};
	filterData->vst3Path = {};

	audio_t *audio = obs_get_audio();
	const struct audio_output_info *aoi = audio_output_get_info(audio);

	const size_t channels = audio_output_get_channels(audio);
	filterData->channels = channels;
	filterData->sampleRate = audio_output_get_sample_rate(audio);
	filterData->frames = static_cast<size_t>(filterData->sampleRate) * kBufferSizeMilliseconds / 1000;
	const size_t frames = filterData->frames;
	filterData->layout = aoi->speakers;
	filterData->hasSidechain.store(false, std::memory_order_relaxed);
	filterData->sidechainEnabled.store(false, std::memory_order_relaxed);
	filterData->noview.store(true, std::memory_order_relaxed);

	filterData->latency = kBufferSizeMilliseconds * 1000000000LL / 1000;

	filterData->copyBuffers[0] = static_cast<float *>(bmalloc(frames * channels * sizeof(float)));
	filterData->sidechainCopyBuffers[0] = static_cast<float *>(bmalloc(frames * channels * sizeof(float)));

	for (size_t c = 1; c < channels; ++c) {
		filterData->copyBuffers[c] = filterData->copyBuffers[c - 1] + frames;
		filterData->sidechainCopyBuffers[c] = filterData->sidechainCopyBuffers[c - 1] + frames;
	}

	for (size_t i = 0; i < channels; i++) {
		deque_reserve(&filterData->inputBuffers[i], 8 * frames * sizeof(float));
		deque_reserve(&filterData->outputBuffers[i], 8 * frames * sizeof(float));
		deque_reserve(&filterData->sidechainInputBuffers[i], 8 * frames * sizeof(float));
	}

	filterData->bypass.store(true, std::memory_order_relaxed);

	vst3Update(filterData, settings);
	return filterData;
}

void vst3Save(void *data, obs_data_t *settings)
{
	auto *filterData = static_cast<Vst3AudioData *>(data);
	if (!filterData) {
		return;
	}

	auto plugin = std::atomic_load(&filterData->plugin);
	if (plugin) {
		std::vector<uint8_t> comp;
		std::vector<uint8_t> ctrl;
		if (plugin->saveStates(comp, ctrl)) {
			obs_data_set_string(settings, "vst3_state", toHex(comp).c_str());
			if (!ctrl.empty()) {
				obs_data_set_string(settings, "vst3_ctrl_state", toHex(ctrl).c_str());
			} else {
				obs_data_set_string(settings, "vst3_ctrl_state", "");
			}
		} else {
			infovst3("VST3 failure; failed to save settings.");
			clear_vst3_state(settings);
		}
		// We store these because the filter might load before VST3s list has been populated with this info.
		obs_data_set_string(settings, "vst3_path", filterData->vst3Path.c_str());
		obs_data_set_string(settings, "vst3_name", filterData->vst3Name.c_str());
	}
}

// -------------- audio processing (incl. sidechain) ---------------
static inline void preprocessInput(struct Vst3AudioData *filterData, const std::shared_ptr<VST3Plugin> &plugin)
{
	const int numberChannels = static_cast<int>(filterData->channels);
	const int sidechainNumberChannels = plugin->sidechainNumberChannels;
	const int frames = static_cast<int>(filterData->frames);
	const size_t segmentSize = filterData->frames * sizeof(float);
	const bool hasSidechain = filterData->hasSidechain.load(std::memory_order_relaxed);
	const bool sidechainEnabled = filterData->sidechainEnabled.load(std::memory_order_relaxed);

	if (hasSidechain && sidechainEnabled) {
		std::lock_guard<std::mutex> lock(filterData->sidechainMutex);
		for (int i = 0; i < numberChannels; i++) {
			if (filterData->sidechainInputBuffers[i].size < segmentSize) {
				deque_push_back_zero(&filterData->sidechainInputBuffers[i], segmentSize);
			}
		}
	}

	for (int i = 0; i < numberChannels; i++) {
		deque_pop_front(&filterData->inputBuffers[i], filterData->copyBuffers[i],
				filterData->frames * sizeof(float));
	}

	if (hasSidechain && sidechainEnabled) {
		std::lock_guard<std::mutex> lock(filterData->sidechainMutex);
		for (int i = 0; i < numberChannels; i++) {
			deque_pop_front(&filterData->sidechainInputBuffers[i], filterData->sidechainCopyBuffers[i],
					filterData->frames * sizeof(float));
		}
	}

	for (int ch = 0; ch < numberChannels; ++ch) {
		auto *inputBuffers = filterData->copyBuffers[ch];
		float *vstInputBuffers = plugin->channelBuffer32(Steinberg::Vst::kInput, ch);
		if (inputBuffers && vstInputBuffers) {
			memcpy(vstInputBuffers, inputBuffers, frames * sizeof(float));
		}
	}

	if (hasSidechain) {
		for (int ch = 0; ch < sidechainNumberChannels; ++ch) {
			float *vstSidechainInputBuffers = plugin->auxChannelBuffer32(Steinberg::Vst::kInput, ch);
			if (vstSidechainInputBuffers) {
				memset(vstSidechainInputBuffers, 0, segmentSize);
			}
		}
	}

	if (hasSidechain && sidechainEnabled) {
		const bool needsResampling =
			filterData->channels != static_cast<size_t>(plugin->sidechainNumberChannels) &&
			(plugin->sidechainNumberChannels == 1 || plugin->sidechainNumberChannels == 2);
		auto sidechainResampler = std::atomic_load(&filterData->sidechainResampler);
		if (needsResampling && sidechainResampler) {
			uint8_t *resampled[2] = {nullptr, nullptr};
			uint32_t outputResamplerFrames;
			uint64_t timestampOffset;

			if (audio_resampler_resample(sidechainResampler.get(), resampled, &outputResamplerFrames,
						     &timestampOffset,
						     (const uint8_t **)filterData->sidechainCopyBuffers,
						     static_cast<uint32_t>(filterData->frames))) {
				for (int ch = 0; ch < sidechainNumberChannels; ++ch) {
					auto *inputBuffers = reinterpret_cast<float *>(resampled[ch]);
					float *vstSidechainInputBuffers =
						plugin->auxChannelBuffer32(Steinberg::Vst::kInput, ch);
					if (inputBuffers && vstSidechainInputBuffers) {
						memcpy(vstSidechainInputBuffers, inputBuffers,
						       outputResamplerFrames * sizeof(float));
					}
				}
			}
		} else {
			for (int ch = 0; ch < sidechainNumberChannels; ++ch) {
				float *inputBuffers = filterData->sidechainCopyBuffers[ch];
				float *vstSidechainInputBuffers =
					plugin->auxChannelBuffer32(Steinberg::Vst::kInput, ch);
				if (inputBuffers && vstSidechainInputBuffers) {
					memcpy(vstSidechainInputBuffers, inputBuffers, frames * sizeof(float));
				}
			}
		}
	}
}

static inline void process(struct Vst3AudioData *filterData, const std::shared_ptr<VST3Plugin> &plugin)
{
	const int numberChannels = static_cast<int>(filterData->channels);
	const int frames = static_cast<int>(filterData->frames);

	preprocessInput(filterData, plugin);
	plugin->process(frames);

	for (int ch = 0; ch < numberChannels; ++ch) {
		auto *outputBuffers = reinterpret_cast<uint8_t *>(filterData->copyBuffers[ch]);
		float *vstOutputBuffers = plugin->channelBuffer32(Steinberg::Vst::kOutput, ch);
		if (outputBuffers && vstOutputBuffers) {
			memcpy(outputBuffers, vstOutputBuffers, frames * sizeof(float));
		}
	}

	for (size_t i = 0; i < filterData->channels; i++) {
		deque_push_back(&filterData->outputBuffers[i], filterData->copyBuffers[i],
				filterData->frames * sizeof(float));
	}
}

// This re-uses the main logic from obs-filters/noise-suppress.c
static struct obs_audio_data *vst3FilterAudio(void *data, struct obs_audio_data *audio)
{
	auto *filterData = static_cast<Vst3AudioData *>(data);
	struct vst3_audio_info info = {};
	size_t segmentSize = filterData->frames * sizeof(float);
	size_t outputSize;
	auto plugin = std::atomic_load(&filterData->plugin);
	bool bypass = filterData->bypass.load(std::memory_order_relaxed);

	if (bypass || !plugin) {
		return audio;
	}

	if (!plugin->numEnabledOutputAudioBuses) {
		return audio;
	}

	// If timestamp has dramatically changed, consider it a new stream of audio data. Clear all deques to prevent
	// old audio data from being processed as part of the new data.
	if (filterData->lastTimestamp) {
		int64_t diff =
			llabs(static_cast<int64_t>(filterData->lastTimestamp) - static_cast<int64_t>(audio->timestamp));

		if (diff > 1000000000LL) {
			resetDequeData(filterData);
		}
	}

	filterData->lastTimestamp = audio->timestamp;

	info.frames = audio->frames;
	info.timestamp = audio->timestamp;
	deque_push_back(&filterData->infoBuffer, &info, sizeof(info));

	for (size_t i = 0; i < filterData->channels; i++) {
		deque_push_back(&filterData->inputBuffers[i], audio->data[i], audio->frames * sizeof(float));
	}

	while (filterData->inputBuffers[0].size >= segmentSize) {
		process(filterData, plugin);
	}

	memset(&info, 0, sizeof(info));
	deque_peek_front(&filterData->infoBuffer, &info, sizeof(info));
	outputSize = info.frames * sizeof(float);

	if (filterData->outputBuffers[0].size < outputSize) {
		return nullptr;
	}

	deque_pop_front(&filterData->infoBuffer, nullptr, sizeof(info));
	da_resize(filterData->outputData, info.frames * filterData->channels);

	for (size_t i = 0; i < filterData->channels; i++) {
		filterData->outputAudio.data[i] =
			reinterpret_cast<uint8_t *>(&filterData->outputData.array[i * info.frames]);

		deque_pop_front(&filterData->outputBuffers[i], filterData->outputAudio.data[i], outputSize);
	}

	filterData->runningSampleCount += info.frames;
	filterData->systemTime = os_gettime_ns();
	filterData->outputAudio.frames = info.frames;
	filterData->outputAudio.timestamp = info.timestamp - filterData->latency;
	return &filterData->outputAudio;
}

static void sidechainCapture(void *data, obs_source_t *source, const struct audio_data *audio, bool muted)
{
	UNUSED_PARAMETER(source);
	UNUSED_PARAMETER(muted);
	auto *filterData = static_cast<struct Vst3AudioData *>(data);
	auto plugin = std::atomic_load(&filterData->plugin);
	bool bypass = filterData->bypass.load(std::memory_order_relaxed);
	bool sidechainEnabled = filterData->sidechainEnabled.load(std::memory_order_relaxed);

	if (bypass || !plugin) {
		return;
	}

	if (!sidechainEnabled) {
		return;
	}

	if (plugin->sidechainNumberChannels != 1 && plugin->sidechainNumberChannels != 2) {
		return;
	}

	if (filterData->sidechainLastTimestamp) {
		int64_t diff = llabs(static_cast<int64_t>(filterData->sidechainLastTimestamp) -
				     static_cast<int64_t>(audio->timestamp));

		if (diff > 1000000000LL) {
			resetSidechainData(filterData);
		}
	}

	filterData->sidechainLastTimestamp = audio->timestamp;

	{
		std::lock_guard<std::mutex> lock(filterData->sidechainMutex);
		for (size_t i = 0; i < filterData->channels; i++) {
			deque_push_back(&filterData->sidechainInputBuffers[i], audio->data[i],
					audio->frames * sizeof(float));
		}
	}
}

// written after obs-filters/compressor-filter.c for the sidechain logic
static void vst3Tick(void *data, float seconds)
{
	auto *filterData = static_cast<struct Vst3AudioData *>(data);
	if (!filterData) {
		return;
	}

	bool hasSidechain = filterData->hasSidechain.load(std::memory_order_relaxed);

	if (!hasSidechain) {
		return;
	}

	auto plugin = std::atomic_load(&filterData->plugin);
	if (!plugin) {
		return;
	}

	const auto sidechainChannels = plugin->sidechainNumberChannels;
	if (sidechainChannels != 1 && sidechainChannels != 2) {
		return;
	}

	std::string newName = {};
	{
		std::lock_guard<std::mutex> lock(filterData->sidechainUpdateMutex);
		if (!filterData->sidechainName.empty() && !filterData->weakSidechain) {
			uint64_t time = os_gettime_ns();

			if (time - filterData->sidechainCheckTime > 3000000000) {
				newName = filterData->sidechainName;
				filterData->sidechainCheckTime = time;
			}
		}
	}

	if (!newName.empty()) {
		obs_source_t *sidechain = obs_get_source_by_name(newName.c_str());
		obs_weak_source_t *weakSidechain = sidechain ? obs_source_get_weak_source(sidechain) : nullptr;
		{
			std::lock_guard<std::mutex> lock(filterData->sidechainUpdateMutex);
			if (!filterData->sidechainName.empty() && filterData->sidechainName == newName) {
				filterData->weakSidechain = weakSidechain;
				weakSidechain = nullptr;
			}
		}
		if (sidechain) {
			// downmix or upmix if channel count is mismatched
			bool needsResampling = filterData->channels != static_cast<size_t>(sidechainChannels);
			if (needsResampling) {
				struct resample_info src = {};
				struct resample_info dst = {};
				src.samples_per_sec = filterData->sampleRate;
				src.format = AUDIO_FORMAT_FLOAT_PLANAR;
				src.speakers = convertSpeakerLayout(static_cast<uint8_t>(filterData->channels));

				dst.samples_per_sec = filterData->sampleRate;
				dst.format = AUDIO_FORMAT_FLOAT_PLANAR;
				dst.speakers = convertSpeakerLayout(static_cast<uint8_t>(sidechainChannels));

				audio_resampler *rawResampler = audio_resampler_create(&dst, &src);
				if (!rawResampler) {
					std::atomic_store(&filterData->sidechainResampler,
							  std::shared_ptr<audio_resampler>{});
				} else {
					std::shared_ptr<audio_resampler> sharedResampler(
						rawResampler, [](audio_resampler *resampler) {
							if (resampler) {
								audio_resampler_destroy(resampler);
							}
						});
					std::atomic_store(&filterData->sidechainResampler, sharedResampler);
				}
			} else {
				std::atomic_store(&filterData->sidechainResampler, std::shared_ptr<audio_resampler>{});
			}
			obs_source_add_audio_capture_callback(sidechain, sidechainCapture, filterData);
			obs_weak_source_release(weakSidechain);
			obs_source_release(sidechain);
		}
	}
	UNUSED_PARAMETER(seconds);
}

// ---------------- properties functions ---------------------

static bool vst3_show_gui_callback(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	auto *filterData = static_cast<Vst3AudioData *>(data);
	if (!filterData) {
		return false;
	}

	auto plugin = std::atomic_load(&filterData->plugin);
	if (!plugin) {
		return false;
	}

	bool noview = filterData->noview.load(std::memory_order_relaxed);
	if (noview) {
		return false;
	}

	if (!plugin->isEditorVisible()) {
		plugin->showEditor();
	} else {
		plugin->hideEditor();
	}

	return true;
}

static bool add_sources(void *data, obs_source_t *source)
{
	const auto *info = static_cast<struct sidechain_prop_info *>(data);
	const uint32_t caps = obs_source_get_output_flags(source);

	if (source == info->parent) {
		return true;
	}

	if ((caps & OBS_SOURCE_AUDIO) == 0) {
		return true;
	}

	const char *name = obs_source_get_name(source);
	obs_property_list_add_string(info->sources, name, name);
	return true;
}

bool on_vst3_changed_cb(void *priv, obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(settings);
	auto filterData = static_cast<struct Vst3AudioData *>(priv);
	if (!filterData) {
		return false;
	}

	const bool hasSidechain = filterData->hasSidechain.load(std::memory_order_relaxed);

	obs_property_t *gui = obs_properties_get(props, S_EDITOR);
	obs_property_set_visible(gui,
				 !filterData->noview.load(std::memory_order_relaxed) && !filterData->lastInitFailed);

	obs_property_t *p = obs_properties_get(props, S_SIDECHAIN_SOURCE);
	if (hasSidechain && !filterData->lastInitFailed) {
		obs_source_t *parent = obs_filter_get_parent(filterData->context);
		obs_property_list_clear(p);
		obs_property_list_add_string(p, obs_module_text("None"), "none");
		struct sidechain_prop_info info = {p, parent};
		obs_enum_sources(add_sources, &info);
		obs_property_set_visible(p, true);
	} else {
		obs_property_set_visible(p, false);
	}

	obs_property_t *noview = obs_properties_get(props, S_NOGUI);
	obs_property_set_visible(noview,
				 filterData->noview.load(std::memory_order_relaxed) && !filterData->lastInitFailed);

	obs_property_t *err = obs_properties_get(props, S_ERR);
	if (err) {
		obs_properties_remove_by_name(props, S_ERR);
	}
	if (filterData->lastInitFailed) {
		obs_property_t *err2 = obs_properties_add_text(props, S_ERR, TEXT_ERR, OBS_TEXT_INFO);
		obs_property_text_set_info_type(err2, OBS_TEXT_INFO_ERROR);
	}
	return true;
}

static bool vst3_rescan_callback(obs_properties_t *props, obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(data);

	if (!vst3ScanDone.exchange(false, std::memory_order_acq_rel)) {
		return false;
	}

	if (vst3ScanThread.joinable()) {
		vst3ScanThread.join();
	}

	vst3ScanThread = std::thread(scanVst3List, false);
	return true;
}

static obs_properties_t *vst3Properties(void *data)
{
	auto filterData = static_cast<struct Vst3AudioData *>(data);
	obs_properties_t *props = obs_properties_create();
	obs_property_t *sources;
	obs_property_t *rescan;
	obs_property_t *vst3list =
		obs_properties_add_list(props, S_PLUGIN, TEXT_PLUGIN, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(vst3list, obs_module_text("VST3.Select"), "");

	const bool scanDone = vst3ScanDone.load(std::memory_order_acquire);

	if (scanDone) {
		for (const auto &plugin : list->pluginList) {
			const bool multi = list->moduleHasMultipleClasses(plugin.path);
			std::string display = multi ? plugin.name + " (" + plugin.pluginName + ")" : plugin.name;
			std::string value = plugin.id;
			obs_property_list_add_string(vst3list, display.c_str(), value.c_str());
		}
	} else {
		obs_property_set_enabled(vst3list, false);
		obs_property_t *scan_err = obs_properties_add_text(props, S_SCAN, TEXT_SCAN, OBS_TEXT_INFO);
		obs_property_text_set_info_type(scan_err, OBS_TEXT_INFO_ERROR);
	}

	obs_property_t *gui = obs_properties_add_button2(props, S_EDITOR, obs_module_text(TEXT_EDITOR),
							 vst3_show_gui_callback, nullptr);
	obs_property_set_visible(gui,
				 !filterData->noview.load(std::memory_order_relaxed) && !filterData->lastInitFailed);

	sources = obs_properties_add_list(props, S_SIDECHAIN_SOURCE, TEXT_SIDECHAIN_SOURCE, OBS_COMBO_TYPE_LIST,
					  OBS_COMBO_FORMAT_STRING);
	obs_property_set_visible(sources, !filterData->lastInitFailed);

	obs_property_set_modified_callback2(vst3list, on_vst3_changed_cb, data);

	obs_property_t *noview = obs_properties_add_text(props, S_NOGUI, TEXT_NOGUI, OBS_TEXT_INFO);
	obs_property_text_set_info_type(noview, OBS_TEXT_INFO_WARNING);
	obs_property_set_visible(noview,
				 filterData->noview.load(std::memory_order_relaxed) && !filterData->lastInitFailed);

	rescan = obs_properties_add_button2(props, S_RESCAN, obs_module_text(TEXT_RESCAN), vst3_rescan_callback,
					    nullptr);
	obs_property_set_enabled(rescan, scanDone);

	if (filterData->lastInitFailed) {
		obs_property_t *err = obs_properties_add_text(props, S_ERR, TEXT_ERR, OBS_TEXT_INFO);
		obs_property_text_set_info_type(err, OBS_TEXT_INFO_ERROR);
	}

	return props;
}

void register_vst3_source()
{
	struct obs_source_info vst3_filter = {};
	vst3_filter.id = "vst3_filter";
	vst3_filter.type = OBS_SOURCE_TYPE_FILTER;
	vst3_filter.output_flags = OBS_SOURCE_AUDIO;
	vst3_filter.get_name = vst3Name;
	vst3_filter.create = vst3Create;
	vst3_filter.destroy = vst3Destroy;
	vst3_filter.update = vst3Update;
	vst3_filter.filter_audio = vst3FilterAudio;
	vst3_filter.get_properties = vst3Properties;
	vst3_filter.save = vst3Save;
	vst3_filter.video_tick = vst3Tick;
	obs_register_source(&vst3_filter);
}
