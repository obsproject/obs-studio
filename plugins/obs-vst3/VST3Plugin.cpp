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
// on linux Qt must be included before X11 headers
#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>
#include <VST3Plugin.h>
#include "VST3HostApp.h"
#include "VST3ComponentHolder.h"
#include "VST3EditorWindow.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/common/memorystream.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Steinberg {
const FUID IPlugView::iid(0x5BC32507, 0xD06049EA, 0xA6151B52, 0x2B755B29);
const FUID IPlugViewContentScaleSupport::iid(0x65ED9690, 0x8AC44525, 0x8AADEF7A, 0x72EA703F);
const FUID IPlugFrame::iid(0x367FAF01, 0xAFA94693, 0x8D4DA2A0, 0xED0882A3);
#if defined(__linux__)
const FUID Linux::IRunLoop::iid(0x18C35366, 0x97764F1A, 0x9C5B8385, 0x7A871389);
#endif
} // namespace Steinberg

extern VST3HostApp *g_host_app;
#ifdef __linux__
extern RunLoopImpl *g_run_loop;
extern Display *g_display;
/* Fix for LSP plugins which crash with NVIDIA drivers. Switch LSP UI rendering from OpenGL(GLX) -> Cairo (software). */
static void ensure_lsp_cairo_backend()
{
	setenv("LSP_WS_LIB_GLXSURFACE", "off", 1);
	infovst3plugin("Workaround for LSP plugins on linux; LSP_WS_LIB_GLXSURFACE=off set (forcing Cairo backend)\n");
}
#endif

VST3Plugin::VST3Plugin()
{
	if (qApp && QThread::currentThread() != qApp->thread())
		this->moveToThread(qApp->thread());

	if (!hostContext) {
		hostContext = g_host_app;
	}

	componentContext = new VST3ComponentHolder(this);

#ifdef __linux__
	ensure_lsp_cairo_backend();
	display = g_display;
	XSync(display, False);
	runLoop = g_run_loop;
#endif
};

VST3Plugin::~VST3Plugin()
{
	if (view) {
		if (window) {
			hideEditor();
			view->removed();
			view->setFrame(nullptr);
		}
	}
	view = nullptr;

	if (window) {
		delete window;
		window = nullptr;
	}

	if (processData.inputEvents) {
		auto *in = dynamic_cast<Steinberg::Vst::EventList *>(processData.inputEvents);
		delete in;
		processData.inputEvents = nullptr;
	}
	if (processData.outputEvents) {
		auto *out = dynamic_cast<Steinberg::Vst::EventList *>(processData.outputEvents);
		delete out;
		processData.outputEvents = nullptr;
	}
	processData.unprepare();
	processData = {};
	processSetup = {};
	processContext = {};

	plugProvider = nullptr;
	module = nullptr;

	inAudioBusInfos.clear();
	outAudioBusInfos.clear();
	numInAudioBuses = 0;
	numOutAudioBuses = 0;
	inSpeakerArrs.clear();
	outSpeakerArrs.clear();
	sampleRate = 0;
	maxBlockSize = 0;
	symbolicSampleSize = 0;
	realtime = false;

	path = "";
	name = "";

	if (componentContext) {
		delete componentContext;
	}
}

void VST3Plugin::deactivateComponent() const
{
	vstPlug->setActive(false);
}

/** Buses :enable, scans and sets up input/output audio buses (called during init) */
void VST3Plugin::setBusActive(MediaType type, BusDirection direction, int which, bool active) const
{
	vstPlug->activateBus(type, direction, which, active);
}

bool VST3Plugin::scanAudioBuses(SpeakerArrangement arr)
{
	int countAuxBus = 0;
	int countMainBus = 0;
	numInAudioBuses = vstPlug->getBusCount(MediaTypes::kAudio, BusDirections::kInput);
	numOutAudioBuses = vstPlug->getBusCount(MediaTypes::kAudio, BusDirections::kOutput);

	infovst3plugin("Input audio buses: %i\n Output audio buses: %i\n", numInAudioBuses, numOutAudioBuses);

	inAudioBusInfos.clear();
	inSpeakerArrs.clear();
	outAudioBusInfos.clear();
	outSpeakerArrs.clear();
	// We enable the 1st compatible Main bus and the 1st Aux bus
	for (int i = 0; i < numInAudioBuses; ++i) {
		BusInfo info = {};
		vstPlug->getBusInfo(kAudio, kInput, i, info);
		inAudioBusInfos.push_back(info);
		bool isMain = (info.busType == Steinberg::Vst::BusTypes::kMain);
		// only 1 Main input bus is enabled by obs + 1 Aux (side-channel) Bus if it is available
		if (isMain) {
			if (countMainBus == 0) {
				setBusActive(kAudio, kInput, i, true);
				mainInputBusNumChannels = info.channelCount;
				mainInputBusIndex = i;
				inSpeakerArrs.push_back(arr);
				numEnabledInAudioBuses++;
				countMainBus = 1;
			} else {
				setBusActive(kAudio, kInput, i, false);
			}
		} else {
			// The 1st aux bus (sidechannel) is enabled only if it is mono or stereo
			if (countAuxBus == 0 && (info.channelCount == 1 || info.channelCount == 2)) {
				setBusActive(kAudio, kInput, i, true);
				SpeakerArrangement speakerArr = info.channelCount == 1
									? Steinberg::Vst::SpeakerArr::kMono
									: Steinberg::Vst::SpeakerArr::kStereo;
				inSpeakerArrs.push_back(speakerArr);
				numEnabledInAudioBuses++;
				sidechainNumChannels = info.channelCount;
				auxBusIndex = i;
				countAuxBus = 1;
			} else {
				setBusActive(kAudio, kInput, i, false);
			}
		}
	}
	// We disable the plugin if it has no Input bus.
	if (!numEnabledInAudioBuses) {
		infovst3plugin(
			"No input bus detected ! OBS VST3 Host only supports audio effects VST3 with 1 Main Input Bus (+ 1 Sidechannel Bus).");
		vstPlug->setActive(false);
		return false;
	}
	// Only the 1st Main output bus is enabled
	for (int i = 0; i < numOutAudioBuses; ++i) {
		BusInfo info = {};
		vstPlug->getBusInfo(kAudio, kOutput, i, info);
		outAudioBusInfos.push_back(info);
		bool isMain = (info.busType == Steinberg::Vst::BusTypes::kMain);
		if (isMain && !numEnabledOutAudioBuses) {
			setBusActive(kAudio, kOutput, i, isMain);
			mainOutputBusIndex = i;
			mainOutputBusNumChannels = info.channelCount;
			outSpeakerArrs.push_back(arr);
			numEnabledOutAudioBuses++;
			return true;
		}
	}
	return false;
}

/** initialization : loading of the VST3 and of the various required interfaces */
bool VST3Plugin::init(const std::string &classId, const std::string &path_, int sample_rate, int max_blocksize,
		      SpeakerArrangement arr)
{
	std::string error;

	path = path_;

	sampleRate = sample_rate;
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

	// module creation
	module = VST3::Hosting::Module::create(path, error);
	if (!module) {
		infovst3plugin("%s", error.c_str());
		return false;
	}

	// enable parameter changes
	componentContext->guiToDsp = &guiToDsp;
	inChanges = std::make_unique<Steinberg::Vst::ParameterChanges>();
	outChanges = std::make_unique<Steinberg::Vst::ParameterChanges>();

	// get factory & set host context
	VST3::Hosting::PluginFactory factory = module->getFactory();
	factory.setHostContext(hostContext->getFUnknown());

	// use plugProvider to retrieve & setup component, processor & controller
	for (auto &classInfo : factory.classInfos()) {
		if (classInfo.category() == kVstAudioEffectClass && classInfo.ID().toString() == classId) {
			if (classId != classInfo.ID().toString())
				continue;
			plugProvider = owned(new OBSPlugProvider(factory, classInfo, false));
			if (plugProvider->setup(hostContext->getFUnknown()) == false)
				plugProvider = nullptr;
			name = classInfo.name();
			break;
		}
	}
	if (!plugProvider) {
		infovst3plugin("No VST3 Audio Module Class with UID %s found. You probably uninstalled the VST3.",
			       classId.c_str());
		return false;
	}

	vstPlug = plugProvider->getComponentPtr(); // IComponent*
	if (!vstPlug) {
		infovst3plugin("No VST3 Component class found.");
		return false;
	}

	editController = plugProvider->getControllerPtr(); // IEditController*
	if (!editController) {
		infovst3plugin("No VST3 EditorController class found.");
		return false;
	} else {
		editController->setComponentHandler(componentContext->getComponentHandler());
	}

	audioEffect = FUnknownPtr<IAudioProcessor>(vstPlug).getInterface();
	if (!audioEffect) {
		infovst3plugin("Failed to get an audio processor from VST3");
		// try to get audioProcessor from EditorController, à la Juce, from badly coded VST3.
		audioEffect = FUnknownPtr<IAudioProcessor>(editController).getInterface();
		if (!audioEffect)
			return false;
	}

	// Getting the audio buses; event buses (for midi) are not scanned.
	if (!scanAudioBuses(arr)) {
		obsVst3Struct->bypass.store(true, std::memory_order_relaxed);
		infovst3plugin("Error during the bus scan.");
		return false;
	}

	// Some plug-ins will crash if we pass a nullptr to setBusArrangements!
	SpeakerArrangement nullArrangement = {};
	auto *inData = inSpeakerArrs.empty() ? &nullArrangement : inSpeakerArrs.data();
	auto *outData = outSpeakerArrs.empty() ? &nullArrangement : outSpeakerArrs.data();
	tresult res = audioEffect->setBusArrangements(inData, numEnabledInAudioBuses, outData, numEnabledOutAudioBuses);
	if (res != kResultTrue) {
		// We check the speaker arrangements to detect what went wrong.
		SpeakerArrangement speakerArr;
		audioEffect->getBusArrangement(kInput, mainInputBusIndex, speakerArr);
		if (speakerArr != arr) {
			infovst3plugin("Failed to set input bus to obs speaker layout.");
			return false;
		}

		if (numEnabledInAudioBuses == 2) {
			audioEffect->getBusArrangement(kInput, auxBusIndex, speakerArr);
			SpeakerArrangement sideArr = sidechainNumChannels == 1 ? Steinberg::Vst::SpeakerArr::kMono
									       : Steinberg::Vst::SpeakerArr::kStereo;
			if (speakerArr != sideArr) {
				infovst3plugin("Failed to set side chain bus to desired speaker layout!");
				return false;
			}
		}

		audioEffect->getBusArrangement(kOutput, mainOutputBusIndex, speakerArr);
		if (speakerArr != arr) {
			infovst3plugin("Failed to set output bus to obs speaker layout.");
			return false;
		}
	}

	// End of setup stage, activation of VST3
	res = audioEffect->setupProcessing(processSetup);
	if (res == kResultOk) {
		processData.prepare(*vstPlug, maxBlockSize, processSetup.symbolicSampleSize);
		// silence outputs on preparation, safety move
		for (int32 busIdx = 0; busIdx < processData.numOutputs; ++busIdx) {
			auto &bus = processData.outputs[busIdx];

			if (bus.channelBuffers32) {
				for (int32 ch = 0; ch < bus.numChannels; ++ch)
					std::fill_n(bus.channelBuffers32[ch], maxBlockSize, 0.0f);
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
	// this often reports 0, which probably means that the VST3 authors didn't really measure the value, lol
	uint32 latency = audioEffect->getLatencySamples();
	infovst3plugin("Latency of the plugin is %i samples", latency);

	return true;
}

/** The preprocess and postprocess functions deal with the exchange of parameters between GUI Editor and AudioProcessor. */
void VST3Plugin::drainDspToGui()
{
	ParamEdit e = {};
	while (dspToGui.pop(e)) {
		if (editController)
			editController->setParamNormalized(e.id, e.value);
	}
	uiDrainScheduled.store(false, std::memory_order_release);
}

void VST3Plugin::preprocess()
{
	inChanges->clearQueue();
	outChanges->clearQueue();
	processData.inputParameterChanges = inChanges.get();
	processData.outputParameterChanges = outChanges.get();

	// Drain GUI->DSP
	if (processData.inputParameterChanges) {
		ParamEdit e = {};
		while (guiToDsp.pop(e)) {
			int32 idx = -1;
			if (auto *q = processData.inputParameterChanges->addParameterData(e.id, idx)) {
				int32 pt = 0;
				q->addPoint(0, e.value, pt);
			}
		}
	}
}

void VST3Plugin::postprocess()
{
	bool hadUpdate = false;
	if (auto *out = processData.outputParameterChanges) {
		for (int32 i = 0, n = out->getParameterCount(); i < n; ++i) {
			if (auto *q = out->getParameterData(i)) {
				const auto id = q->getParameterId();
				const int32 cnt = q->getPointCount();
				if (cnt > 0) {
					int32 off;
					Steinberg::Vst::ParamValue v;
					if (q->getPoint(cnt - 1, off, v) == Steinberg::kResultOk) {
						dspToGui.push({id, v});
						hadUpdate = true;
					}
				}
			}
		}
	}

	if (hadUpdate && !uiDrainScheduled.exchange(true, std::memory_order_acq_rel)) {
		QObject *receiver = QCoreApplication::instance();
		if (receiver) {
			QMetaObject::invokeMethod(receiver, [this] { this->drainDspToGui(); }, Qt::QueuedConnection);
		} else {
			uiDrainScheduled.store(false, std::memory_order_release);
		}
	}
}

void VST3Plugin::setProcessing(bool processing) const
{
	audioEffect->setProcessing(processing);
}

/** the main processing function & buffers */
bool VST3Plugin::process(int numSamples)
{
	if (!audioEffect)
		return false;

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

Steinberg::Vst::Sample32 *VST3Plugin::sidechannelBuffer32(const BusDirection direction, const int ch) const
{
	if (direction == kInput) {
		return processData.inputs[auxBusIndex].channelBuffers32[ch];
	} else {
		return nullptr;
	}
}

/** GUI functions w/ a hack ripped from Juce, to create the view even with badly coded VST3s... */
void VST3Plugin::tryCreatingView()
{
	view = editController->createView(Vst::ViewType::kEditor);

	if (view == nullptr)
		view = editController->createView(nullptr);

	if (view == nullptr)
		editController->queryInterface(IPlugView::iid, reinterpret_cast<void **>(&view));
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
		if (view)
			view->release();
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
	if (view->isPlatformTypeSupported(Steinberg::kPlatformTypeX11EmbedWindowID) != Steinberg::kResultTrue) {
		infovst3plugin("Editor view does not support X11");
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
	if (!window) {
		int width = 800, height = 600;
		if (view) {
			Steinberg::ViewRect rect;
			if (view->getSize(&rect) == Steinberg::kResultOk) {
				width = rect.getWidth();
				height = rect.getHeight();
			} else {
				infovst3plugin("Failed to get size before attaching an IFrame. Not SDK compliant.");
			}
		}
		std::string sourceName = obs_source_get_name(obsVst3Struct->context);
		std::string windowName = sourceName + ": VST3 Plugin - " + name;
#ifdef __linux__
		window = new VST3EditorWindow(view, windowName, display, runLoop);
#else
		window = new VST3EditorWindow(view, windowName);
#endif
		if (window->create(width, height)) {
			window->show();
		}
	} else {
		window->show();
	}
	editorVisible = true;
}

void VST3Plugin::hideEditor()
{
	if (window && view) {
		window->close();
	}
	editorVisible = false;
}

// This function is required because we don't really close the GUI window; we hide it on Windows & macOS.
// This then means we have to track when a GUI has been closed by the user when clicking X. I decided to just hide because
// creating the GUI each time the user wants to display it, was prone to crashes. This also simplified the coding.
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

/** Load/Save the VST3 settings */
bool VST3Plugin::saveStates(std::vector<uint8_t> &compOut, std::vector<uint8_t> &ctrlOut) const
{
	compOut.clear();
	ctrlOut.clear();

	if (!vstPlug)
		return false;

	// --- component save
	{
		Steinberg::MemoryStream s;
		if (vstPlug->getState(&s) != Steinberg::kResultOk)
			return false;

		Steinberg::int64 size = 0, seekRes = 0;
		s.tell(&size);
		if (size <= 0)
			return false;

		compOut.resize(static_cast<size_t>(size));
		s.seek(0, Steinberg::IBStream::kIBSeekSet, &seekRes);
		Steinberg::int32 actuallyRead = 0;
		s.read(compOut.data(), static_cast<Steinberg::int32>(size), &actuallyRead);
		if (actuallyRead < size)
			compOut.resize(static_cast<size_t>(actuallyRead));
	}

	// --- controller save
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
				if (actuallyRead < size)
					ctrlOut.resize(static_cast<size_t>(actuallyRead));
			}
		}
	}

	return true;
}

bool VST3Plugin::loadStates(const std::vector<uint8_t> &comp, const std::vector<uint8_t> &ctrl)
{
	if (!vstPlug || comp.empty())
		return false;

	Steinberg::MemoryStream compStream;
	Steinberg::int32 w = 0;
	Steinberg::int64 dummy = 0;

	compStream.write((void *)comp.data(), (Steinberg::int32)comp.size(), &w);
	compStream.seek(0, Steinberg::IBStream::kIBSeekSet, &dummy);
	if (vstPlug->setState(&compStream) != Steinberg::kResultOk)
		return false;

	if (editController) {
		compStream.seek(0, Steinberg::IBStream::kIBSeekSet, &dummy);
		(void)editController->setComponentState(&compStream);
		if (!ctrl.empty()) {
			Steinberg::MemoryStream ctrlStream;
			ctrlStream.write((void *)ctrl.data(), (Steinberg::int32)ctrl.size(), &w);
			ctrlStream.seek(0, Steinberg::IBStream::kIBSeekSet, &dummy);
			(void)editController->setState(&ctrlStream);
		}
	}
	return true;
}
