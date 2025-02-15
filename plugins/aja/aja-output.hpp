#pragma once

#include "aja-props.hpp"

#include <ajantv2/includes/ntv2testpatterngen.h>

#include <ajabase/common/types.h>
#include <ajabase/system/thread.h>

// #define AJA_WRITE_DEBUG_WAV
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

using VideoQueue = std::deque<VideoFrame>;
using AudioQueue = std::deque<AudioFrames>;

class CNTV2Card; // forward decl

class AJAOutput {
public:
	enum {
		// min queue sizes computed in AJAOutput
		kVideoQueueMaxSize = 15,
		kAudioQueueMaxSize = 96, // ~(48000 / 1024 samples per audio_frame) * 2sec
	};

	AJAOutput(CNTV2Card *card, const std::string &cardID, const std::string &outputID, UWord deviceIndex,
		  const NTV2DeviceID deviceID);

	~AJAOutput();

	CNTV2Card *GetCard();

	void Initialize(const OutputProps &props);

	void SetOBSOutput(obs_output_t *output);
	obs_output_t *GetOBSOutput();

	void SetOutputProps(const OutputProps &props);
	OutputProps GetOutputProps() const;

	void CacheConnections(const NTV2XptConnections &cnx);
	void ClearConnections();

	void GenerateTestPattern(NTV2VideoFormat vf, NTV2PixelFormat pf, NTV2TestPatternSelect pattern,
				 uint32_t frameNum);

	void QueueVideoFrame(struct video_data *frame, size_t size);
	void QueueAudioFrames(struct audio_data *frames, size_t size);
	void ClearVideoQueue();
	void ClearAudioQueue();
	size_t VideoQueueSize();
	size_t AudioQueueSize();

	void DMAAudioFromQueue(NTV2AudioSystem audioSys, uint32_t channels, uint32_t sampleRate, uint32_t sampleSize);
	void DMAVideoFromQueue();

	void CreateThread(bool enable = false);
	void StopThread();
	bool ThreadRunning();
	static void OutputThread(AJAThread *thread, void *ctx);

	std::string mCardID;
	std::string mOutputID;
	UWord mDeviceIndex;
	NTV2DeviceID mDeviceID;

	uint32_t mAudioPlayCursor;
	uint32_t mAudioWriteCursor;
	uint32_t mAudioWrapAddress;

	uint64_t mAudioQueueBytes;
	uint64_t mAudioWriteBytes;
	uint64_t mAudioPlayBytes;

	uint32_t mNumCardFrames;
	uint32_t mFirstCardFrame;
	uint32_t mLastCardFrame;
	uint32_t mWriteCardFrame;
	uint32_t mPlayCardFrame;
	uint32_t mPlayCardNext;
	uint32_t mFrameRateNum;
	uint32_t mFrameRateDen;

	uint64_t mVideoQueueFrames;
	uint64_t mVideoWriteFrames;
	uint64_t mVideoPlayFrames;

	uint64_t mFirstVideoTS;
	uint64_t mFirstAudioTS;
	uint64_t mLastVideoTS;
	uint64_t mLastAudioTS;

	int64_t mOutputDelay;
	int64_t mVideoMax;
	int64_t mVideoDelay;
	int64_t mVideoSync;
	int64_t mVideoAdjust;
	int64_t mAudioMax;
	int64_t mAudioDelay;
	int64_t mAudioSync;
	int64_t mAudioAdjust;
	int64_t mLastStatTime;
#ifdef AJA_WRITE_DEBUG_WAV
	AJAWavWriter *mWaveWriter;
#endif

private:
	void reset_frame_counts();
	void calculate_card_frame_indices(uint32_t numFrames, NTV2DeviceID id, NTV2Channel channel, NTV2VideoFormat vf,
					  NTV2PixelFormat pf);
	uint32_t get_card_play_count();
	void dma_audio_samples(NTV2AudioSystem audioSys, uint32_t *data, size_t size);

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

	NTV2XptConnections mCrosspoints;
};
