//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstaudioprocessor.h
// Created by  : Steinberg, 10/2005
// Description : VST Audio Processing Interfaces
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "ivstcomponent.h"
#include "vstspeaker.h"

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

//------------------------------------------------------------------------
/** Class Category Name for Audio Processor Component */
//------------------------------------------------------------------------
#ifndef kVstAudioEffectClass
#define kVstAudioEffectClass "Audio Module Class"
#endif

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

class IEventList;
class IParameterChanges;
struct ProcessContext;

//------------------------------------------------------------------------
/** Component Types used as subCategories in PClassInfo2 */
namespace PlugType
{
/**
 * \defgroup plugType Plug-in Type used for subCategories
 * @{ */
SMTG_CONSTEXPR const CString kFx					= "Fx";				///< others type (not categorized)
SMTG_CONSTEXPR const CString kFxAnalyzer			= "Fx|Analyzer";	///< Scope, FFT-Display, Loudness Processing...
SMTG_CONSTEXPR const CString kFxBass				= "Fx|Bass";		///< Tools dedicated to Bass Guitar
SMTG_CONSTEXPR const CString kFxChannelStrip		= "Fx|Channel Strip"; ///< Tools dedicated to Channel Strip
SMTG_CONSTEXPR const CString kFxDelay				= "Fx|Delay";		///< Delay, Multi-tap Delay, Ping-Pong Delay...
SMTG_CONSTEXPR const CString kFxDistortion			= "Fx|Distortion";	///< Amp Simulator, Sub-Harmonic, SoftClipper...
SMTG_CONSTEXPR const CString kFxDrums				= "Fx|Drums";		///< Tools dedicated to Drums...
SMTG_CONSTEXPR const CString kFxDynamics			= "Fx|Dynamics";	///< Compressor, Expander, Gate, Limiter, Maximizer, Tape Simulator, EnvelopeShaper...
SMTG_CONSTEXPR const CString kFxEQ					= "Fx|EQ";			///< Equalization, Graphical EQ...
SMTG_CONSTEXPR const CString kFxFilter				= "Fx|Filter";		///< WahWah, ToneBooster, Specific Filter,...
SMTG_CONSTEXPR const CString kFxGenerator			= "Fx|Generator";	///< Tone Generator, Noise Generator...
SMTG_CONSTEXPR const CString kFxGuitar				= "Fx|Guitar";		///< Tools dedicated to Guitar
SMTG_CONSTEXPR const CString kFxInstrument			= "Fx|Instrument";	///< Fx which could be loaded as Instrument too
SMTG_CONSTEXPR const CString kFxInstrumentExternal	= "Fx|Instrument|External";	///< Fx which could be loaded as Instrument too and is external (wrapped Hardware)
SMTG_CONSTEXPR const CString kFxMastering			= "Fx|Mastering";	///< Dither, Noise Shaping,...
SMTG_CONSTEXPR const CString kFxMicrophone			= "Fx|Microphone";	///< Tools dedicated to Microphone
SMTG_CONSTEXPR const CString kFxModulation			= "Fx|Modulation";	///< Phaser, Flanger, Chorus, Tremolo, Vibrato, AutoPan, Rotary, Cloner...
SMTG_CONSTEXPR const CString kFxNetwork				= "Fx|Network";		///< using Network
SMTG_CONSTEXPR const CString kFxPitchShift			= "Fx|Pitch Shift";	///< Pitch Processing, Pitch Correction, Vocal Tuning...
SMTG_CONSTEXPR const CString kFxRestoration			= "Fx|Restoration";	///< Denoiser, Declicker,...
SMTG_CONSTEXPR const CString kFxReverb				= "Fx|Reverb";		///< Reverberation, Room Simulation, Convolution Reverb...
SMTG_CONSTEXPR const CString kFxSpatial				= "Fx|Spatial";		///< MonoToStereo, StereoEnhancer,...
SMTG_CONSTEXPR const CString kFxSurround			= "Fx|Surround";	///< dedicated to surround processing: LFE Splitter, Bass Manager...
SMTG_CONSTEXPR const CString kFxTools				= "Fx|Tools";		///< Volume, Mixer, Tuner...
SMTG_CONSTEXPR const CString kFxVocals				= "Fx|Vocals";		///< Tools dedicated to Vocals

SMTG_CONSTEXPR const CString kInstrument			= "Instrument";			///< Effect used as instrument (sound generator), not as insert
SMTG_CONSTEXPR const CString kInstrumentDrum		= "Instrument|Drum";	///< Instrument for Drum sounds
SMTG_CONSTEXPR const CString kInstrumentExternal	= "Instrument|External";///< External Instrument (wrapped Hardware)
SMTG_CONSTEXPR const CString kInstrumentPiano		= "Instrument|Piano";	///< Instrument for Piano sounds
SMTG_CONSTEXPR const CString kInstrumentSampler		= "Instrument|Sampler";	///< Instrument based on Samples
SMTG_CONSTEXPR const CString kInstrumentSynth		= "Instrument|Synth";	///< Instrument based on Synthesis
SMTG_CONSTEXPR const CString kInstrumentSynthSampler = "Instrument|Synth|Sampler";	///< Instrument based on Synthesis and Samples

SMTG_CONSTEXPR const CString kAmbisonics			= "Ambisonics";		///< used for Ambisonics channel (FX or Panner/Mixconverter/Up-Mixer/Down-Mixer when combined with other category)
SMTG_CONSTEXPR const CString kAnalyzer			    = "Analyzer";	    ///< Meter, Scope, FFT-Display, not selectable as insert plug-in
SMTG_CONSTEXPR const CString kNoOfflineProcess		= "NoOfflineProcess";	///< will be NOT used for plug-in offline processing (will work as normal insert plug-in)
SMTG_CONSTEXPR const CString kOnlyARA				= "OnlyARA";		///< used for plug-ins that require ARA to operate (will not work as normal insert plug-in)
SMTG_CONSTEXPR const CString kOnlyOfflineProcess	= "OnlyOfflineProcess";	///< used for plug-in offline processing  (will not work as normal insert plug-in)
SMTG_CONSTEXPR const CString kOnlyRealTime			= "OnlyRT";			///< indicates that it supports only realtime process call, no processing faster than realtime
SMTG_CONSTEXPR const CString kSpatial				= "Spatial";		///< used for SurroundPanner
SMTG_CONSTEXPR const CString kSpatialFx				= "Spatial|Fx";		///< used for SurroundPanner and as insert effect
SMTG_CONSTEXPR const CString kUpDownMix				= "Up-Downmix";		///< used for Mixconverter/Up-Mixer/Down-Mixer

SMTG_CONSTEXPR const CString kMono					= "Mono";			///< used for Mono only plug-in [optional]
SMTG_CONSTEXPR const CString kStereo				= "Stereo";			///< used for Stereo only plug-in [optional]
SMTG_CONSTEXPR const CString kSurround				= "Surround";		///< used for Surround only plug-in [optional]

/**@}*/
}

//------------------------------------------------------------------------
/** Component Flags used as classFlags in PClassInfo2 */
enum ComponentFlags
{
//------------------------------------------------------------------------
	kDistributable			= 1 << 0,	///< Component can be run on remote computer
	kSimpleModeSupported	= 1 << 1	///< Component supports simple IO mode (or works in simple mode anyway) see \ref vst3IoMode
//------------------------------------------------------------------------
};

//------------------------------------------------------------------------
/** Symbolic sample size.
 * \see ProcessSetup, ProcessData
 */
enum SymbolicSampleSizes
{
	kSample32,		///< 32-bit precision
	kSample64		///< 64-bit precision
};

//------------------------------------------------------------------------
/** Processing mode informs the plug-in about the context and at which frequency the process call is
 * called.
 * .
 * VST3 defines 3 modes:
 *
 *    - kRealtime: each process call is called at a realtime frequency (defined by [numSamples of
 * ProcessData] / samplerate). The plug-in should always try to process as fast as possible in order
 * to let enough time slice to other plug-ins.
 *
 *    - kPrefetch: each process call could be called at a variable frequency (jitter, slower /
 * faster than realtime), the plug-in should process at the same quality level than realtime,
 * plug-in must not slow down to realtime (e.g. disk streaming)! The host should avoid to process in
 * kPrefetch mode such sampler based plug-in.
 *
 *    - kOffline:  each process call could be faster than realtime or slower, higher quality than
 * realtime could be used. plug-ins using disk streaming should be sure that they have enough time
 * in the process call for streaming, if needed by slowing down to realtime or slower.
 * .
 * Note about Process Modes switching:
 *    - Switching between kRealtime and kPrefetch process modes are done in realtime thread without
 * need of calling IAudioProcessor::setupProcessing, the plug-in should check in process call the
 * member processMode of ProcessData in order to know in which mode it is processed.
 *    - Switching between kRealtime (or kPrefetch) and kOffline requires that the host calls
 * IAudioProcessor::setupProcessing in order to inform the plug-in about this mode change.
 * .
 * \see ProcessSetup, ProcessData
 */
enum ProcessModes
{
	kRealtime,		///< realtime processing
	kPrefetch,		///< prefetch processing
	kOffline		///< offline processing
};

//------------------------------------------------------------------------
/** kNoTail
 *
 * to be returned by getTailSamples when no tail is wanted
 * \see IAudioProcessor::getTailSamples
 */
static const uint32 kNoTail = 0;

//------------------------------------------------------------------------
/** kInfiniteTail
 *
 * to be returned by getTailSamples when infinite tail is wanted
 * \see IAudioProcessor::getTailSamples
 */
static const uint32 kInfiniteTail = kMaxInt32u;

//------------------------------------------------------------------------
/** Audio processing setup.
 * \see IAudioProcessor::setupProcessing
 */
struct ProcessSetup
{
//------------------------------------------------------------------------
	int32 processMode;			///< \ref ProcessModes
	int32 symbolicSampleSize;	///< \ref SymbolicSampleSizes
	int32 maxSamplesPerBlock;	///< maximum number of samples per audio block
	SampleRate sampleRate;		///< sample rate
//------------------------------------------------------------------------
};

//------------------------------------------------------------------------
/** Processing buffers of an audio bus.
This structure contains the processing buffer for each channel of an audio bus.

- The number of channels (numChannels) must always match the current bus arrangement. It could be
set to value '0' when the host wants to flush the parameters (when the plug-in is not processed).

- The size of the channel buffer array must always match the number of channels. So the host must
always supply an array for the channel buffers, regardless if the bus is active or not. However, if
an audio bus is currently inactive, the actual sample buffer addresses are safe to be null.

- The silence flag is set when every sample of the according buffer has the value '0'. It is
intended to be used as help for optimizations allowing a plug-in to reduce processing activities.
But even if this flag is set for a channel, the channel buffers must still point to valid memory!
This flag is optional. A host is free to support it or not.
.
\see ProcessData
*/
struct AudioBusBuffers
{
	AudioBusBuffers () : numChannels (0), silenceFlags (0), channelBuffers64 (nullptr) {}

//------------------------------------------------------------------------
	int32 numChannels;		///< number of audio channels in bus
	uint64 silenceFlags;	///< Bitset of silence state per channel
	union
	{
		Sample32** channelBuffers32;	///< sample buffers to process with 32-bit precision
		Sample64** channelBuffers64;	///< sample buffers to process with 64-bit precision
	};
//------------------------------------------------------------------------
};

//------------------------------------------------------------------------
/** Any data needed in audio processing.
 * The host prepares AudioBusBuffers for each input/output bus,
 * regardless of the bus activation state. Bus buffer indices always match
 * with bus indices used in IComponent::getBusInfo of media type kAudio.
 * \see AudioBusBuffers, IParameterChanges, IEventList, ProcessContext, IProcessContextRequirements
 */
struct ProcessData
{
	ProcessData ()
	: processMode (0)
	, symbolicSampleSize (kSample32)
	, numSamples (0)
	, numInputs (0)
	, numOutputs (0)
	, inputs (nullptr)
	, outputs (nullptr)
	, inputParameterChanges (nullptr)
	, outputParameterChanges (nullptr)
	, inputEvents (nullptr)
	, outputEvents (nullptr)
	, processContext (nullptr)
	{
	}

//------------------------------------------------------------------------
	int32 processMode;			///< processing mode - value of \ref ProcessModes
	int32 symbolicSampleSize;   ///< sample size - value of \ref SymbolicSampleSizes
	int32 numSamples;			///< number of samples to process
	int32 numInputs;			///< number of audio input busses
	int32 numOutputs;			///< number of audio output busses
	AudioBusBuffers* inputs;	///< buffers of input busses
	AudioBusBuffers* outputs;	///< buffers of output busses

	IParameterChanges* inputParameterChanges;	///< incoming parameter changes for this block
	IParameterChanges* outputParameterChanges;	///< outgoing parameter changes for this block (optional)
	IEventList* inputEvents;				///< incoming events for this block (optional)
	IEventList* outputEvents;				///< outgoing events for this block (optional)
	ProcessContext* processContext;			///< processing context (optional, but most welcome)
//------------------------------------------------------------------------
};

//------------------------------------------------------------------------
/** Audio processing interface: Vst::IAudioProcessor
\ingroup vstIPlug vst300
- [plug imp]
- [extends IComponent]
- [released: 3.0.0]
- [mandatory]

This interface must always be supported by audio processing plug-ins.
*/
class IAudioProcessor : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Try to set (host => plug-in) a wanted arrangement for inputs and outputs.
	 * The host should always deliver the same number of input and output busses than the plug-in
	 * needs (see \ref IComponent::getBusCount). The plug-in has 3 possibilities to react on this
	 * setBusArrangements call:\n
	 *
	 * 1. The plug-in accepts these arrangements, then it should modify, if needed, its busses to
	 * match these new arrangements (later on asked by the host with IComponent::getBusInfo () or
	 * IAudioProcessor::getBusArrangement ()) and then should return kResultTrue.\n
	 *
	 * 2. The plug-in does not accept or support these requested arrangements for all
	 * inputs/outputs or just for some or only one bus, but the plug-in can try to adapt its
	 * current arrangements according to the requested ones (requested arrangements for kMain busses
	 * should be handled with more priority than the ones for kAux busses), then it should modify
	 * its busses arrangements and should return kResultFalse.\n
	 *
	 * 3. Same than the point 2 above the plug-in does not support these requested arrangements but
	 * the plug-in cannot find corresponding arrangements, the plug-in could keep its current
	 * arrangement or fall back to a default arrangement by modifying its busses arrangements and
	 * should return kResultFalse.\n
	 *
	 * \param inputs pointer to an array of \ref SpeakerArrangement
	 * \param numIns number of \ref SpeakerArrangement in inputs array
	 * \param outputs pointer to an array of \ref SpeakerArrangement
	 * \param numOuts number of \ref SpeakerArrangement in outputs array
	 * Returns kResultTrue when Arrangements is supported and is the current one, else returns
	 * kResultFalse.
	 * 
	 * \note [UI-thread & (Initialized | Connected | Setup Done)] */
	virtual tresult PLUGIN_API setBusArrangements (SpeakerArrangement* inputs /*in*/,
	                                               int32 numIns /*in*/,
	                                               SpeakerArrangement* outputs /*in*/,
	                                               int32 numOuts /*in*/) = 0;

	/** Gets the bus arrangement for a given direction (input/output) and index.
	 * Note: IComponent::getBusInfo () and IAudioProcessor::getBusArrangement () should be always
	 * return the same information about the busses arrangements.
	 * \note [UI-thread & (Initialized | Connected | Setup Done | Activated | Processing] */
	virtual tresult PLUGIN_API getBusArrangement (BusDirection dir /*in*/, int32 index /*in*/,
	                                              SpeakerArrangement& arr /*inout*/) = 0;

	/** Asks if a given sample size is supported see \ref SymbolicSampleSizes.
	 * \note [UI-thread & (Initialized | Connected)] */
	virtual tresult PLUGIN_API canProcessSampleSize (int32 symbolicSampleSize /*in*/) = 0;

	/** Gets the current Latency in samples.
	 * The returned value defines the group delay or the latency of the plug-in. For example, if
	 * the plug-in internally needs to look in advance (like compressors) 512 samples then this
	 * plug-in should report 512 as latency. If during the use of the plug-in this latency change,
	 * the plug-in has to inform the host by using IComponentHandler::restartComponent
	 * (kLatencyChanged), this could lead to audio playback interruption because the host has to
	 * recompute its internal mixer delay compensation. Note that for player live recording this
	 * latency should be zero or small.
	 * \note [UI-thread & Setup Done] */
	virtual uint32 PLUGIN_API getLatencySamples () = 0;

	/** Called in disable state (setActive not called with true) before setProcessing is called and
	 * processing will begin.
	 * \note [UI-thread & (Initialized | Connected)]] */
	virtual tresult PLUGIN_API setupProcessing (ProcessSetup& setup /*in*/) = 0;

	/** Informs the plug-in about the processing state. This will be called before any process calls
	 * start with true and after with false.
	 * Note that setProcessing (false) may be called after setProcessing (true) without any process
	 * calls.
	 * Note this function could be called in the UI or in Processing Thread, thats why the plug-in
	 * should only light operation (no memory allocation or big setup reconfiguration),
	 * this could be used to reset some buffers (like Delay line or Reverb).
	 * The host has to be sure that it is called only when the plug-in is enable (setActive (true)
	 * was called).
	 * \note [(UI-thread or processing-thread) & Activated] */
	virtual tresult PLUGIN_API setProcessing (TBool state /*in*/) = 0;

	/** The Process call, where all information (parameter changes, event, audio buffer) are passed.
	 * \note [processing-thread & Processing] */
	virtual tresult PLUGIN_API process (ProcessData& data /*in*/) = 0;

	/** Gets tail size in samples. For example, if the plug-in is a Reverb plug-in and it knows that
	 * the maximum length of the Reverb is 2sec, then it has to return in getTailSamples ()
	 * (in VST2 it was getGetTailSize ()): 2*sampleRate.
	 * This information could be used by host for offline processing, process optimization and
	 * downmix (avoiding signal cut (clicks)).
	 * It should return:
	 *  - kNoTail when no tail
	 *  - x * sampleRate when x Sec tail.
	 *  - kInfiniteTail when infinite tail.
	 * 
	 * \note [UI-thread & Setup Done] */
	virtual uint32 PLUGIN_API getTailSamples () = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IAudioProcessor, 0x42043F99, 0xB7DA453C, 0xA569E79D, 0x9AAEC33D)

//------------------------------------------------------------------------
/** Extended IAudioProcessor interface for a component: Vst::IAudioPresentationLatency
\ingroup vstIPlug vst310
- [plug imp]
- [extends IAudioProcessor]
- [released: 3.1.0]
- [optional]

Inform the plug-in about how long from the moment of generation/acquiring (from file or from Input)
it will take for its input to arrive, and how long it will take for its output to be presented (to
output or to speaker).

Note for Input Presentation Latency: when reading from file, the first plug-in will have an input
presentation latency set to zero. When monitoring audio input from an audio device, the initial
input latency is the input latency of the audio device itself.

Note for Output Presentation Latency: when writing to a file, the last plug-in will have an output
presentation latency set to zero. When the output of this plug-in is connected to an audio device,
the initial output latency is the output latency of the audio device itself.

A value of zero either means no latency or an unknown latency.

Each plug-in adding a latency (returning a none zero value for IAudioProcessor::getLatencySamples)
will modify the input presentation latency of the next plug-ins in the mixer routing graph and will
modify the output presentation latency of the previous plug-ins.

\n
\image html "iaudiopresentationlatency_usage.png"
\n
\see IAudioProcessor
\see IComponent
*/
class IAudioPresentationLatency : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Informs the plug-in about the Audio Presentation Latency in samples for a given direction
	 * (kInput/kOutput) and bus index.
	 * \note [UI-thread & Activated] */
	virtual tresult PLUGIN_API setAudioPresentationLatencySamples (
	    BusDirection dir /*in*/, int32 busIndex /*in*/, uint32 latencyInSamples /*in*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IAudioPresentationLatency, 0x309ECE78, 0xEB7D4fae, 0x8B2225D9, 0x09FD08B6)

//------------------------------------------------------------------------
/** Extended IAudioProcessor interface for a component: Vst::IProcessContextRequirements
 \ingroup vstIPlug vst370
 - [plug imp]
 - [extends IAudioProcessor]
 - [released: 3.7.0]
 - [mandatory]

To get accurate process context information (Vst::ProcessContext), it is now required to implement
this interface and return the desired bit mask of flags which your audio effect needs. If you do not
implement this interface, you may not get any information at all of the process function!

The host asks for this information once between initialize and setActive (in Setup Done). It cannot
be changed afterwards.

This gives the host the opportunity to better optimize the audio process graph when it knows which
plug-ins need which information.

Plug-ins built with an earlier SDK version (< 3.7) will still get the old information, but the
information may not be as accurate as when using this interface.
*/
class IProcessContextRequirements : public FUnknown
{
public:
	enum Flags
	{
		kNeedSystemTime				= 1 <<  0, // kSystemTimeValid
		kNeedContinousTimeSamples	= 1 <<  1, // kContTimeValid
		kNeedProjectTimeMusic		= 1 <<  2, // kProjectTimeMusicValid
		kNeedBarPositionMusic		= 1 <<  3, // kBarPositionValid
		kNeedCycleMusic				= 1 <<  4, // kCycleValid
		kNeedSamplesToNextClock		= 1 <<  5, // kClockValid
		kNeedTempo					= 1 <<  6, // kTempoValid
		kNeedTimeSignature			= 1 <<  7, // kTimeSigValid
		kNeedChord					= 1 <<  8, // kChordValid
		kNeedFrameRate				= 1 <<  9, // kSmpteValid
		kNeedTransportState			= 1 << 10, // kPlaying, kCycleActive, kRecording
	};
	/** Allows the host to ask the plug-in what is really needed for the process context information
	 * (Vst::ProcessContext).
	 * \note [UI-thread & Setup Done] */
	virtual uint32 PLUGIN_API getProcessContextRequirements () = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IProcessContextRequirements, 0x2A654303, 0xEF764E3D, 0x95B5FE83, 0x730EF6D0)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
