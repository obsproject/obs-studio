#pragma once

#include <ajantv2/includes/ntv2enums.h>
#include <ajantv2/includes/ntv2vpid.h>

/*
 * Wrapper class for handling SMPTE 352M Video Payload ID (VPID) data
 * Each SDI data stream contains two 32-bit VPID bitfields, representing
 * characteristics about the video streams on the SDI wire. AJA and other SDI
 * devices may use VPID to help determine what kind of video data is being
 * conveyed by the incoming/outgoing streams.
 */
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
	VPIDBitDepth BitDepth() const;

private:
	ULWord mVpidA;
	ULWord mVpidB;
	VPIDStandard mStandardA;
	VPIDSampling mSamplingA;
	VPIDStandard mStandardB;
	VPIDSampling mSamplingB;
	VPIDBitDepth mBitDepthA;
	VPIDBitDepth mBitDepthB;
};

using VPIDDataList = std::vector<VPIDData>;
