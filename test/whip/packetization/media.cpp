#include "../../../plugins/obs-webrtc/whip-media-utils.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

static void require(bool condition, const char *message)
{
	if (!condition) {
		throw std::runtime_error(message);
	}
}

static void test_timestamps()
{
	constexpr uint32_t offset = 0xfffffff0;
	require(whip_rtp_timestamp(0, 90000, offset) == offset, "Initial RTP offset changed");
	require(whip_rtp_timestamp(20000, 48000, offset) == uint32_t(offset + 960), "Opus step or wrap failed");
	require(whip_rtp_timestamp(-20000, 48000, offset) == uint32_t(offset - 960), "Negative preroll failed");
	require(whip_rtp_timestamp(250, 90000, 0) == 23, "Positive half tick must round up");
	require(whip_rtp_timestamp(-250, 90000, 0) == uint32_t(-23), "Negative half tick must round down");

	// Compare to the encoder timebase for every frame over an hour. Incrementally
	// rounded durations drift by hundreds of milliseconds at fractional rates.
	for (auto fps :
	     {std::pair{24, 1}, {30, 1}, {60, 1}, {24000, 1001}, {30000, 1001}, {60000, 1001}, {120000, 1001}}) {
		const int64_t frames = int64_t(3600) * fps.first / fps.second;
		for (int64_t frame = 0; frame <= frames; frame++) {
			const int64_t usec = frame * fps.second * 1000000 / fps.first;
			const uint32_t expected =
				uint32_t(std::llround(double(frame) * fps.second * 90000 / fps.first));
			const uint32_t actual = whip_rtp_timestamp(usec, 90000, offset) - offset;
			require(std::abs(int64_t(actual) - expected) <= 1, "Fractional frame rate accumulated drift");
		}
	}

	// Long uptime, skipped frames, and multiple RTP wraps must not change the
	// clock conversion or overflow the microseconds-to-ticks multiplication.
	for (int64_t seconds : {0LL, 47722LL, 89479LL, 864000LL, 1000000000000LL}) {
		for (uint32_t rate : {48000U, 90000U}) {
			const uint32_t expected = uint32_t(uint64_t(seconds) * rate + rate / 2);
			require(whip_rtp_timestamp(seconds * 1000000 + 500000, rate, offset) - offset == expected,
				"Long-running media clock overflowed");
		}
	}
	const int64_t last = std::numeric_limits<int64_t>::max();
	const uint32_t tail = whip_rtp_timestamp(last, 90000, 0) - whip_rtp_timestamp(last - 1000000, 90000, 0);
	require(tail == 90000, "Extreme media clock must retain one-second steps");
	std::cout << "RTP: seven frame rates, negative preroll, Opus, skipped time, offsets and wrap passed\n";
}

static void test_settings()
{
	for (int64_t value : std::initializer_list<int64_t>{-10, 0, 2, 10, std::numeric_limits<int64_t>::max()}) {
		require(whip_keyframe_interval(value) == 2, "Default or long keyframe interval was not bounded");
	}
	require(whip_keyframe_interval(1) == 1, "Shorter keyframe interval was not preserved");
	for (const char *mode : {"ICQ", "LA_ICQ", "CQP", "crf", "CQ", "lossless"}) {
		for (int64_t inactive : {0LL, 1LL, 500LL, 500000LL}) {
			require(whip_pacing_bitrate(mode, inactive, 2000, false) == 100000000,
				"Inactive bitrate or peak throttled quality mode");
		}
	}
	for (const char *mode : {"VBR", "AVBR", "LA_VBR", "VBR_LAT", "HQVBR", "QVBR"}) {
		require(whip_pacing_bitrate(mode, 2500, 12000, false) == 120000000, "VBR peak ignored");
		require(whip_pacing_bitrate(mode, 12000, 2500, false) == 120000000, "Lower peak throttled target");
	}
	for (const char *mode : {"CQVBR", "VBR_CQ"}) {
		require(whip_pacing_bitrate(mode, 1, 12000, false) == 120000000, "Quality peak ignored");
		require(whip_pacing_bitrate(mode, 1, 0, false) == 100000000, "Unbounded quality used inactive bitrate");
	}
	require(whip_pacing_bitrate("CRF", 1, 8000, true) == 80000000, "VideoToolbox CRF limit ignored");
	require(whip_pacing_bitrate("ABR", 2500, 8000, true) == 80000000, "VideoToolbox ABR limit ignored");
	require(whip_pacing_bitrate("ABR", 2500, 8000, false) == 25000000, "Inactive ABR peak was used");
	require(whip_pacing_bitrate("CBR", 2500, 50000, false) == 25000000, "CBR headroom changed");
	require(whip_pacing_bitrate("CBR", 100, 0, false) == 4000000, "Low-rate overshoot headroom missing");
	require(whip_pacing_bitrate("CBR", 500000, 0, false) == 5000000000.0, "High bitrate overflowed");
	require(whip_pacing_bitrate("CBR", 0, 0, false) == 0, "Zero bitrate enabled a stalled pacer");
	require(whip_pacing_bitrate("CBR", -1, 0, false) == 0, "Negative bitrate enabled a stalled pacer");
	std::cout << "Encoder settings: keyframe interval, rate-control modes, limits and overflow passed\n";
}

int main()
{
	test_timestamps();
	test_settings();
}
