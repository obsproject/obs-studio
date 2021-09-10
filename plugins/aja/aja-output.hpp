#pragma once

#include "aja-props.hpp"

#include <ajantv2/includes/ntv2testpatterngen.h>

#include <ajabase/common/types.h>
#include <ajabase/system/thread.h>

// #define AJA_OUTPUT_PROFILE 1
#ifdef AJA_OUTPUT_PROFILE
#include <ajabase/system/log.h>
#endif

// #define AJA_WRITE_DEBUG_WAV 1
#ifdef AJA_WRITE_DEBUG_WAV
#include <ajabase/common/wavewriter.h>
#endif

#include <deque>
#include <memory>
#include <mutex>

struct VideoFrame {
	struct video_data frame;
	int64_t frameNum;
	size_t size;
};

struct AudioFrames {
	struct audio_data frames;
	size_t offset;
	size_t size;
};

//TODO(paulh): Refactor me into OutputProps
struct FrameTimes {
	double obsFps;
	uint64_t obsFrameTime;
	double cardFps;
	uint64_t cardFrameTime;
};

using VideoQueue = std::deque<VideoFrame>;
using AudioQueue = std::deque<AudioFrames>;

class CNTV2Card; // forward decl

class AJAOutput {
public:
	enum {
		// min queue sizes computed in AJAOutput
		kVideoQueueMaxSize = 15,
		kAudioQueueMaxSize =
			96, // ~(48000 / 1024 samples per audio_frame) * 2sec
	};

	AJAOutput(CNTV2Card *card, const std::string &cardID,
		  const std::string &outputID, UWord deviceIndex,
		  const NTV2DeviceID deviceID);

	~AJAOutput();

	CNTV2Card *GetCard();

	void Initialize(const OutputProps &props);

	void SetOBSOutput(obs_output_t *output);
	obs_output_t *GetOBSOutput();

	void SetOutputProps(const OutputProps &props);
	OutputProps GetOutputProps() const;

	void GenerateTestPattern(NTV2VideoFormat vf, NTV2PixelFormat pf,
				 NTV2TestPatternSelect pattern);

	void QueueVideoFrame(struct video_data *frame, size_t size);
	void QueueAudioFrames(struct audio_data *frames, size_t size);
	void ClearVideoQueue();
	void ClearAudioQueue();
	size_t VideoQueueSize();
	size_t AudioQueueSize();

	bool PrerolledAudio();
	bool HaveEnoughAudio(size_t needAudioSize);
	void DMAAudioFromQueue(NTV2AudioSystem audioSys);
	void DMAVideoFromQueue();

	void CreateThread(bool enable = false);
	void StopThread();
	bool ThreadRunning();
	static void OutputThread(AJAThread *thread, void *ctx);

	std::string mCardID;
	std::string mOutputID;
	UWord mDeviceIndex;
	NTV2DeviceID mDeviceID;

	FrameTimes mFrameTimes;

	double mAudioPrerollSec;
	uint32_t mAudioPrerollBytes;

	uint32_t mMinVideoQueueSize;
	uint32_t mMinAudioQueueSize;

	uint32_t mAudioPlayCursor;
	uint32_t mAudioWriteCursor;
	uint32_t mAudioWrapAddress;

	uint32_t mNumCardFrames;
	uint32_t mFirstCardFrame;
	uint32_t mCurrentCardFrame;
	uint32_t mLastCardFrame;

	uint64_t mVideoFrameCount;
	uint64_t mFirstVideoTS;
	uint64_t mFirstAudioTS;
	uint64_t mLastVideoTS;
	uint64_t mLastAudioTS;

#ifdef AJA_OUTPUT_PROFILE
	uint64_t mVideoLastTime{0};
	uint64_t mAudioLastTime{0};
	AJARunAverage *mVideoQueueTimes;
	AJARunAverage *mAudioQueueTimes;
#endif

#ifdef AJA_WRITE_DEBUG_WAV
	AJAWavWriter *mWaveWriter;
#endif

private:
	void calculate_card_frame_indices(uint32_t numFrames, NTV2DeviceID id,
					  NTV2Channel channel,
					  NTV2VideoFormat vf,
					  NTV2PixelFormat pf);

	void increment_card_frame();

	void dma_audio_samples(NTV2AudioSystem audioSys, uint32_t *data,
			       size_t size);

	void adjust_initial_card_av_sync();

	CNTV2Card *mCard;

	OutputProps mOutputProps;

	NTV2TestPatternBuffer mTestPattern;

	bool mIsRunning;
	bool mAudioStarted;

	AJAThread mRunThread;
	mutable std::mutex mVideoLock;
	mutable std::mutex mAudioLock;
	mutable std::mutex mRunThreadLock;

	std::unique_ptr<VideoQueue> mVideoQueue;
	std::unique_ptr<AudioQueue> mAudioQueue;

	obs_output_t *mOBSOutput;
};
