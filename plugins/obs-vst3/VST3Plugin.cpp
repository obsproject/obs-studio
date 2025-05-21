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

#include "VST3Plugin.h"

#include "obs-vst3.h"
#include "VST3EditorWindow.h"
#include "VST3HostApp.h"

#include <pluginterfaces/base/smartpointer.h>
#include <pluginterfaces/gui/iplugview.h>
#include <pluginterfaces/gui/iplugviewcontentscalesupport.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <pluginterfaces/vst/vstspeaker.h>
#include <public.sdk/source/common/memorystream.h>
#include <public.sdk/source/vst/hosting/module.h>
#include <public.sdk/source/vst/hosting/parameterchanges.h>
#include <public.sdk/source/vst/hosting/plugprovider.h>
#include <public.sdk/source/vst/hosting/processdata.h>

#include <obs-module.h>
#include <util/platform.h>

#include <QCoreApplication>
#include <QMetaObject>
#include <QPointer>

#include <algorithm>
#include <atomic>
#include <cstdint>
#ifdef __linux__
#include <cstdlib>
#endif
#include <memory>
#include <string>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Steinberg {
const FUID IPlugView::iid(0x5BC32507, 0xD06049EA, 0xA6151B52, 0x2B755B29);
const FUID IPlugViewContentScaleSupport::iid(0x65ED9690, 0x8AC44525, 0x8AADEF7A, 0x72EA703F);
const FUID IPlugFrame::iid(0x367FAF01, 0xAFA94693, 0x8D4DA2A0, 0xED0882A3);
} // namespace Steinberg

#ifdef __linux__
// Hack for LSP plugins which crash with NVIDIA drivers. Switch LSP UI rendering from OpenGL(GLX) to Cairo (software).
static void ensure_lsp_cairo_backend()
{
	setenv("LSP_WS_LIB_GLXSURFACE", "off", 1);
	infovst3plugin("Workaround for LSP plugins on linux; LSP_WS_LIB_GLXSURFACE=off set (forcing Cairo backend)\n");
}
#endif

VST3Plugin::VST3Plugin()
{
	if (!hostContext) {
		hostContext = getHostApp();
	}

	componentContext = new VST3ComponentHolder(this);

#ifdef __linux__
	ensure_lsp_cairo_backend();
#endif
};

VST3Plugin::~VST3Plugin()
{
	if (audioEffect) {
		audioEffect->setProcessing(false);
	}

	if (vstPlug) {
		vstPlug->setActive(false);
	}

	if (window) {
		hideEditor();

		if (view) {
			view->removed();
			view->setFrame(nullptr);
		}

		delete window;
		window = nullptr;
	}

	view = nullptr;

	processData.unprepare();

	plugProvider = nullptr;
	module = nullptr;

	delete componentContext;
	componentContext = nullptr;
}

void VST3Plugin::deactivateComponent() const
{
	vstPlug->setActive(false);
}

void VST3Plugin::setBusActive(MediaType type, BusDirection direction, int which, bool active) const
{
	vstPlug->activateBus(type, direction, which, active);
}

bool VST3Plugin::scanAudioBuses(SpeakerArrangement arr)
{
	int auxBusCount = 0;
	int mainBusCount = 0;
	numInputAudioBuses = vstPlug->getBusCount(MediaTypes::kAudio, BusDirections::kInput);
	numOutputAudioBuses = vstPlug->getBusCount(MediaTypes::kAudio, BusDirections::kOutput);

	infovst3plugin("Input audio buses: %i\n Output audio buses: %i\n", numInputAudioBuses, numOutputAudioBuses);

	inputAudioBusInfos.clear();
	inputSpeakerArrangements.clear();
	outputAudioBusInfos.clear();
	outputSpeakerArrangements.clear();
	// We enable the 1st compatible Main bus and the 1st Aux bus
	for (int i = 0; i < numInputAudioBuses; ++i) {
		BusInfo info = {};
		vstPlug->getBusInfo(kAudio, kInput, i, info);
		inputAudioBusInfos.push_back(info);
		// only 1 Main input bus is enabled by obs + 1 Aux (side-channel) Bus if it is available
		if (info.busType == Steinberg::Vst::BusTypes::kMain) {
			if (mainBusCount == 0) {
				setBusActive(kAudio, kInput, i, true);
				mainInputBusNumChannels = info.channelCount;
				mainInputBusIndex = i;
				numEnabledInputAudioBuses++;
				mainBusCount = 1;
				inputSpeakerArrangements.push_back(arr);
			} else {
				setBusActive(kAudio, kInput, i, false);
				SpeakerArrangement currentArrangement = {};
				if (audioEffect->getBusArrangement(kInput, i, currentArrangement) != kResultTrue) {
					return false;
				}
				inputSpeakerArrangements.push_back(currentArrangement);
			}
		} else {
			// The 1st aux bus (sidechain) is enabled only if it is mono or stereo
			if (auxBusCount == 0 && (info.channelCount == 1 || info.channelCount == 2)) {
				setBusActive(kAudio, kInput, i, true);
				SpeakerArrangement speakerArr = info.channelCount == 1
									? Steinberg::Vst::SpeakerArr::kMono
									: Steinberg::Vst::SpeakerArr::kStereo;
				inputSpeakerArrangements.push_back(speakerArr);
				numEnabledInputAudioBuses++;
				sidechainNumberChannels = info.channelCount;
				auxBusIndex = i;
				auxBusCount = 1;
			} else {
				setBusActive(kAudio, kInput, i, false);
				SpeakerArrangement currentArrangement = {};
				if (audioEffect->getBusArrangement(kInput, i, currentArrangement) != kResultTrue) {
					return false;
				}
				inputSpeakerArrangements.push_back(currentArrangement);
			}
		}
	}
	// We disable the plugin if it has no Input bus.
	if (!mainBusCount) {
		infovst3plugin(
			"No (main) input bus detected ! OBS VST3 Host only supports audio effects VST3 with 1 Main Input Bus (+ 1 Sidechannel Bus).");
		vstPlug->setActive(false);
		return false;
	}
	// Only the 1st Main output bus is enabled
	for (int i = 0; i < numOutputAudioBuses; ++i) {
		BusInfo info = {};
		vstPlug->getBusInfo(kAudio, kOutput, i, info);
		outputAudioBusInfos.push_back(info);
		bool isMain = (info.busType == Steinberg::Vst::BusTypes::kMain);
		if (isMain && !numEnabledOutputAudioBuses) {
			setBusActive(kAudio, kOutput, i, isMain);
			mainOutputBusIndex = i;
			mainOutputBusNumChannels = info.channelCount;
			numEnabledOutputAudioBuses++;
			outputSpeakerArrangements.push_back(arr);
		} else {
			setBusActive(kAudio, kOutput, i, false);
			SpeakerArrangement currentArrangement = {};
			if (audioEffect->getBusArrangement(kOutput, i, currentArrangement) != kResultTrue) {
				return false;
			}
			outputSpeakerArrangements.push_back(currentArrangement);
		}
	}
	if (!numEnabledOutputAudioBuses) {
		infovst3plugin(
			"No output bus detected ! OBS VST3 Host only supports audio effects VST3 with 1 Main output Bus.");
		return false;
	}

	return true;
}

bool VST3Plugin::init(const std::string &classId, const std::string &path_, int sampleRate, int max_blocksize,
		      SpeakerArrangement arrangement)
{
	std::string error;

	path = path_;

	sampleRate = sampleRate;
	maxBlockSize = max_blocksize;
	symbolicSampleSize = kSample32;
	realtime = kRealtime;

	processSetup.processMode = realtime;
	processSetup.symbolicSampleSize = symbolicSampleSize;
	processSetup.sampleRate = sampleRate;
	processSetup.maxSamplesPerBlock = maxBlockSize;

	processContext.state = ProcessContext::kPlaying | ProcessContext::kRecording | ProcessContext::kSystemTimeValid;
	processContext.sampleRate = sampleRate;

	processData.numSamples = 0;
	processData.symbolicSampleSize = symbolicSampleSize;
	processData.processContext = &processContext;

	module = VST3::Hosting::Module::create(path, error);
	if (!module) {
		infovst3plugin("%s", error.c_str());
		return false;
	}

	componentContext->guiToDsp = &guiToDsp;
	inputParameterChanges = std::make_unique<Steinberg::Vst::ParameterChanges>();
	outputParameterChanges = std::make_unique<Steinberg::Vst::ParameterChanges>();

	VST3::Hosting::PluginFactory factory = module->getFactory();
	factory.setHostContext(hostContext->getFUnknown());

	for (auto &classInfo : factory.classInfos()) {
		if (classInfo.category() == kVstAudioEffectClass && classInfo.ID().toString() == classId) {
			if (classId != classInfo.ID().toString()) {
				continue;
			}
			plugProvider = owned(new OBSPlugProvider(factory, classInfo, false));
			if (plugProvider->setup(hostContext->getFUnknown()) == false) {
				plugProvider = nullptr;
			}
			name = classInfo.name();
			break;
		}
	}
	if (!plugProvider) {
		infovst3plugin("No VST3 Audio Module Class with UID %s found. You probably uninstalled the VST3.",
			       classId.c_str());
		return false;
	}

	vstPlug = plugProvider->getComponentPtr();
	if (!vstPlug) {
		infovst3plugin("No VST3 Component class found.");
		return false;
	}

	editController = plugProvider->getControllerPtr();
	if (!editController) {
		infovst3plugin("No VST3 EditorController class found.");
		return false;
	} else {
		editController->setComponentHandler(componentContext->getComponentHandler());
	}

	const int32 paramCount = editController ? editController->getParameterCount() : 0;
	guiToDsp.setMaxParameters(paramCount > 0 ? paramCount : 256);
	dspToGui.setMaxParameters(paramCount > 0 ? paramCount : 256);

	audioEffect = FUnknownPtr<IAudioProcessor>(vstPlug).getInterface();
	if (!audioEffect) {
		infovst3plugin("Failed to get an audio processor from VST3");
		// try to get audioProcessor from EditorController, à la Juce, from badly coded VST3.
		audioEffect = FUnknownPtr<IAudioProcessor>(editController).getInterface();
		if (!audioEffect) {
			return false;
		}
	}

	if (!scanAudioBuses(arrangement)) {
		filterData->bypass.store(true, std::memory_order_relaxed);
		infovst3plugin("Error during the bus scan.");
		return false;
	}

	// Some plug-ins will crash if we pass a nullptr to setBusArrangements!
	SpeakerArrangement nullArrangement = {};
	auto *inputArrangements = inputSpeakerArrangements.empty() ? &nullArrangement : inputSpeakerArrangements.data();
	auto *outputArrangements = outputSpeakerArrangements.empty() ? &nullArrangement
								     : outputSpeakerArrangements.data();
	tresult res = audioEffect->setBusArrangements(inputArrangements, numInputAudioBuses, outputArrangements,
						      numOutputAudioBuses);
	if (res != kResultTrue) {
		SpeakerArrangement speakerArrangement;
		if (audioEffect->getBusArrangement(kInput, mainInputBusIndex, speakerArrangement) != kResultTrue) {
			return false;
		}
		if (speakerArrangement != arrangement) {
			infovst3plugin("Failed to set input bus to obs speaker layout.");
			return false;
		}

		if (numEnabledInputAudioBuses == 2) {
			if (audioEffect->getBusArrangement(kInput, auxBusIndex, speakerArrangement) != kResultTrue) {
				return false;
			}
			SpeakerArrangement sideArr = sidechainNumberChannels == 1 ? Steinberg::Vst::SpeakerArr::kMono
										  : Steinberg::Vst::SpeakerArr::kStereo;
			if (speakerArrangement != sideArr) {
				infovst3plugin("Failed to set side chain bus to desired speaker layout!");
				return false;
			}
		}

		if (audioEffect->getBusArrangement(kOutput, mainOutputBusIndex, speakerArrangement) != kResultTrue) {
			return false;
		}
		if (speakerArrangement != arrangement) {
			infovst3plugin("Failed to set output bus to obs speaker layout.");
			return false;
		}
	}

	res = audioEffect->setupProcessing(processSetup);
	if (res == kResultOk) {
		processData.prepare(*vstPlug, maxBlockSize, processSetup.symbolicSampleSize);
		// silence outputs on preparation, better safe than sorry
		for (int32 busIdx = 0; busIdx < processData.numOutputs; ++busIdx) {
			auto &bus = processData.outputs[busIdx];

			if (bus.channelBuffers32) {
				for (int32 ch = 0; ch < bus.numChannels; ++ch) {
					std::fill_n(bus.channelBuffers32[ch], maxBlockSize, 0.0f);
				}
			}
		}
	} else {
		infovst3plugin("Failed to setup VST3 processing.");
		return false;
	}

	if (vstPlug->setActive(true) != kResultTrue) {
		infovst3plugin("Failed to activate VST3 component.");
		return false;
	}
	// this often reports 0 in my tests, which probably means that the VST3 authors didn't really measure the value, lol
	uint32 latency = audioEffect->getLatencySamples();
	infovst3plugin("Latency of the plugin is %i samples", latency);

	return true;
}

void VST3Plugin::drainDspToGui()
{
	uiDrainScheduled.store(false, std::memory_order_release);

	ParamID id;
	ParamValue value;
	int32 sampleOffset;

	while (dspToGui.getNextChange(id, value, sampleOffset)) {
		if (editController) {
			editController->setParamNormalized(id, value);
		}
	}
}

void VST3Plugin::preprocess()
{
	inputParameterChanges->clearQueue();
	outputParameterChanges->clearQueue();
	processData.inputParameterChanges = inputParameterChanges.get();
	processData.outputParameterChanges = outputParameterChanges.get();
	guiToDsp.transferChangesTo(*inputParameterChanges);
}

void VST3Plugin::postprocess()
{
	if (!processData.outputParameterChanges || outputParameterChanges->getParameterCount() == 0) {
		return;
	}

	dspToGui.transferChangesFrom(*outputParameterChanges);

	if (!uiDrainScheduled.exchange(true, std::memory_order_acq_rel)) {
		QObject *receiver = QCoreApplication::instance();
		if (receiver) {
			QPointer<VST3Plugin> self(this);
			QMetaObject::invokeMethod(
				receiver,
				[self] {
					if (self) {
						self->drainDspToGui();
					}
				},
				Qt::QueuedConnection);
		} else {
			uiDrainScheduled.store(false, std::memory_order_release);
		}
	}
}

void VST3Plugin::setProcessing(bool processing) const
{
	audioEffect->setProcessing(processing);
}

bool VST3Plugin::process(int numSamples)
{
	if (!audioEffect) {
		return false;
	}

	preprocess();

	if (numSamples > maxBlockSize) {
#ifdef _DEBUG
		infovst3plugin("numSamples > _maxBlockSize");
#endif
		numSamples = maxBlockSize;
	}

	processData.numSamples = numSamples;
	processContext.projectTimeSamples += numSamples;
	processContext.systemTime = static_cast<int64>(os_gettime_ns());

	tresult result = audioEffect->process(processData);

	if (result != kResultOk) {
		return false;
	}

	postprocess();

	return true;
}

Steinberg::Vst::Sample32 *VST3Plugin::channelBuffer32(const BusDirection direction, const int ch) const
{
	if (direction == kInput) {
		return processData.inputs[mainInputBusIndex].channelBuffers32[ch];
	} else if (direction == kOutput) {
		return processData.outputs[mainOutputBusIndex].channelBuffers32[ch];
	} else {
		return nullptr;
	}
}

Steinberg::Vst::Sample32 *VST3Plugin::auxChannelBuffer32(const BusDirection direction, const int ch) const
{
	if (direction == kInput) {
		return processData.inputs[auxBusIndex].channelBuffers32[ch];
	} else {
		return nullptr;
	}
}

/* hack ripped from Juce, to create the view even with badly coded VST3s... */
void VST3Plugin::tryCreatingView()
{
	if (auto *raw = editController->createView(Vst::ViewType::kEditor)) {
		view = IPtr<IPlugView>::adopt(raw);
		return;
	}

	if (auto *raw = editController->createView(nullptr)) {
		view = IPtr<IPlugView>::adopt(raw);
		return;
	}

	IPlugView *raw = nullptr;
	if (editController->queryInterface(IPlugView::iid, reinterpret_cast<void **>(&raw)) == kResultOk) {
		view = IPtr<IPlugView>::adopt(raw);
	}
}

bool VST3Plugin::createView()
{
	if (!editController) {
		infovst3plugin("VST3 does not provide an edit controller");
		return false;
	}

	if (view) {
		debugvst3plugin("Editor view or window already exists");
		return false;
	} else {
		tryCreatingView();
	}

	if (!view) {
		infovst3plugin("EditController does not provide its own view");
		return false;
	}

#ifdef _WIN32
	if (view->isPlatformTypeSupported(Steinberg::kPlatformTypeHWND) != Steinberg::kResultTrue) {
		infovst3plugin("Editor view does not support HWND");
		return false;
	}
#elif defined(__APPLE__)
	if (view->isPlatformTypeSupported(Steinberg::kPlatformTypeNSView) != Steinberg::kResultTrue) {
		infovst3plugin("Editor view does not support NSView");
		return false;
	}
#elif defined(__linux__)
	if (hostContext->backend() == VST3Backend::X11 &&
	    view->isPlatformTypeSupported(Steinberg::kPlatformTypeX11EmbedWindowID) != Steinberg::kResultTrue) {
		infovst3plugin("Editor view does not support X11");
		return false;
	}
	if (hostContext->backend() == VST3Backend::Wayland) {
		infovst3plugin(
			"Our VST3 host does not support Wayland for GUI. VST3s will be active in headless mode.");
		return false;
	}
#else
	infovst3plugin("Platform is not supported yet");
	return false;
#endif

	return true;
}

void VST3Plugin::showEditor()
{
	if (!view) {
		return;
	}

	if (!window) {
		int width = 800;
		int height = 600;
		Steinberg::ViewRect rect;
		if (view->getSize(&rect) == Steinberg::kResultOk) {
			width = rect.getWidth();
			height = rect.getHeight();
		} else {
			infovst3plugin("Failed to get size before attaching an IFrame. Not SDK compliant.");
		}
		std::string sourceName = obs_source_get_name(filterData->context);
		std::string windowName = sourceName + ": VST3 Plugin - " + name;

		window = new VST3EditorWindow(view, windowName);

		if (window->create(width, height)) {
			window->show();
			editorVisible = true;
		} else {
			infovst3plugin("Failed to create editor window");
			delete window;
			window = nullptr;
		}
	} else {
		window->show();
		editorVisible = true;
	}
}

void VST3Plugin::hideEditor()
{
	if (window && view) {
		window->close();
	}
	editorVisible = false;
}

// This function is required because we don't really close the GUI window; we hide it. This then means we have to track
// when a GUI has been closed by the user when clicking X. I decided to just hide because creating the GUI each time
// the user wants to display it, was prone to crashes. This also simplified the coding.
bool VST3Plugin::isEditorVisible()
{
	if (window) {
		bool wasClosed = window->getClosedState();
		if (wasClosed && editorVisible) {
			editorVisible = false;
		}
	}
	return editorVisible;
}

bool VST3Plugin::saveStates(std::vector<uint8_t> &compOut, std::vector<uint8_t> &ctrlOut) const
{
	compOut.clear();
	ctrlOut.clear();

	if (!vstPlug) {
		return false;
	}

	{
		Steinberg::MemoryStream s;
		if (vstPlug->getState(&s) != Steinberg::kResultOk) {
			return false;
		}

		Steinberg::int64 size = 0;
		Steinberg::int64 seekRes = 0;
		s.tell(&size);
		if (size <= 0) {
			return false;
		}

		compOut.resize(static_cast<size_t>(size));
		s.seek(0, Steinberg::IBStream::kIBSeekSet, &seekRes);
		Steinberg::int32 actuallyRead = 0;
		s.read(compOut.data(), static_cast<Steinberg::int32>(size), &actuallyRead);
		if (actuallyRead < size) {
			compOut.resize(static_cast<size_t>(actuallyRead));
		}
	}

	if (editController) {
		Steinberg::MemoryStream s;
		if (editController->getState(&s) == Steinberg::kResultOk) {
			Steinberg::int64 size = 0, seekRes = 0;
			s.tell(&size);
			if (size > 0) {
				ctrlOut.resize(static_cast<size_t>(size));
				s.seek(0, Steinberg::IBStream::kIBSeekSet, &seekRes);
				Steinberg::int32 actuallyRead = 0;
				s.read(ctrlOut.data(), static_cast<Steinberg::int32>(size), &actuallyRead);
				if (actuallyRead < size) {
					ctrlOut.resize(static_cast<size_t>(actuallyRead));
				}
			}
		}
	}

	return true;
}

bool VST3Plugin::loadStates(const std::vector<uint8_t> &comp, const std::vector<uint8_t> &ctrl) const
{
	if (!vstPlug || comp.empty()) {
		return false;
	}

	Steinberg::MemoryStream compStream;
	Steinberg::int32 w = 0;
	Steinberg::int64 dummy = 0;

	compStream.write((void *)comp.data(), static_cast<Steinberg::int32>(comp.size()), &w);
	compStream.seek(0, Steinberg::IBStream::kIBSeekSet, &dummy);
	if (vstPlug->setState(&compStream) != Steinberg::kResultOk) {
		return false;
	}

	if (editController) {
		compStream.seek(0, Steinberg::IBStream::kIBSeekSet, &dummy);
		(void)editController->setComponentState(&compStream);
		if (!ctrl.empty()) {
			Steinberg::MemoryStream ctrlStream;
			ctrlStream.write((void *)ctrl.data(), static_cast<Steinberg::int32>(ctrl.size()), &w);
			ctrlStream.seek(0, Steinberg::IBStream::kIBSeekSet, &dummy);
			(void)editController->setState(&ctrlStream);
		}
	}
	return true;
}
