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

#include "VST3ComponentHolder.h"

#include "pluginterfaces/base/smartpointer.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "public.sdk/source/vst/hosting/processdata.h"

#include <QObject>

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
struct vst3_audio_data;

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

	VST3Plugin(const VST3Plugin &) = delete;
	VST3Plugin &operator=(const VST3Plugin &) = delete;
	VST3Plugin(VST3Plugin &&) = delete;
	VST3Plugin &operator=(VST3Plugin &&) = delete;

	struct vst3_audio_data *obsVst3Data = nullptr;

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

	bool loadStates(const std::vector<uint8_t> &comp, const std::vector<uint8_t> &ctrl);
	bool saveStates(std::vector<uint8_t> &compOut, std::vector<uint8_t> &ctrlOut) const;

	bool scanAudioBuses(SpeakerArrangement arr);
	void setBusActive(MediaType type, BusDirection direction, int which, bool active) const;
	bool init(const std::string &classId, const std::string &path, int sampleRate, int maxBlockSize,
		  SpeakerArrangement arr);
	void deactivateComponent() const;

	void setProcessing(bool processing) const;
	bool process(int numSamples);
	void preprocess();
	void postprocess();
	[[nodiscard]] Sample32 *channelBuffer32(BusDirection direction, int which) const;
	[[nodiscard]] Sample32 *auxChannelBuffer32(BusDirection direction, int ch) const;

	void tryCreatingView();
	bool createView();
	void showEditor();
	void hideEditor();
	VST3EditorWindow *window = nullptr;
	bool editorVisible = false;
	bool isEditorVisible();

	ParameterChangeTransfer guiToDsp;
	ParameterChangeTransfer dspToGui;
	std::unique_ptr<ParameterChanges> inputParameterChanges;
	std::unique_ptr<ParameterChanges> outputParameterChanges;
	std::atomic<bool> uiDrainScheduled{false};
	void drainDspToGui();

	int sampleRate = 0;
	int maxBlockSize = 0;
	int symbolicSampleSize = 0;
	bool realtime = kRealtime;
	std::vector<BusInfo> inputAudioBusInfos, outputAudioBusInfos;
	int numInputAudioBuses = 0;
	int numOutputAudioBuses = 0;
	int numEnabledInputAudioBuses = 0;
	int numEnabledOutputAudioBuses = 0;
	int mainInputBusNumChannels = 0;
	int mainOutputBusNumChannels = 0;
	int sidechainNumChannels = 0;
	int mainInputBusIndex = 0;
	int mainOutputBusIndex = 0;
	int auxBusIndex = 0;
	std::vector<SpeakerArrangement> inputSpeakerArrangements, outputSpeakerArrangements;

	std::string path;
	std::string name;

#ifdef __linux__
	Display *display = nullptr;
	RunLoopImpl *runLoop = nullptr;
#endif
};
