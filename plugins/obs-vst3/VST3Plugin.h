/******************************************************************************
    Copyright (C) 2025-2026 pkv <pkv@obsproject.com>
    This file is part of obs-vst3.
    It uses the Steinberg VST3 SDK, which is licensed under MIT license.
    See https://github.com/steinbergmedia/vst3sdk for details.
    Portions are derived from EasyVst (https://github.com/iffyloop/EasyVst),
    licensed under Public Domain (Unlicense) or MIT No Attribution.

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
#pragma once
#include "obs-vst3.h"
#include "VST3ParamEditQueue.h"

// on linux Qt must be included before X11 headers
#include "VST3ComponentHolder.h"

#include <QObject>
#include "pluginterfaces/base/smartpointer.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstmessage.h"

#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/hosting/processdata.h"

#ifdef __linux__
struct _XDisplay;
typedef _XDisplay Display;
class RunLoopImpl;
#endif

#define do_logVST3(level, format, ...) \
	blog(level, "[VST3 Plugin:] " format, ## __VA_ARGS__)

#define warnvst3plugin(format, ...) do_logVST3(LOG_WARNING, format, ## __VA_ARGS__)
#define infovst3plugin(format, ...) do_logVST3(LOG_INFO, format, ## __VA_ARGS__)
#define debugvst3plugin(format, ...) do_logVST3(LOG_DEBUG, format, ## __VA_ARGS__)

using namespace Steinberg;
using namespace Vst;

class VST3EditorWindow;
class VST3ComponentHolder;
class VST3HostApp;

/**
 * PlugProvider is a helper class from the sdk which takes care of initializing an IComponent & an IEditController w/
 * the hostContext & detects if the plugin bundled the 2 interfaces; in case they are distinct as advised by the SDK,
 * it takes care of connecting them as IConnectionPoints. We wrap it into our own tiny class to be able to do the setup
 * directly.
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
	VST3HostApp *hostContext = nullptr;
	VST3ComponentHolder *componentContext = nullptr;
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
	VST3ParamEditQueue guiToDsp;
	VST3ParamEditQueue dspToGui;
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
	Display *display;
	RunLoopImpl *runLoop;
#endif
};
