#pragma once

#include "aja-enums.hpp"

#include <media-io/audio-io.h>

#include <ajantv2/includes/ntv2enums.h>
#include <ajantv2/includes/ntv2formatdescriptor.h>

#include <map>
#include <string>
#include <vector>

class VPIDData {
public:
	VPIDData();
	VPIDData(ULWord vpidA, ULWord vpidB);
	VPIDData(const VPIDData &other);
	VPIDData(VPIDData &&other);
	~VPIDData() = default;

	VPIDData &operator=(const VPIDData &other);
	VPIDData &operator=(VPIDData &&other);
	bool operator==(const VPIDData &rhs) const;
	bool operator!=(const VPIDData &rhs) const;

	void SetA(ULWord vpidA);
	void SetB(ULWord vpidB);
	void Parse();
	bool IsRGB() const;

	VPIDStandard Standard() const;
	VPIDSampling Sampling() const;

private:
	ULWord mVpidA;
	ULWord mVpidB;
	VPIDStandard mStandardA;
	VPIDSampling mSamplingA;
	VPIDStandard mStandardB;
	VPIDSampling mSamplingB;
};

using VPIDDataList = std::vector<VPIDData>;

//TODO(paulh): Consolidate the two Props classes
class SourceProps {
public:
	explicit SourceProps();
	explicit SourceProps(NTV2DeviceID devID);
	~SourceProps() = default;
	SourceProps(const SourceProps &props);
	SourceProps(SourceProps &&props);
	void operator=(const SourceProps &props);
	void operator=(SourceProps &&props);
	bool operator==(const SourceProps &props);
	bool operator!=(const SourceProps &props);

	NTV2Channel Channel() const;
	NTV2AudioSystem AudioSystem() const;
	NTV2AudioRate AudioRate() const;
	size_t AudioSize() const;
	audio_format AudioFormat() const;
	speaker_layout SpeakerLayout() const;

	NTV2DeviceID deviceID;
	IOSelection ioSelect;
	NTV2InputSource inputSource;
	NTV2VideoFormat videoFormat;
	NTV2PixelFormat pixelFormat;
	SDI4KTransport sdi4kTransport;
	VPIDDataList vpids;
	uint32_t audioNumChannels;
	uint32_t audioSampleSize;
	uint32_t audioSampleRate;
	bool autoDetect;
	bool deactivateWhileNotShowing;
};

class OutputProps {
public:
	explicit OutputProps(NTV2DeviceID devID);
	~OutputProps() = default;
	OutputProps(const OutputProps &props);
	OutputProps(OutputProps &&props);
	void operator=(const OutputProps &props);
	void operator=(OutputProps &&props);
	bool operator==(const OutputProps &props);
	bool operator!=(const OutputProps &props);

	NTV2FormatDesc FormatDesc();
	NTV2Channel Channel() const;
	NTV2AudioSystem AudioSystem() const;
	NTV2AudioRate AudioRate() const;
	size_t AudioSize() const;
	audio_format AudioFormat() const;
	speaker_layout SpeakerLayout() const;

	NTV2DeviceID deviceID;
	IOSelection ioSelect;
	NTV2OutputDestination outputDest;
	NTV2VideoFormat videoFormat;
	NTV2PixelFormat pixelFormat;
	SDI4KTransport sdi4kTransport;
	uint32_t audioNumChannels;
	uint32_t audioSampleSize;
	uint32_t audioSampleRate;
};
