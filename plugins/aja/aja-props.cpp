#include "aja-props.hpp"
#include "aja-common.hpp"

#include <ajantv2/includes/ntv2devicefeatures.h>
#include <ajantv2/includes/ntv2utils.h>

// AJASource Properties
SourceProps::SourceProps()
	: deviceID{DEVICE_ID_NOTFOUND},
	  ioSelect{IOSelection::Invalid},
	  //   inputSource{NTV2_INPUTSOURCE_INVALID},
	  videoFormat{NTV2_FORMAT_UNKNOWN},
	  pixelFormat{NTV2_FBF_INVALID},
	  sdiTransport{SDITransport::SingleLink},
	  sdi4kTransport{SDITransport4K::TwoSampleInterleave},
	  audioNumChannels{8},
	  audioSampleSize{4},
	  audioSampleRate{48000},
	  vpids{},
	  autoDetect{false},
	  deactivateWhileNotShowing{false}
{
}

SourceProps::SourceProps(NTV2DeviceID devID)
	: deviceID{devID},
	  ioSelect{IOSelection::Invalid},
	  //   inputSource{NTV2_INPUTSOURCE_INVALID},
	  videoFormat{NTV2_FORMAT_UNKNOWN},
	  pixelFormat{NTV2_FBF_INVALID},
	  sdiTransport{SDITransport::SingleLink},
	  sdi4kTransport{SDITransport4K::TwoSampleInterleave},
	  audioNumChannels{8},
	  audioSampleSize{4},
	  audioSampleRate{48000},
	  vpids{},
	  autoDetect{false},
	  deactivateWhileNotShowing{false}
{
}

SourceProps::SourceProps(const SourceProps &props)
{
	deviceID = props.deviceID;
	ioSelect = props.ioSelect;
	// inputSource = props.inputSource;
	videoFormat = props.videoFormat;
	pixelFormat = props.pixelFormat;
	sdiTransport = props.sdiTransport;
	sdi4kTransport = props.sdi4kTransport;
	audioNumChannels = props.audioNumChannels;
	audioSampleSize = props.audioSampleSize;
	audioSampleRate = props.audioSampleRate;
	vpids = props.vpids;
	autoDetect = props.autoDetect;
	deactivateWhileNotShowing = props.deactivateWhileNotShowing;
}

SourceProps::SourceProps(SourceProps &&props)
{
	deviceID = props.deviceID;
	ioSelect = props.ioSelect;
	// inputSource = props.inputSource;
	videoFormat = props.videoFormat;
	pixelFormat = props.pixelFormat;
	sdiTransport = props.sdiTransport;
	sdi4kTransport = props.sdi4kTransport;
	audioNumChannels = props.audioNumChannels;
	audioSampleSize = props.audioSampleSize;
	audioSampleRate = props.audioSampleRate;
	vpids = props.vpids;
	autoDetect = props.autoDetect;
	deactivateWhileNotShowing = props.deactivateWhileNotShowing;
}

void SourceProps::operator=(const SourceProps &props)
{
	deviceID = props.deviceID;
	ioSelect = props.ioSelect;
	// inputSource = props.inputSource;
	videoFormat = props.videoFormat;
	pixelFormat = props.pixelFormat;
	sdiTransport = props.sdiTransport;
	sdi4kTransport = props.sdi4kTransport;
	audioNumChannels = props.audioNumChannels;
	audioSampleSize = props.audioSampleSize;
	audioSampleRate = props.audioSampleRate;
	vpids = props.vpids;
	autoDetect = props.autoDetect;
	deactivateWhileNotShowing = props.deactivateWhileNotShowing;
}

void SourceProps::operator=(SourceProps &&props)
{
	deviceID = props.deviceID;
	ioSelect = props.ioSelect;
	// inputSource = props.inputSource;
	videoFormat = props.videoFormat;
	pixelFormat = props.pixelFormat;
	sdiTransport = props.sdiTransport;
	sdi4kTransport = props.sdi4kTransport;
	audioNumChannels = props.audioNumChannels;
	audioSampleSize = props.audioSampleSize;
	audioSampleRate = props.audioSampleRate;
	vpids = props.vpids;
	autoDetect = props.autoDetect;
	deactivateWhileNotShowing = props.deactivateWhileNotShowing;
}

bool SourceProps::operator==(const SourceProps &props)
{
	return (deviceID == props.deviceID && ioSelect == props.ioSelect &&
		// inputSource == props.inputSource &&
		videoFormat == props.videoFormat &&
		pixelFormat == props.pixelFormat &&
		// vpid == props.vpid &&
		autoDetect == props.autoDetect &&
		sdiTransport == props.sdiTransport &&
		sdi4kTransport == props.sdi4kTransport &&
		audioNumChannels == props.audioNumChannels &&
		audioSampleSize == props.audioSampleSize &&
		audioSampleRate == props.audioSampleRate &&
		deactivateWhileNotShowing == props.deactivateWhileNotShowing);
}

bool SourceProps::operator!=(const SourceProps &props)
{
	return !operator==(props);
}

NTV2InputSource SourceProps::InitialInputSource() const
{
	auto inputSources = InputSources();
	if (!inputSources.empty()) {
		return *inputSources.begin();
	}
	return NTV2_INPUTSOURCE_INVALID;
}

NTV2InputSourceSet SourceProps::InputSources() const
{
	NTV2InputSourceSet inputSources;
	aja::IOSelectionToInputSources(ioSelect, inputSources);
	return inputSources;
}

NTV2Channel SourceProps::Channel() const
{
	return NTV2InputSourceToChannel(InitialInputSource());
}

NTV2Channel SourceProps::Framestore() const
{
	if (deviceID == DEVICE_ID_KONAHDMI && ioSelect == IOSelection::HDMI2 &&
	    NTV2_IS_4K_VIDEO_FORMAT(videoFormat)) {
		return NTV2_CHANNEL3;
	}
	return Channel();
}

NTV2AudioSystem SourceProps::AudioSystem() const
{
	return NTV2ChannelToAudioSystem(Channel());
}

NTV2AudioRate SourceProps::AudioRate() const
{
	NTV2AudioRate rate = NTV2_AUDIO_48K;
	switch (audioSampleRate) {
	default:
	case 48000:
		rate = NTV2_AUDIO_48K;
		break;
	case 96000:
		rate = NTV2_AUDIO_96K;
		break;
	case 192000:
		rate = NTV2_AUDIO_192K;
		break;
	}

	return rate;
}

// Size in bytes of N channels of audio
size_t SourceProps::AudioSize() const
{
	return audioNumChannels * audioSampleSize;
}

audio_format SourceProps::AudioFormat() const
{
	// NTV2 is always 32-bit PCM
	return AUDIO_FORMAT_32BIT;
}

speaker_layout SourceProps::SpeakerLayout() const
{
	if (audioNumChannels == 2)
		return SPEAKERS_STEREO;
	// NTV2 is always at least 8ch on modern boards
	return SPEAKERS_7POINT1;
}

//
// AJAOutput Properties
//
OutputProps::OutputProps(NTV2DeviceID devID)
	: deviceID{devID},
	  ioSelect{IOSelection::Invalid},
	  outputDest{NTV2_OUTPUTDESTINATION_ANALOG},
	  videoFormat{NTV2_FORMAT_UNKNOWN},
	  pixelFormat{NTV2_FBF_INVALID},
	  sdi4kTransport{SDITransport4K::TwoSampleInterleave},
	  audioNumChannels{8},
	  audioSampleSize{4},
	  audioSampleRate{48000}
{
}

OutputProps::OutputProps(OutputProps &&props)
{
	deviceID = props.deviceID;
	ioSelect = props.ioSelect;
	outputDest = props.outputDest;
	videoFormat = props.videoFormat;
	pixelFormat = props.pixelFormat;
	sdiTransport = props.sdiTransport;
	sdi4kTransport = props.sdi4kTransport;
	audioNumChannels = props.audioNumChannels;
	audioSampleSize = props.audioSampleSize;
	audioSampleRate = props.audioSampleRate;
}

OutputProps::OutputProps(const OutputProps &props)
{
	deviceID = props.deviceID;
	ioSelect = props.ioSelect;
	outputDest = props.outputDest;
	videoFormat = props.videoFormat;
	pixelFormat = props.pixelFormat;
	sdiTransport = props.sdiTransport;
	sdi4kTransport = props.sdi4kTransport;
	audioNumChannels = props.audioNumChannels;
	audioSampleSize = props.audioSampleSize;
	audioSampleRate = props.audioSampleRate;
}

void OutputProps::operator=(const OutputProps &props)
{
	deviceID = props.deviceID;
	ioSelect = props.ioSelect;
	outputDest = props.outputDest;
	videoFormat = props.videoFormat;
	pixelFormat = props.pixelFormat;
	sdiTransport = props.sdiTransport;
	sdi4kTransport = props.sdi4kTransport;
	audioNumChannels = props.audioNumChannels;
	audioSampleSize = props.audioSampleSize;
	audioSampleRate = props.audioSampleRate;
}

void OutputProps::operator=(OutputProps &&props)
{
	deviceID = props.deviceID;
	ioSelect = props.ioSelect;
	outputDest = props.outputDest;
	videoFormat = props.videoFormat;
	pixelFormat = props.pixelFormat;
	sdiTransport = props.sdiTransport;
	sdi4kTransport = props.sdi4kTransport;
	audioNumChannels = props.audioNumChannels;
	audioSampleSize = props.audioSampleSize;
	audioSampleRate = props.audioSampleRate;
}

bool OutputProps::operator==(const OutputProps &props)
{
	return (deviceID == props.deviceID && ioSelect == props.ioSelect &&
		// outputDest == props.outputDest &&
		videoFormat == props.videoFormat &&
		pixelFormat == props.pixelFormat &&
		sdiTransport == props.sdiTransport &&
		sdi4kTransport == props.sdi4kTransport &&
		audioNumChannels == props.audioNumChannels &&
		audioSampleSize == props.audioSampleSize &&
		audioSampleRate == props.audioSampleRate);
}

bool OutputProps::operator!=(const OutputProps &props)
{
	return !operator==(props);
}

NTV2FormatDesc OutputProps::FormatDesc()
{
	return NTV2FormatDesc(videoFormat, pixelFormat, NTV2_VANCMODE_OFF);
}

NTV2Channel OutputProps::Channel() const
{
	// Output Channel Special Cases
	// KONA1 -- Has 2 framestores but only 1 bi-directional SDI widget
	if (deviceID == DEVICE_ID_KONA1) {
		return NTV2_CHANNEL2;
	} else if ((deviceID == DEVICE_ID_IO4K ||
		    deviceID == DEVICE_ID_IO4KPLUS) &&
		   outputDest == NTV2_OUTPUTDESTINATION_SDI5) {
		// IO4K/IO4K+ SDI Monitor - Use framestore 4 but SDI5
		return NTV2_CHANNEL4;
	}

	if (NTV2_OUTPUT_DEST_IS_HDMI(outputDest)) {
		if (aja::CardCanDoHDMIMonitorOutput(deviceID) &&
		    NTV2_IS_4K_VIDEO_FORMAT(videoFormat))
			return NTV2_CHANNEL3;
		return static_cast<NTV2Channel>(
			NTV2DeviceGetNumFrameStores(deviceID) - 1);
	}

	return NTV2OutputDestinationToChannel(outputDest);
}

NTV2Channel OutputProps::Framestore() const
{
	if (deviceID == DEVICE_ID_TTAP_PRO) {
		return NTV2_CHANNEL1;
	} else if (deviceID == DEVICE_ID_KONA1) {
		return NTV2_CHANNEL2;
	} else if (deviceID == DEVICE_ID_IO4K ||
		   deviceID == DEVICE_ID_IO4KPLUS) {
		// SDI Monitor output uses framestore 4
		if (ioSelect == IOSelection::SDI5)
			return NTV2_CHANNEL4;
	}
	// HDMI Monitor output uses framestore 4
	if (ioSelect == IOSelection::HDMIMonitorOut) {
		if (deviceID == DEVICE_ID_KONA5_8K)
			return NTV2_CHANNEL4;
		if (NTV2_IS_4K_VIDEO_FORMAT(videoFormat))
			return NTV2_CHANNEL3;
		else
			return NTV2_CHANNEL4;
	}
	return NTV2OutputDestinationToChannel(outputDest);
}

NTV2AudioSystem OutputProps::AudioSystem() const
{
	return NTV2ChannelToAudioSystem(Channel());
}

NTV2AudioRate OutputProps::AudioRate() const
{
	NTV2AudioRate rate = NTV2_AUDIO_48K;
	switch (audioSampleRate) {
	default:
	case 48000:
		rate = NTV2_AUDIO_48K;
		break;
	case 96000:
		rate = NTV2_AUDIO_96K;
		break;
	case 192000:
		rate = NTV2_AUDIO_192K;
		break;
	}

	return rate;
}

// Size in bytes of N channels of audio
size_t OutputProps::AudioSize() const
{
	return audioNumChannels * audioSampleSize;
}

audio_format OutputProps::AudioFormat() const
{
	// NTV2 is always 32-bit PCM
	return AUDIO_FORMAT_32BIT;
}

speaker_layout OutputProps::SpeakerLayout() const
{
	if (audioNumChannels == 2)
		return SPEAKERS_STEREO;
	// NTV2 is always at least 8ch on modern boards
	return SPEAKERS_7POINT1;
}
