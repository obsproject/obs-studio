#pragma once

#include "aja-enums.hpp"
#include "aja-vpid-data.hpp"

#include <media-io/audio-io.h>

#include <ajantv2/includes/ntv2enums.h>
#include <ajantv2/includes/ntv2formatdescriptor.h>

#include <map>
#include <string>
#include <vector>

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

	NTV2InputSource InitialInputSource() const;
	NTV2InputSourceSet InputSources() const;
	NTV2Channel Channel() const;
	NTV2Channel Framestore() const;
	NTV2AudioSystem AudioSystem() const;
	NTV2AudioRate AudioRate() const;
	size_t AudioSize() const;
	audio_format AudioFormat() const;
	speaker_layout SpeakerLayout() const;

	NTV2DeviceID deviceID;
	IOSelection ioSelect;
	NTV2VideoFormat videoFormat;
	NTV2PixelFormat pixelFormat;
	SDITransport sdiTransport;
	SDITransport4K sdi4kTransport;
	VPIDDataList vpids;
	uint32_t audioNumChannels;
	uint32_t audioSampleSize;
	uint32_t audioSampleRate;
	bool autoDetect;
	bool deactivateWhileNotShowing;
	bool swapFrontCenterLFE;
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
	NTV2Channel Framestore() const;
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
	SDITransport sdiTransport;
	SDITransport4K sdi4kTransport;
	uint32_t audioNumChannels;
	uint32_t audioSampleSize;
	uint32_t audioSampleRate;
};
