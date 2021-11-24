#include "aja-props.hpp"

#include <ajantv2/includes/ntv2devicefeatures.h>
#include <ajantv2/includes/ntv2utils.h>
#include <ajantv2/includes/ntv2vpid.h>

VPIDData::VPIDData()
	: mVpidA{0},
	  mVpidB{0},
	  mStandardA{VPIDStandard_Unknown},
	  mStandardB{VPIDStandard_Unknown},
	  mSamplingA{VPIDSampling_XYZ_444},
	  mSamplingB{VPIDSampling_XYZ_444}
{
}

VPIDData::VPIDData(ULWord vpidA, ULWord vpidB)
	: mVpidA{vpidA},
	  mVpidB{vpidB},
	  mStandardA{VPIDStandard_Unknown},
	  mStandardB{VPIDStandard_Unknown},
	  mSamplingA{VPIDSampling_XYZ_444},
	  mSamplingB{VPIDSampling_XYZ_444}
{
	Parse();
}

VPIDData::VPIDData(const VPIDData &other)
	: mVpidA{other.mVpidA},
	  mVpidB{other.mVpidB},
	  mStandardA{VPIDStandard_Unknown},
	  mStandardB{VPIDStandard_Unknown},
	  mSamplingA{VPIDSampling_XYZ_444},
	  mSamplingB{VPIDSampling_XYZ_444}
{
	Parse();
}
VPIDData::VPIDData(VPIDData &&other)
	: mVpidA{other.mVpidA},
	  mVpidB{other.mVpidB},
	  mStandardA{VPIDStandard_Unknown},
	  mStandardB{VPIDStandard_Unknown},
	  mSamplingA{VPIDSampling_XYZ_444},
	  mSamplingB{VPIDSampling_XYZ_444}
{
	Parse();
}

VPIDData &VPIDData::operator=(const VPIDData &other)
{
	mVpidA = other.mVpidA;
	mVpidB = other.mVpidB;
	return *this;
}

VPIDData &VPIDData::operator=(VPIDData &&other)
{
	mVpidA = other.mVpidA;
	mVpidB = other.mVpidB;
	return *this;
}

bool VPIDData::operator==(const VPIDData &rhs) const
{
	return (mVpidA == rhs.mVpidA && mVpidB == rhs.mVpidB);
}

bool VPIDData::operator!=(const VPIDData &rhs) const
{
	return !operator==(rhs);
}

void VPIDData::SetA(ULWord vpidA)
{
	mVpidA = vpidA;
}

void VPIDData::SetB(ULWord vpidB)
{
	mVpidB = vpidB;
}

void VPIDData::Parse()
{
	CNTV2VPID parserA;
	parserA.SetVPID(mVpidA);
	mStandardA = parserA.GetStandard();
	mSamplingA = parserA.GetSampling();

	CNTV2VPID parserB;
	parserB.SetVPID(mVpidB);
	mStandardB = parserB.GetStandard();
	mSamplingB = parserB.GetSampling();
}

bool VPIDData::IsRGB() const
{
	switch (mSamplingA) {
	default:
		break;
	case VPIDSampling_GBR_444:
	case VPIDSampling_GBRA_4444:
	case VPIDSampling_GBRD_4444:
		return true;
	}
	return false;
}

VPIDStandard VPIDData::Standard() const
{
	return mStandardA;
}

VPIDSampling VPIDData::Sampling() const
{
	return mSamplingA;
}

// AJASource Properties
SourceProps::SourceProps()
	: deviceID{DEVICE_ID_NOTFOUND},
	  ioSelect{IOSelection::Invalid},
	  inputSource{NTV2_INPUTSOURCE_INVALID},
	  videoFormat{NTV2_FORMAT_UNKNOWN},
	  pixelFormat{NTV2_FBF_INVALID},
	  sdi4kTransport{SDI4KTransport::TwoSampleInterleave},
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
	  inputSource{NTV2_INPUTSOURCE_INVALID},
	  videoFormat{NTV2_FORMAT_UNKNOWN},
	  pixelFormat{NTV2_FBF_INVALID},
	  sdi4kTransport{SDI4KTransport::TwoSampleInterleave},
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
	inputSource = props.inputSource;
	videoFormat = props.videoFormat;
	pixelFormat = props.pixelFormat;
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
	inputSource = props.inputSource;
	videoFormat = props.videoFormat;
	pixelFormat = props.pixelFormat;
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
	inputSource = props.inputSource;
	videoFormat = props.videoFormat;
	pixelFormat = props.pixelFormat;
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
	inputSource = props.inputSource;
	videoFormat = props.videoFormat;
	pixelFormat = props.pixelFormat;
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

NTV2Channel SourceProps::Channel() const
{
	return NTV2InputSourceToChannel(inputSource);
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
	  sdi4kTransport{SDI4KTransport::TwoSampleInterleave},
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

	if (NTV2_OUTPUT_DEST_IS_HDMI(outputDest))
		return static_cast<NTV2Channel>(
			NTV2DeviceGetNumFrameStores(deviceID) - 1);

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
