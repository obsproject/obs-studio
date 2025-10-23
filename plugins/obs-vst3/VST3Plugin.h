/* Copyright (c) 2025 pkv <pkv@obsproject.com>
 * This file is part of obs-vst3.
 *
 * Portions are derived from EasyVst (https://github.com/iffyloop/EasyVst),
 * licensed under Public Domain (Unlicense) or MIT No Attribution.
 *
 * This file uses the Steinberg VST3 SDK, which is licensed under MIT license.
 * See https://github.com/steinbergmedia/vst3sdk for details.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */

#pragma once
#include <obs-module.h>
#include <media-io/audio-resampler.h>
#include <util/deque.h>
#include <util/platform.h>
#include <util/threading.h>

#include <vector>
#include <string>
#include <mutex>
#include <atomic>

#include <QObject>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
#include "editor/linux/RunLoopImpl.h"
#endif

#include "sdk/public.sdk/source/vst/hosting/plugprovider.h"
#include "sdk/public.sdk/source/vst/hosting/module.h"
#include "sdk/public.sdk/source/vst/hosting/hostclasses.h"
#include "sdk/public.sdk/source/vst/hosting/parameterchanges.h"
#include "sdk/public.sdk/source/vst/hosting/processdata.h"
#include "sdk/pluginterfaces/base/smartpointer.h"
#include "sdk/pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "sdk/pluginterfaces/vst/ivsteditcontroller.h"
#include "sdk/pluginterfaces/vst/ivstprocesscontext.h"
#include "sdk/pluginterfaces/vst/ivstaudioprocessor.h"
#include "sdk/pluginterfaces/vst/ivstunits.h"
#include "sdk/pluginterfaces/vst/ivstmessage.h"

#define MAX_PREPROC_CHANNELS 8
#define MAX_SC_CHANNELS 2
#define BUFFER_SIZE_MSEC 10
#define FRAME_SIZE 480

class VST3EditorWindow;
class VST3Plugin;

using namespace Steinberg;
using namespace Vst;

struct vst3_audio_data {
	obs_source_t *context;

	std::string vst3_id;
	std::string vst3_path;
	std::string vst3_name;

	uint64_t last_timestamp;
	uint64_t latency;

	size_t frames;
	size_t channels;
	speaker_layout layout;

	struct deque info_buffer;
	struct deque input_buffers[MAX_PREPROC_CHANNELS];
	struct deque output_buffers[MAX_PREPROC_CHANNELS];

	/* Sidechain data */
	size_t sc_channels;
	uint64_t sc_last_timestamp;
	uint64_t sc_latency;
	struct deque sc_input_buffers[MAX_PREPROC_CHANNELS];

	bool has_mono_src;
	volatile bool reinit_done;

	uint32_t sample_rate;
	unsigned int num_samples_per_frame, num_channels;

	/* PCM buffers */
	float *copy_buffers[MAX_PREPROC_CHANNELS];
	float *sc_copy_buffers[MAX_PREPROC_CHANNELS];

	/* output data */
	struct obs_audio_data output_audio;
	DARRAY(float) output_data;

	VST3Plugin *plugin = nullptr;
	int64_t running_sample_count = 0;
	uint64_t system_time = 0;

	volatile bool bypass;
	bool noview;
	std::atomic_flag init_in_progress = ATOMIC_FLAG_INIT;
	std::mutex plugin_state_mutex;

	/* Sidechain */
	bool has_sidechain;
	bool sidechain_enabled;
	obs_weak_source_t *weak_sidechain;
	std::string sidechain_name;
	uint64_t sidechain_check_time;
	std::mutex sidechain_update_mutex;
	/* Resampler for sidechain */
	audio_resampler_t *sc_resampler;
};

struct ParamEdit {
	ParamID id;
	ParamValue value;
};

class ParamEditQueue {
	static constexpr size_t CAP = 1024;
	ParamEdit buf[CAP];
	std::atomic<size_t> head{0}, tail{0};

public:
	bool push(const ParamEdit &e)
	{
		size_t h = head.load(std::memory_order_relaxed);
		size_t n = (h + 1) % CAP;
		if (n == tail.load(std::memory_order_acquire))
			return false; // full
		buf[h] = e;
		head.store(n, std::memory_order_release);
		return true;
	}
	bool pop(ParamEdit &out)
	{
		size_t t = tail.load(std::memory_order_relaxed);
		if (t == head.load(std::memory_order_acquire))
			return false;
		out = buf[t];
		tail.store((t + 1) % CAP, std::memory_order_release);
		return true;
	}
};

/**
 * This usually is a global object for the whole host; but to insulate each instantiation of a VST3, each VST3 will
 * have its own host context and on linux, its own runloop.
 */
class VST3HostApp : public IComponentHandler,
		    public IUnitHandler,
		    public IHostApplication,
		    public IPlugInterfaceSupport {
public:
	VST3HostApp()
	{
		// define plugin interfaces which are supported
		addPlugInterfaceSupported(IComponent::iid);
		addPlugInterfaceSupported(IAudioProcessor::iid);
		addPlugInterfaceSupported(IEditController::iid);
		addPlugInterfaceSupported(IConnectionPoint::iid);
	}
	virtual ~VST3HostApp() noexcept {FUNKNOWN_DTOR}

	//========== IComponentHandler methods ===========//
	ParamEditQueue *guiToDsp = nullptr;

	tresult PLUGIN_API beginEdit(ParamID) override { return kResultOk; }

	tresult PLUGIN_API performEdit(ParamID id, ParamValue valueNormalized) override
	{
		if (guiToDsp)
			guiToDsp->push({id, valueNormalized});
		return kResultOk;
	}

	tresult PLUGIN_API endEdit(ParamID) override { return kResultOk; }

	tresult PLUGIN_API restartComponent(int32 flags) override;

	//========== IHostApplication methods ===========//
	tresult PLUGIN_API getName(String128 name) override
	{
		std::memset(name, 0, sizeof(String128));
#if defined(_WIN32)
		const char *src = "OBS VST3 Host";
		MultiByteToWideChar(CP_UTF8, 0, src, -1, (wchar_t *)name, 128);
#else
		std::u16string src = u"OBS VST3 Host";
		src.copy(name, src.size());
#endif
		return kResultOk;
	}

	tresult PLUGIN_API createInstance(TUID cid, TUID _iid, void **obj) override
	{
		if (FUnknownPrivate::iidEqual(cid, IMessage::iid) && FUnknownPrivate::iidEqual(_iid, IMessage::iid)) {
			*obj = new HostMessage;
			return kResultTrue;
		}
		if (FUnknownPrivate::iidEqual(cid, IAttributeList::iid) &&
		    FUnknownPrivate::iidEqual(_iid, IAttributeList::iid)) {
			if (auto al = HostAttributeList::make()) {
				*obj = al.take();
				return kResultTrue;
			}
			return kOutOfMemory;
		}
		*obj = nullptr;
		return kResultFalse;
	}

	//========== IIUnitHandler methods ===========//
	tresult PLUGIN_API notifyUnitSelection(UnitID) override { return kResultTrue; }
	tresult PLUGIN_API notifyProgramListChange(ProgramListID, int32) override { return kResultTrue; }

	//========== IPlugInterfaceSupport methods ===========//
	tresult PLUGIN_API isPlugInterfaceSupported(const TUID _iid) override
	{
		auto uid = FUID::fromTUID(_iid);
		if (std::find(mFUIDArray.begin(), mFUIDArray.end(), uid) != mFUIDArray.end())
			return kResultTrue;
		return kResultFalse;
	}

	//========== FUnknown methods ===========//
	tresult PLUGIN_API queryInterface(const TUID _iid, void **obj) override
	{
		if (FUnknownPrivate::iidEqual(_iid, IHostApplication::iid)) {
			*obj = static_cast<IHostApplication *>(this);
			return kResultOk;
		}
		if (FUnknownPrivate::iidEqual(_iid, IPlugInterfaceSupport::iid)) {
			*obj = static_cast<IPlugInterfaceSupport *>(this);
			return kResultOk;
		}
#ifdef __linux__
		if (runLoop && FUnknownPrivate::iidEqual(_iid, Linux::IRunLoop::iid)) {
			*obj = static_cast<Linux::IRunLoop *>(runLoop);
			return kResultOk;
		}
#endif
		*obj = nullptr;
		return kNoInterface;
	}
	// we do not care here of the ref-counting. A plug-in call of release should not destroy this class !
	uint32 PLUGIN_API addRef() override { return 1000; }
	uint32 PLUGIN_API release() override { return 1000; }

	IComponentHandler *getComponentHandler() noexcept { return static_cast<IComponentHandler *>(this); }

	FUnknown *getFUnknown() noexcept { return static_cast<FUnknown *>(static_cast<IHostApplication *>(this)); }

	//========== VST3 plugin ==========//
	VST3Plugin *plugin = nullptr;

private:
	std::vector<FUID> mFUIDArray;
	void addPlugInterfaceSupported(const TUID _iid) { mFUIDArray.push_back(FUID::fromTUID(_iid)); }

#ifdef __linux__
	//========== Pass Runloop ===========//
public:
	void setRunLoop(Linux::IRunLoop *rl) { runLoop = rl; }

private:
	Linux::IRunLoop *runLoop = nullptr;
#endif
};

/**
 * PlugProvider is a helper class from the sdk which takes care of initializing an IComponent & an IEditController w/
 * the hostContext & detects if the plugin bundled the 2 interfaces; in case they are distinct as advised by the SDK,
 * it takes care of connecting them as IConnectionPoints. We wrap it into our own tiny class to remove quirks due to
 * using a singleton host context.
 * */
class OBSPlugProvider : public PlugProvider {
public:
	using PlugProvider::PlugProvider;
	bool setup(FUnknown *context) { return setupPlugin(context); }
};

class VST3Plugin : public QObject {
	Q_OBJECT
public:
	VST3Plugin();
	~VST3Plugin();

	// glue to obs-vst3 filter
	struct vst3_audio_data *obsVst3Struct = nullptr;

	// Basic Interfaces
	VST3HostApp *pluginContext = nullptr;
	VST3::Hosting::Module::Ptr module = nullptr;
	IPtr<OBSPlugProvider> plugProvider = nullptr;

	IComponent *vstPlug = nullptr;
	IAudioProcessor *audioEffect = nullptr;
	IEditController *editController = nullptr;

	HostProcessData processData = {};
	ProcessSetup processSetup = {};
	ProcessContext processContext = {};

	IPtr<IPlugView> view = nullptr;

	// load and save VST3 state
	bool loadStates(const std::vector<uint8_t> &comp, const std::vector<uint8_t> &ctrl);
	bool saveStates(std::vector<uint8_t> &compOut, std::vector<uint8_t> &ctrlOut) const;

	// initialization stage
	bool scanAudioBuses(SpeakerArrangement arr);
	void setBusActive(MediaType type, BusDirection direction, int which, bool active) const;
	bool init(const std::string &classId, const std::string &path, int sampleRate, int maxBlockSize,
		  SpeakerArrangement arr);
	void deactivateComponent() const;

	// processing stage
	void setProcessing(bool processing) const;
	bool process(int numSamples);
	void preprocess();
	void postprocess();
	[[nodiscard]] Sample32 *channelBuffer32(BusDirection direction, int which) const;
	[[nodiscard]] Sample32 *sidechannelBuffer32(BusDirection direction, int ch) const;
	// GUI Editor window
	void tryCreatingView();
	bool createView();
	void showEditor();
	void hideEditor();
	VST3EditorWindow *window = nullptr;
	bool editorVisible = false;
	bool isEditorVisible();

	// Communication EditController <=> Processor IComponent / IAudioProcessor
	ParamEditQueue guiToDsp;
	ParamEditQueue dspToGui;
	std::unique_ptr<ParameterChanges> inChanges;
	std::unique_ptr<ParameterChanges> outChanges;
	std::atomic<bool> uiDrainScheduled{false};
	void drainDspToGui();

	// buses & audio
	int sampleRate = 0, maxBlockSize = 0, symbolicSampleSize = 0;
	bool realtime = kRealtime;
	std::vector<BusInfo> inAudioBusInfos, outAudioBusInfos;
	int numInAudioBuses = 0, numOutAudioBuses = 0;
	int numEnabledInAudioBuses = 0, numEnabledOutAudioBuses = 0;
	int mainInputBusNumChannels = 0, mainOutputBusNumChannels = 0;
	int sidechainNumChannels = 0;
	int mainInputBusIndex = 0, mainOutputBusIndex = 0;
	int auxBusIndex = 0;
	std::vector<SpeakerArrangement> inSpeakerArrs, outSpeakerArrs;

	// misc
	std::string path;
	std::string name;

#ifdef __linux__
	void stopRunLoop() const;
	Display *display;
	RunLoopImpl *runLoop;
#endif
};
