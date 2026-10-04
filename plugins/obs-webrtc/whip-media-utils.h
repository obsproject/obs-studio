#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

// Convert the absolute OBS media clock, not separately rounded frame durations.
// Split seconds from the fraction to avoid overflow on long-running sessions.
// Unsigned arithmetic preserves RTP's 32-bit wrap, including negative preroll.
inline uint32_t whip_rtp_timestamp(int64_t usec, uint32_t clock_rate, uint32_t offset)
{
	const int64_t fraction = (usec % 1000000) * int64_t(clock_rate);
	const int64_t rounded = (fraction + (fraction < 0 ? -500000 : 500000)) / 1000000;
	return offset + uint32_t(usec / 1000000) * clock_rate + uint32_t(rounded);
}

inline int64_t whip_keyframe_interval(int64_t seconds)
{
	return seconds <= 0 || seconds > 2 ? 2 : seconds;
}

// This is a packet-pacing allowance, not a change to the encoder's bitrate.
inline double whip_pacing_bitrate(std::string mode, int64_t bitrate_kbps, int64_t maximum_kbps, bool limit_bitrate)
{
	std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c) { return std::toupper(c); });
	const bool quality = mode == "ICQ" || mode == "LA_ICQ" || mode == "CQP" || mode == "CRF" || mode == "CQ" ||
			     mode == "LOSSLESS";
	const bool capped_quality = mode == "CQVBR" || mode == "VBR_CQ";
	if (quality || capped_quality) {
		// Quality modes have no target bitrate; the saved bitrate field is inactive.
		// VideoToolbox can explicitly cap CRF; NVENC CQVBR has an optional peak.
		if ((limit_bitrate || capped_quality) && maximum_kbps > 0) {
			bitrate_kbps = maximum_kbps;
		} else {
			return 100000000.0;
		}
	} else if (mode == "VBR" || mode == "AVBR" || mode == "LA_VBR" || mode == "VBR_LAT" || mode == "HQVBR" ||
		   mode == "QVBR" || (mode == "ABR" && limit_bitrate)) {
		bitrate_kbps = std::max(bitrate_kbps, maximum_kbps);
	}

	if (bitrate_kbps <= 0) {
		return 0.0;
	}
	// Retain WHIP's 10x keyframe headroom, with room for low-rate encoder overshoot.
	// Convert before multiplying: high encoder bitrates can overflow a 32-bit int.
	return std::max(4000000.0, double(bitrate_kbps) * 10000.0);
}
