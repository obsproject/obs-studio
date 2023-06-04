#include "aja-card-manager.hpp"
#include "aja-common.hpp"
#include "aja-ui-props.hpp"
#include "aja-output.hpp"
#include "aja-routing.hpp"

#include <obs-module.h>
#include <util/platform.h>

#include <ajabase/common/timer.h>
#include <ajabase/system/systemtime.h>

#include <ajantv2/includes/ntv2card.h>
#include <ajantv2/includes/ntv2devicefeatures.h>

#include <atomic>
#include <stdlib.h>
#include <inttypes.h>

// Log AJA Output video/audio delay/sync
// #define AJA_OUTPUT_STATS
// Log AJA Output card frame buffer numbers
// #define AJA_OUTPUT_FRAME_NUMBERS

#define MATCH_OBS_FRAMERATE true

static constexpr uint32_t kNumCardFrames = 8;
static constexpr uint32_t kFrameDelay = 4;
static constexpr int64_t kDefaultStatPeriod = 2000000000;
static constexpr int64_t kAudioSyncAdjust = 500;
static constexpr int64_t kVideoSyncAdjust = 1;

static void copy_audio_data(struct audio_data *src, struct audio_data *dst,
			    size_t size)
{
	if (src->data[0]) {
		dst->data[0] = (uint8_t *)bmemdup(src->data[0], size);
	}
}

static void free_audio_data(struct audio_data *frames)
{
	if (frames->data[0]) {
		bfree(frames->data[0]);
		frames->data[0] = NULL;
	}
	memset(frames, 0, sizeof(*frames));
}

static void copy_video_data(struct video_data *src, struct video_data *dst,
			    size_t size)
{
	if (src->data[0]) {
		dst->data[0] = (uint8_t *)bmemdup(src->data[0], size);
	}
}

static void free_video_frame(struct video_data *frame)
{
	if (frame->data[0]) {
		bfree(frame->data[0]);
		frame->data[0] = NULL;
	}

	memset(frame, 0, sizeof(*frame));
}

static void update_sdi_transport_and_sdi_transport_4k(obs_properties_t *props,
						      NTV2DeviceID device_id,
						      IOSelection io,
						      NTV2VideoFormat vf)
{
	// Update SDI Transport and SDI 4K Transport selections
	obs_property_t *sdi_trx_list =
		obs_properties_get(props, kUIPropSDITransport.id);
	obs_property_list_clear(sdi_trx_list);
	populate_sdi_transport_list(sdi_trx_list, device_id);
	obs_property_t *sdi_4k_trx_list =
		obs_properties_get(props, kUIPropSDITransport4K.id);
	obs_property_list_clear(sdi_4k_trx_list);
	populate_sdi_4k_transport_list(sdi_4k_trx_list);

	bool is_sdi = aja::IsIOSelectionSDI(io);
	obs_property_set_visible(sdi_trx_list, is_sdi);
	obs_property_set_visible(sdi_4k_trx_list,
				 is_sdi && NTV2_IS_4K_VIDEO_FORMAT(vf));
}

AJAOutput::AJAOutput(CNTV2Card *card, const std::string &cardID,
		     const std::string &outputID, UWord deviceIndex,
		     const NTV2DeviceID deviceID)
	: mCardID{cardID},
	  mOutputID{outputID},
	  mDeviceIndex{deviceIndex},
	  mDeviceID{deviceID},
	  mAudioPlayCursor{0},
	  mAudioWriteCursor{0},
	  mAudioWrapAddress{0},
	  mAudioQueueBytes{0},
	  mAudioWriteBytes{0},
	  mAudioPlayBytes{0},
	  mNumCardFrames{0},
	  mFirstCardFrame{0},
	  mLastCardFrame{0},
	  mWriteCardFrame{0},
	  mPlayCardFrame{0},
	  mPlayCardNext{0},
	  mFrameRateNum{0},
	  mFrameRateDen{0},
	  mVideoQueueFrames{0},
	  mVideoWriteFrames{0},
	  mVideoPlayFrames{0},
	  mFirstVideoTS{0},
	  mFirstAudioTS{0},
	  mLastVideoTS{0},
	  mLastAudioTS{0},
	  mOutputDelay{0},
	  mVideoMax{0},
	  mVideoDelay{0},
	  mVideoSync{0},
	  mVideoAdjust{0},
	  mAudioMax{0},
	  mAudioDelay{0},
	  mAudioSync{0},
	  mAudioAdjust{0},
	  mLastStatTime{0},
#ifdef AJA_WRITE_DEBUG_WAV
	  mWaveWriter{nullptr},
#endif
	  mCard{card},
	  mOutputProps{DEVICE_ID_NOTFOUND},
	  mTestPattern{},
	  mIsRunning{false},
	  mAudioStarted{false},
	  mRunThread{},
	  mVideoLock{},
	  mAudioLock{},
	  mRunThreadLock{},
	  mVideoQueue{},
	  mAudioQueue{},
	  mOBSOutput{nullptr},
	  mCrosspoints{}
{
	mVideoQueue = std::make_unique<VideoQueue>();
	mAudioQueue = std::make_unique<AudioQueue>();
}

AJAOutput::~AJAOutput()
{
	if (mVideoQueue)
		mVideoQueue.reset();
	if (mAudioQueue)
		mAudioQueue.reset();
}

CNTV2Card *AJAOutput::GetCard()
{
	return mCard;
}

void AJAOutput::Initialize(const OutputProps &props)
{
	reset_frame_counts();

	// Store the address to the end of the card's audio buffer.
	mCard->GetAudioWrapAddress(mAudioWrapAddress, props.AudioSystem());

	// Specify small ring of frame buffers on the card for DMA and playback.
	// Starts at frame index corresponding to the output Channel * numFrames.
	calculate_card_frame_indices(kNumCardFrames, mCard->GetDeviceID(),
				     props.Channel(), props.videoFormat,
				     props.pixelFormat);

	// Write black frames to the card to prevent playing garbage before the first DMA write.
	for (uint32_t i = mFirstCardFrame; i <= mLastCardFrame; i++) {
		GenerateTestPattern(props.videoFormat, props.pixelFormat,
				    NTV2_TestPatt_Black, i);
	}

	mCard->WaitForOutputVerticalInterrupt(props.Channel());
	const auto &cardFrameRate =
		GetNTV2FrameRateFromVideoFormat(props.videoFormat);
	ULWord fpsNum = 0;
	ULWord fpsDen = 0;
	GetFramesPerSecond(cardFrameRate, fpsNum, fpsDen);
	mFrameRateNum = fpsNum;
	mFrameRateDen = fpsDen;
	mOutputDelay =
		(int64_t)kFrameDelay * 1000000 * mFrameRateDen / mFrameRateNum;
	mVideoMax = (int64_t)kVideoSyncAdjust * 1000000 * mFrameRateDen /
		    mFrameRateNum;
	mVideoMax -= 100;
	mAudioMax = (int64_t)kAudioSyncAdjust * 1000000 / props.audioSampleRate;
	SetOutputProps(props);
}

void AJAOutput::reset_frame_counts()
{
	mAudioQueueBytes = 0;
	mAudioWriteBytes = 0;
	mAudioPlayBytes = 0;
	mVideoQueueFrames = 0;
	mVideoWriteFrames = 0;
	mVideoPlayFrames = 0;
}

void AJAOutput::SetOBSOutput(obs_output_t *output)
{
	mOBSOutput = output;
}

obs_output_t *AJAOutput::GetOBSOutput()
{
	return mOBSOutput;
}

void AJAOutput::SetOutputProps(const OutputProps &props)
{
	mOutputProps = props;
}

OutputProps AJAOutput::GetOutputProps() const
{
	return mOutputProps;
}

void AJAOutput::CacheConnections(const NTV2XptConnections &cnx)
{
	mCrosspoints.clear();
	mCrosspoints = cnx;
}

void AJAOutput::ClearConnections()
{
	for (auto &&xpt : mCrosspoints) {
		mCard->Connect(xpt.first, NTV2_XptBlack);
	}
	mCrosspoints.clear();
}

void AJAOutput::GenerateTestPattern(NTV2VideoFormat vf, NTV2PixelFormat pf,
				    NTV2TestPatternSelect pattern,
				    uint32_t frameNum)
{
	NTV2VideoFormat vid_fmt = vf;
	NTV2PixelFormat pix_fmt = pf;
	if (vid_fmt == NTV2_FORMAT_UNKNOWN)
		vid_fmt = NTV2_FORMAT_720p_5994;
	if (pix_fmt == NTV2_FBF_INVALID)
		pix_fmt = kDefaultAJAPixelFormat;

	NTV2FormatDesc fd(vid_fmt, pix_fmt, NTV2_VANCMODE_OFF);
	auto bufSize = fd.GetTotalRasterBytes();
	if (bufSize != mTestPattern.size()) {
		// Raster size changed, regenerate pattern
		mTestPattern.clear();
		mTestPattern.resize(bufSize);
		NTV2TestPatternGen gen;
		gen.DrawTestPattern(pattern, fd.GetRasterWidth(),
				    fd.GetRasterHeight(), pix_fmt,
				    mTestPattern);
	}

	if (mTestPattern.size() == 0) {
		blog(LOG_DEBUG,
		     "AJAOutput::GenerateTestPattern: Error generating test pattern!");
		return;
	}

	if (mCard->DMAWriteFrame(
		    frameNum,
		    reinterpret_cast<ULWord *>(&mTestPattern.data()[0]),
		    static_cast<ULWord>(mTestPattern.size()))) {
		mCard->SetOutputFrame(mOutputProps.Channel(), frameNum);
	}
}

void AJAOutput::QueueVideoFrame(struct video_data *frame, size_t size)
{
	const std::lock_guard<std::mutex> lock(mVideoLock);

	VideoFrame vf;
	vf.frame = *frame;
	vf.frameNum = mVideoWriteFrames;
	vf.size = size;

	if (mVideoQueue->size() > kVideoQueueMaxSize) {
		auto &front = mVideoQueue->front();
		free_video_frame(&front.frame);
		mVideoQueue->pop_front();
	}

	copy_video_data(frame, &vf.frame, size);

	mVideoQueue->push_back(vf);
	mVideoQueueFrames++;
}

void AJAOutput::QueueAudioFrames(struct audio_data *frames, size_t size)
{
	const std::lock_guard<std::mutex> lock(mAudioLock);

	AudioFrames af;
	af.frames = *frames;
	af.offset = 0;
	af.size = size;

	if (mAudioQueue->size() > kAudioQueueMaxSize) {
		auto &front = mAudioQueue->front();
		free_audio_data(&front.frames);
		mAudioQueue->pop_front();
	}

	copy_audio_data(frames, &af.frames, size);

	mAudioQueue->push_back(af);
	mAudioQueueBytes += size;
}

void AJAOutput::ClearVideoQueue()
{
	const std::lock_guard<std::mutex> lock(mVideoLock);
	while (mVideoQueue->size() > 0) {
		auto &vf = mVideoQueue->front();
		free_video_frame(&vf.frame);
		mVideoQueue->pop_front();
	}
}

void AJAOutput::ClearAudioQueue()
{
	const std::lock_guard<std::mutex> lock(mAudioLock);
	while (mAudioQueue->size() > 0) {
		auto &af = mAudioQueue->front();
		free_audio_data(&af.frames);
		mAudioQueue->pop_front();
	}
}

size_t AJAOutput::VideoQueueSize()
{
	return mVideoQueue->size();
}

size_t AJAOutput::AudioQueueSize()
{
	return mAudioQueue->size();
}

// lock audio queue before calling
void AJAOutput::DMAAudioFromQueue(NTV2AudioSystem audioSys, uint32_t channels,
				  uint32_t sampleRate, uint32_t sampleSize)
{
	AudioFrames &af = mAudioQueue->front();
	size_t sizeLeft = af.size - af.offset;

	if (!mFirstAudioTS)
		mFirstAudioTS = af.frames.timestamp;
	mLastAudioTS = af.frames.timestamp;

	if (sizeLeft == 0) {
		free_audio_data(&af.frames);
		mAudioQueue->pop_front();
		return;
	}

	// Get audio play cursor
	mCard->ReadAudioLastOut(mAudioPlayCursor, audioSys);

	// Calculate audio delay
	uint32_t audioPlaySamples = 0;
	uint32_t audioPlayBytes = mAudioWriteCursor - mAudioPlayCursor;
	if (mAudioPlayCursor <= mAudioWriteCursor) {
		audioPlaySamples = audioPlayBytes / (channels * sampleSize);
	} else {
		audioPlayBytes = mAudioWrapAddress - mAudioPlayCursor +
				 mAudioWriteCursor;
		audioPlaySamples = audioPlayBytes / (channels * sampleSize);
	}
	mAudioDelay = 1000000 * (int64_t)audioPlaySamples / sampleRate;
	mAudioPlayBytes += audioPlayBytes;

	// Adjust audio sync when requested
	if (mAudioAdjust != 0) {
		if (mAudioAdjust > 0) {
			// Throw away some samples to resync audio
			uint32_t adjustSamples =
				(uint32_t)mAudioAdjust * sampleRate / 1000000;
			uint32_t adjustSize =
				adjustSamples * sampleSize * channels;
			if (adjustSize <= sizeLeft) {
				af.offset += adjustSize;
				sizeLeft -= adjustSize;
				mAudioAdjust = 0;
				blog(LOG_DEBUG,
				     "AJAOutput::DMAAudioFromQueue: Drop %d audio samples",
				     adjustSamples);
			} else {
				uint32_t samples = (uint32_t)sizeLeft /
						   (sampleSize * channels);
				af.offset += sizeLeft;
				sizeLeft = 0;
				adjustSamples -= samples;
				mAudioAdjust = (int64_t)adjustSamples *
					       1000000 / sampleRate;
				blog(LOG_DEBUG,
				     "AJAOutput::DMAAudioFromQueue: Drop %d audio samples",
				     samples);
			}
		} else {
			// Add some silence to resync audio
			uint32_t adjustSamples = (uint32_t)(-mAudioAdjust) *
						 sampleRate / 1000000;
			uint32_t adjustSize =
				adjustSamples * sampleSize * channels;
			uint8_t *silentBuffer = new uint8_t[adjustSize];
			memset(silentBuffer, 0, adjustSize);
			dma_audio_samples(audioSys, (uint32_t *)silentBuffer,
					  adjustSize);
			delete[] silentBuffer;
			mAudioAdjust = 0;
			blog(LOG_DEBUG,
			     "AJAOutput::DMAAudioFromQueue: Add %d audio samples",
			     adjustSamples);
		}
	}

	// Write audio to the hardware ring
	if (af.frames.data[0] && sizeLeft > 0) {
		dma_audio_samples(audioSys,
				  (uint32_t *)&af.frames.data[0][af.offset],
				  sizeLeft);
		af.offset += sizeLeft;
	}

	// Free the audio buffer
	if (af.offset == af.size) {
		free_audio_data(&af.frames);
		mAudioQueue->pop_front();
	}
}

// lock video queue before calling
void AJAOutput::DMAVideoFromQueue()
{
	bool writeFrame = true;
	bool freeFrame = true;

	auto &vf = mVideoQueue->front();
	auto data = vf.frame.data[0];

	if (!mFirstVideoTS)
		mFirstVideoTS = vf.frame.timestamp;
	mLastVideoTS = vf.frame.timestamp;

	mVideoDelay = (((int64_t)mWriteCardFrame + (int64_t)mNumCardFrames -
			(int64_t)mPlayCardFrame) %
		       (int64_t)mNumCardFrames) *
		      1000000 * mFrameRateDen / mFrameRateNum;

	// Adjust video sync when requested
	if (mVideoAdjust != 0) {
		if (mVideoAdjust > 0) {
			// skip writing this frame
			writeFrame = false;
		} else {
			// write this frame twice
			freeFrame = false;
		}
		mVideoAdjust = 0;
	}

	if (writeFrame) {
		// find the next buffer
		uint32_t writeCardFrame = mWriteCardFrame + 1;
		if (writeCardFrame > mLastCardFrame)
			writeCardFrame = mFirstCardFrame;

		// use the next buffer if available
		if (writeCardFrame != mPlayCardFrame)
			mWriteCardFrame = writeCardFrame;

		mVideoWriteFrames++;

		auto result = mCard->DMAWriteFrame(
			mWriteCardFrame, reinterpret_cast<ULWord *>(data),
			(ULWord)vf.size);
		if (!result)
			blog(LOG_DEBUG,
			     "AJAOutput::DMAVideoFromQueue: Failed to write video frame!");
	}

	if (freeFrame) {
		free_video_frame(&vf.frame);
		mVideoQueue->pop_front();
	}
}

// TODO(paulh): Keep track of framebuffer indices used on the card, between the capture
// and output plugins, so that we can optimize frame index placement in memory and
// reduce unused gaps in between channel frame indices.
void AJAOutput::calculate_card_frame_indices(uint32_t numFrames,
					     NTV2DeviceID id,
					     NTV2Channel channel,
					     NTV2VideoFormat vf,
					     NTV2PixelFormat pf)
{
	ULWord channelIndex = GetIndexForNTV2Channel(channel);
	ULWord totalCardFrames = NTV2DeviceGetNumberFrameBuffers(
		id, GetNTV2FrameGeometryFromVideoFormat(vf), pf);
	mFirstCardFrame = channelIndex * numFrames;
	uint32_t lastFrame = mFirstCardFrame + (numFrames - 1);
	if (totalCardFrames - mFirstCardFrame > 0 &&
	    totalCardFrames - lastFrame > 0) {
		// Reserve N framebuffers in card DRAM.
		mNumCardFrames = numFrames;
		mWriteCardFrame = mFirstCardFrame;
		mLastCardFrame = lastFrame;
		mPlayCardFrame = mWriteCardFrame;
		mPlayCardNext = mPlayCardFrame + 1;
		blog(LOG_INFO, "AJA Output using %d card frames (%d-%d).",
		     mNumCardFrames, mFirstCardFrame, mLastCardFrame);
	} else {
		blog(LOG_WARNING,
		     "AJA Output Card frames %d-%d out of bounds. %d total frames on card!",
		     mFirstCardFrame, mLastCardFrame, totalCardFrames);
	}
}

uint32_t AJAOutput::get_card_play_count()
{
	uint32_t frameCount = 0;
	NTV2Channel channel = mOutputProps.Channel();
	INTERRUPT_ENUMS interrupt = NTV2ChannelToOutputInterrupt(channel);
	bool isProgressiveTransport = NTV2_IS_PROGRESSIVE_STANDARD(
		::GetNTV2StandardFromVideoFormat(mOutputProps.videoFormat));

	if (isProgressiveTransport) {
		mCard->GetInterruptCount(interrupt, frameCount);
	} else {
		uint32_t intCount;
		uint32_t nextCount;
		NTV2FieldID fieldID;
		mCard->GetInterruptCount(interrupt, intCount);
		mCard->GetOutputFieldID(channel, fieldID);
		mCard->GetInterruptCount(interrupt, nextCount);
		if (intCount != nextCount) {
			mCard->GetInterruptCount(interrupt, intCount);
			mCard->GetOutputFieldID(channel, fieldID);
		}
		if (fieldID == NTV2_FIELD1)
			intCount--;
		frameCount = intCount / 2;
	}

	return frameCount;
}

// Perform DMA of audio samples to AJA card while taking into account wrapping around the
// ends of the card's audio buffer (size set to 4MB in aja::Routing::ConfigureOutputAudio).
void AJAOutput::dma_audio_samples(NTV2AudioSystem audioSys, uint32_t *data,
				  size_t size)
{
	bool result = false;

	if ((mAudioWriteCursor + size) > mAudioWrapAddress) {
		const uint32_t remainingBuffer =
			mAudioWrapAddress - mAudioWriteCursor;

		auto audioDataRemain = reinterpret_cast<const ULWord *>(
			(uint8_t *)(data) + remainingBuffer);

		// Incoming audio size will wrap around the end of the card audio buffer.
		// Transfer enough bytes to fill to the end of the buffer...
		if (remainingBuffer > 0) {
			result = mCard->DMAWriteAudio(audioSys, data,
						      mAudioWriteCursor,
						      remainingBuffer);
			if (result) {
				mAudioWriteBytes += remainingBuffer;
			} else {
				blog(LOG_DEBUG,
				     "AJAOutput::dma_audio_samples: "
				     "failed to write bytes at end of buffer (address = %d)",
				     mAudioWriteCursor);
			}
		}

		// ...transfer remaining bytes at the front of the card audio buffer.
		size_t frontRemaining = size - remainingBuffer;
		if (frontRemaining > 0) {
			result = mCard->DMAWriteAudio(audioSys, audioDataRemain,
						      0,
						      (ULWord)frontRemaining);
			if (result) {
				mAudioWriteBytes += frontRemaining;
			} else {
				blog(LOG_DEBUG,
				     "AJAOutput::dma_audio_samples "
				     "failed to write bytes at front of buffer (address = %d)",
				     mAudioWriteCursor);
			}
		}

		mAudioWriteCursor = (uint32_t)size - remainingBuffer;
	} else {
		//	No wrap, so just do a linear DMA from the buffer...
		if (size > 0) {
			result = mCard->DMAWriteAudio(audioSys, data,
						      mAudioWriteCursor,
						      (ULWord)size);
			if (result) {
				mAudioWriteBytes += size;
			} else {
				blog(LOG_DEBUG,
				     "AJAOutput::dma_audio_samples "
				     "failed to write bytes to buffer (address = %d)",
				     mAudioWriteCursor);
			}
		}

		mAudioWriteCursor += (uint32_t)size;
	}
}

void AJAOutput::CreateThread(bool enable)
{
	const std::lock_guard<std::mutex> lock(mRunThreadLock);
	if (!mRunThread.Active()) {
		mRunThread.SetPriority(AJA_ThreadPriority_High);
		mRunThread.SetThreadName("AJA Video Output Thread");
		mRunThread.Attach(AJAOutput::OutputThread, this);
	}
	if (enable) {
		mIsRunning = true;
		mRunThread.Start();
	}
}

void AJAOutput::StopThread()
{
	const std::lock_guard<std::mutex> lock(mRunThreadLock);
	mIsRunning = false;
	if (mRunThread.Active()) {
		mRunThread.Stop();
	}
}

bool AJAOutput::ThreadRunning()
{
	return mIsRunning;
}

void AJAOutput::OutputThread(AJAThread *thread, void *ctx)
{
	UNUSED_PARAMETER(thread);

	AJAOutput *ajaOutput = static_cast<AJAOutput *>(ctx);
	if (!ajaOutput) {
		blog(LOG_ERROR,
		     "AJAOutput::OutputThread: AJA Output instance is null!");
		return;
	}

	CNTV2Card *card = ajaOutput->GetCard();
	if (!card) {
		blog(LOG_ERROR,
		     "AJAOutput::OutputThread: Card instance is null!");
		return;
	}

	const auto &props = ajaOutput->GetOutputProps();
	const auto &audioSystem = props.AudioSystem();
	uint64_t videoPlayLast = ajaOutput->get_card_play_count();
	uint32_t audioSyncSlowCount = 0;
	uint32_t audioSyncFastCount = 0;
	uint32_t audioSyncCountMax = 3;
	uint32_t videoSyncSlowCount = 0;
	uint32_t videoSyncFastCount = 0;
	uint32_t videoSyncCountMax = 3;
	int64_t audioSyncSlowSum = 0;
	int64_t audioSyncFastSum = 0;

	// thread loop
	while (ajaOutput->ThreadRunning()) {
		// Wait for preroll
		if (!ajaOutput->mAudioStarted &&
		    (ajaOutput->mAudioDelay > ajaOutput->mOutputDelay)) {
			card->StartAudioOutput(audioSystem, false);
			ajaOutput->mAudioStarted = true;
			blog(LOG_DEBUG,
			     "AJAOutput::OutputThread: Audio Preroll complete");
		}

		// Check if a vsync occurred
		uint32_t frameCount = ajaOutput->get_card_play_count();
		if (frameCount > videoPlayLast) {
			videoPlayLast = frameCount;
			ajaOutput->mPlayCardFrame = ajaOutput->mPlayCardNext;

			if (ajaOutput->mPlayCardFrame !=
			    ajaOutput->mWriteCardFrame) {
				uint32_t playCardNext =
					ajaOutput->mPlayCardFrame + 1;
				if (playCardNext > ajaOutput->mLastCardFrame)
					playCardNext =
						ajaOutput->mFirstCardFrame;

				if (playCardNext !=
				    ajaOutput->mWriteCardFrame) {
					ajaOutput->mPlayCardNext = playCardNext;
					// Increment the play frame
					ajaOutput->mCard->SetOutputFrame(
						ajaOutput->mOutputProps
							.Channel(),
						ajaOutput->mPlayCardNext);
					ajaOutput->mVideoPlayFrames++;
				}
			}
		}

		// Audio DMA
		{
			const std::lock_guard<std::mutex> lock(
				ajaOutput->mAudioLock);
			while (ajaOutput->AudioQueueSize() > 0) {
				ajaOutput->DMAAudioFromQueue(
					audioSystem, props.audioNumChannels,
					props.audioSampleRate,
					props.audioSampleSize);
			}
		}

		// Video DMA
		{
			const std::lock_guard<std::mutex> lock(
				ajaOutput->mVideoLock);
			while (ajaOutput->VideoQueueSize() > 0) {
				ajaOutput->DMAVideoFromQueue();
			}
		}

		// Get current time and audio play cursor
		int64_t curTime = (int64_t)os_gettime_ns();
		card->ReadAudioLastOut(ajaOutput->mAudioPlayCursor,
				       audioSystem);

		if ((curTime - ajaOutput->mLastStatTime) > kDefaultStatPeriod) {
			ajaOutput->mLastStatTime = curTime;

			if (ajaOutput->mAudioStarted) {
				// Calculate audio sync delay
				ajaOutput->mAudioSync = ajaOutput->mAudioDelay -
							ajaOutput->mOutputDelay;

				if (ajaOutput->mAudioSync >
				    ajaOutput->mAudioMax) {
					audioSyncSlowCount++;
					audioSyncSlowSum +=
						ajaOutput->mAudioSync;
					if (audioSyncSlowCount >=
					    audioSyncCountMax) {
						ajaOutput->mAudioAdjust =
							audioSyncSlowSum /
							audioSyncCountMax;
						audioSyncSlowCount = 0;
						audioSyncSlowSum = 0;
					}
				} else {
					audioSyncSlowCount = 0;
					audioSyncSlowSum = 0;
				}
				if (ajaOutput->mAudioSync <
				    -ajaOutput->mAudioMax) {
					audioSyncFastCount++;
					audioSyncFastSum +=
						ajaOutput->mAudioSync;
					if (audioSyncFastCount >=
					    audioSyncCountMax) {
						ajaOutput->mAudioAdjust =
							audioSyncFastSum /
							audioSyncCountMax;
						audioSyncFastCount = 0;
						audioSyncFastSum = 0;
					}
				} else {
					audioSyncFastCount = 0;
					audioSyncFastSum = 0;
				}
			}

			// calculate video sync delay
			if (ajaOutput->mVideoDelay > 0) {
				ajaOutput->mVideoSync = ajaOutput->mVideoDelay -
							ajaOutput->mOutputDelay;

				if (ajaOutput->mVideoSync >
				    ajaOutput->mVideoMax) {
					videoSyncSlowCount++;
					if (videoSyncSlowCount >
					    videoSyncCountMax) {
						ajaOutput->mVideoAdjust = 1;
					}
				} else {
					videoSyncSlowCount = 0;
				}
				if (ajaOutput->mVideoSync <
				    -ajaOutput->mVideoMax) {
					videoSyncFastCount++;
					if (videoSyncFastCount >
					    videoSyncCountMax) {
						ajaOutput->mVideoAdjust = -1;
					}
				} else {
					videoSyncFastCount = 0;
				}
			}

#ifdef AJA_OUTPUT_STATS
			blog(LOG_INFO,
			     "AJAOutput::OutputThread: od %li  vd %li  vs %li  vm %li  ad %li  as %li  am %li",
			     ajaOutput->mOutputDelay, ajaOutput->mVideoDelay,
			     ajaOutput->mVideoSync, ajaOutput->mVideoMax,
			     ajaOutput->mAudioDelay, ajaOutput->mAudioSync,
			     ajaOutput->mAudioMax);
#endif
		}
#ifdef AJA_OUTPUT_FRAME_NUMBERS
		blog(LOG_INFO,
		     "AJAOutput::OutputThread: dma: %d play: %d next: %d",
		     ajaOutput->mWriteCardFrame, ajaOutput->mPlayCardFrame,
		     ajaOutput->mPlayCardNext);
#endif
		os_sleep_ms(1);
	}

	ajaOutput->mAudioStarted = false;

	// Log total number of queued/written/played video frames and audio samples
	uint32_t audioSize = props.audioNumChannels / props.audioSampleSize;
	if (audioSize > 0) {
		blog(LOG_INFO,
		     "AJAOutput::OutputThread: Thread stopped\n[Video] qf: %" PRIu64
		     " wf: %" PRIu64 " pf: %" PRIu64 "\n[Audio] qs: %" PRIu64
		     " ws: %" PRIu64 " ps: %" PRIu64,
		     ajaOutput->mVideoQueueFrames, ajaOutput->mVideoWriteFrames,
		     ajaOutput->mVideoPlayFrames,
		     ajaOutput->mAudioQueueBytes / audioSize,
		     ajaOutput->mAudioWriteBytes / audioSize,
		     ajaOutput->mAudioPlayBytes / audioSize);
	}
}

void populate_output_device_list(obs_property_t *list)
{
	obs_property_list_clear(list);

	auto &cardManager = aja::CardManager::Instance();
	cardManager.EnumerateCards();
	for (auto &iter : cardManager.GetCardEntries()) {
		if (!iter.second)
			continue;

		CNTV2Card *card = iter.second->GetCard();
		if (!card)
			continue;

		NTV2DeviceID deviceID = card->GetDeviceID();

		//TODO(paulh): Add support for analog I/O
		// w/ NTV2DeviceGetNumAnalogVideoOutputs(cardEntry.deviceID)
		if (NTV2DeviceGetNumVideoOutputs(deviceID) > 0 ||
		    NTV2DeviceGetNumHDMIVideoOutputs(deviceID) > 0) {

			obs_property_list_add_string(
				list, iter.second->GetDisplayName().c_str(),
				iter.second->GetCardID().c_str());
		}
	}
}

bool aja_output_device_changed(void *data, obs_properties_t *props,
			       obs_property_t *list, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);

	blog(LOG_DEBUG, "AJA Output Device Changed");

	populate_output_device_list(list);

	const char *cardID = obs_data_get_string(settings, kUIPropDevice.id);
	if (!cardID || !cardID[0])
		return false;

	const char *outputID =
		obs_data_get_string(settings, kUIPropAJAOutputID.id);
	auto &cardManager = aja::CardManager::Instance();
	cardManager.EnumerateCards();
	auto cardEntry = cardManager.GetCardEntry(cardID);
	if (!cardEntry) {
		blog(LOG_ERROR,
		     "aja_output_device_changed: Card Entry not found for %s",
		     cardID);
		return false;
	}

	CNTV2Card *card = cardEntry->GetCard();
	if (!card) {
		blog(LOG_ERROR,
		     "aja_output_device_changed: Card instance is null!");
		return false;
	}

	obs_property_t *io_select_list =
		obs_properties_get(props, kUIPropOutput.id);
	obs_property_t *vid_fmt_list =
		obs_properties_get(props, kUIPropVideoFormatSelect.id);
	obs_property_t *pix_fmt_list =
		obs_properties_get(props, kUIPropPixelFormatSelect.id);

	const NTV2DeviceID deviceID = cardEntry->GetDeviceID();
	populate_io_selection_output_list(cardID, outputID, deviceID,
					  io_select_list);

	// If Channel 1 is actively in use, filter the video format list to only
	// show video formats within the same framerate family. If Channel 1 is
	// not active we just go ahead and try to set all framestores to the same video format.
	// This is because Channel 1's clock rate will govern the card's Free Run clock.
	NTV2VideoFormat videoFormatChannel1 = NTV2_FORMAT_UNKNOWN;
	if (!cardEntry->ChannelReady(NTV2_CHANNEL1, outputID)) {
		card->GetVideoFormat(videoFormatChannel1, NTV2_CHANNEL1);
	}

	obs_property_list_clear(vid_fmt_list);
	populate_video_format_list(deviceID, vid_fmt_list, videoFormatChannel1,
				   false, MATCH_OBS_FRAMERATE);

	obs_property_list_clear(pix_fmt_list);
	populate_pixel_format_list(deviceID, pix_fmt_list);

	IOSelection io_select = static_cast<IOSelection>(
		obs_data_get_int(settings, kUIPropOutput.id));

	update_sdi_transport_and_sdi_transport_4k(
		props, cardEntry->GetDeviceID(), io_select,
		static_cast<NTV2VideoFormat>(obs_data_get_int(
			settings, kUIPropVideoFormatSelect.id)));

	return true;
}

bool aja_output_dest_changed(obs_properties_t *props, obs_property_t *list,
			     obs_data_t *settings)
{
	blog(LOG_DEBUG, "AJA Output Dest Changed");

	const char *cardID = obs_data_get_string(settings, kUIPropDevice.id);
	if (!cardID || !cardID[0])
		return false;

	auto &cardManager = aja::CardManager::Instance();
	auto cardEntry = cardManager.GetCardEntry(cardID);
	if (!cardEntry) {
		blog(LOG_DEBUG,
		     "aja_output_dest_changed: Card Entry not found for %s",
		     cardID);
		return false;
	}

	bool itemFound = false;
	const long long dest = obs_data_get_int(settings, kUIPropOutput.id);
	const size_t itemCount = obs_property_list_item_count(list);
	for (size_t i = 0; i < itemCount; i++) {
		const long long itemDest = obs_property_list_item_int(list, i);
		if (dest == itemDest) {
			itemFound = true;
			break;
		}
	}
	if (!itemFound) {
		obs_property_list_insert_int(list, 0, "", dest);
		obs_property_list_item_disable(list, 0, true);
		return true;
	}

	// Revert to "Select..." if desired IOSelection is already in use
	auto io_select = static_cast<IOSelection>(
		obs_data_get_int(settings, kUIPropOutput.id));
	for (size_t i = 0; i < obs_property_list_item_count(list); i++) {
		auto io_item = static_cast<IOSelection>(
			obs_property_list_item_int(list, i));
		if (io_item == io_select &&
		    obs_property_list_item_disabled(list, i)) {
			obs_data_set_int(
				settings, kUIPropOutput.id,
				static_cast<long long>(IOSelection::Invalid));
			blog(LOG_WARNING,
			     "aja_output_dest_changed: IOSelection %s is already in use",
			     aja::IOSelectionToString(io_select).c_str());
			return false;
		}
	}

	update_sdi_transport_and_sdi_transport_4k(
		props, cardEntry->GetDeviceID(), io_select,
		static_cast<NTV2VideoFormat>(obs_data_get_int(
			settings, kUIPropVideoFormatSelect.id)));

	return true;
}

static void aja_output_destroy(void *data)
{
	blog(LOG_DEBUG, "AJA Output Destroy");

	auto ajaOutput = (AJAOutput *)data;
	if (!ajaOutput) {
		blog(LOG_ERROR, "aja_output_destroy: Plugin instance is null!");
		return;
	}

#ifdef AJA_WRITE_DEBUG_WAV
	if (ajaOutput->mWaveWriter) {
		ajaOutput->mWaveWriter->close();
		delete ajaOutput->mWaveWriter;
		ajaOutput->mWaveWriter = nullptr;
	}
#endif
	ajaOutput->StopThread();
	ajaOutput->ClearVideoQueue();
	ajaOutput->ClearAudioQueue();
	delete ajaOutput;
	ajaOutput = nullptr;
}

static void *aja_output_create(obs_data_t *settings, obs_output_t *output)
{
	blog(LOG_INFO, "Creating AJA Output...");

	const char *cardID = obs_data_get_string(settings, kUIPropDevice.id);
	if (!cardID || !cardID[0])
		return nullptr;

	const char *outputID =
		obs_data_get_string(settings, kUIPropAJAOutputID.id);

	auto &cardManager = aja::CardManager::Instance();
	auto cardEntry = cardManager.GetCardEntry(cardID);
	if (!cardEntry) {
		blog(LOG_ERROR,
		     "aja_output_create: Card Entry not found for %s", cardID);
		return nullptr;
	}

	CNTV2Card *card = cardEntry->GetCard();
	if (!card) {
		blog(LOG_ERROR,
		     "aja_output_create: Card instance is null for %s", cardID);
		return nullptr;
	}

	NTV2DeviceID deviceID = card->GetDeviceID();
	OutputProps outputProps(deviceID);
	outputProps.ioSelect = static_cast<IOSelection>(
		obs_data_get_int(settings, kUIPropOutput.id));
	outputProps.videoFormat = static_cast<NTV2VideoFormat>(
		obs_data_get_int(settings, kUIPropVideoFormatSelect.id));
	outputProps.pixelFormat = static_cast<NTV2PixelFormat>(
		obs_data_get_int(settings, kUIPropPixelFormatSelect.id));
	outputProps.sdiTransport = static_cast<SDITransport>(
		obs_data_get_int(settings, kUIPropSDITransport.id));
	outputProps.sdi4kTransport = static_cast<SDITransport4K>(
		obs_data_get_int(settings, kUIPropSDITransport4K.id));
	outputProps.audioNumChannels = kDefaultAudioChannels;
	outputProps.audioSampleSize = kDefaultAudioSampleSize;
	outputProps.audioSampleRate = kDefaultAudioSampleRate;

	if (outputProps.ioSelect == IOSelection::Invalid) {
		blog(LOG_DEBUG,
		     "aja_output_create: Select a valid AJA Output IOSelection!");
		return nullptr;
	}
	if (outputProps.videoFormat == NTV2_FORMAT_UNKNOWN ||
	    outputProps.pixelFormat == NTV2_FBF_INVALID) {
		blog(LOG_ERROR,
		     "aja_output_create: Select a valid video and/or pixel format!");
		return nullptr;
	}

	const std::string &ioSelectStr =
		aja::IOSelectionToString(outputProps.ioSelect);

	NTV2OutputDestinations outputDests;
	aja::IOSelectionToOutputDests(outputProps.ioSelect, outputDests);
	if (outputDests.empty()) {
		blog(LOG_ERROR,
		     "No Output Destinations found for IOSelection %s!",
		     ioSelectStr.c_str());
		return nullptr;
	}
	outputProps.outputDest = *outputDests.begin();

	if (!cardEntry->AcquireOutputSelection(outputProps.ioSelect, deviceID,
					       outputID)) {
		blog(LOG_ERROR,
		     "aja_output_create: Error acquiring IOSelection %s for card ID %s",
		     ioSelectStr.c_str(), cardID);
		return nullptr;
	}

	auto ajaOutput = new AJAOutput(card, cardID, outputID,
				       (UWord)cardEntry->GetCardIndex(),
				       deviceID);
	ajaOutput->Initialize(outputProps);
	ajaOutput->ClearVideoQueue();
	ajaOutput->ClearAudioQueue();
	ajaOutput->SetOBSOutput(output);
	ajaOutput->CreateThread(true);

#ifdef AJA_WRITE_DEBUG_WAV
	AJAWavWriterAudioFormat wavFormat;
	wavFormat.channelCount = outputProps.AudioChannels();
	wavFormat.sampleRate = outputProps.audioSampleRate;
	wavFormat.sampleSize = outputProps.AudioSize();
	ajaOutput->mWaveWriter =
		new AJAWavWriter("obs_aja_output.wav", wavFormat);
	ajaOutput->mWaveWriter->open();
#endif

	blog(LOG_INFO, "AJA Output created!");

	return ajaOutput;
}

static void aja_output_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
	blog(LOG_INFO, "AJA Output Update...");
}

static bool aja_output_start(void *data)
{
	blog(LOG_INFO, "Starting AJA Output...");

	auto ajaOutput = (AJAOutput *)data;
	if (!ajaOutput) {
		blog(LOG_ERROR, "aja_output_start: Plugin instance is null!");
		return false;
	}

	const std::string &cardID = ajaOutput->mCardID;
	auto &cardManager = aja::CardManager::Instance();
	cardManager.EnumerateCards();
	auto cardEntry = cardManager.GetCardEntry(cardID);
	if (!cardEntry) {
		blog(LOG_DEBUG,
		     "aja_io_selection_changed: Card Entry not found for %s",
		     cardID.c_str());
		return false;
	}

	CNTV2Card *card = ajaOutput->GetCard();
	if (!card) {
		blog(LOG_ERROR, "aja_output_start: Card instance is null!");
		return false;
	}

	auto outputProps = ajaOutput->GetOutputProps();
	auto audioSystem = outputProps.AudioSystem();
	auto outputDest = outputProps.outputDest;
	auto videoFormat = outputProps.videoFormat;
	auto pixelFormat = outputProps.pixelFormat;

	blog(LOG_INFO,
	     "Output Dest: %s | Audio System: %s | Video Format: %s | Pixel Format: %s",
	     NTV2OutputDestinationToString(outputDest, true).c_str(),
	     NTV2AudioSystemToString(audioSystem, true).c_str(),
	     NTV2VideoFormatToString(videoFormat, false).c_str(),
	     NTV2FrameBufferFormatToString(pixelFormat, true).c_str());

	const NTV2DeviceID deviceID = card->GetDeviceID();

	if (GetIndexForNTV2Channel(outputProps.Channel()) > 0) {
		auto numFramestores = aja::CardNumFramestores(deviceID);
		for (UWord i = 0; i < numFramestores; i++) {
			auto channel = GetNTV2ChannelForIndex(i);
			if (cardEntry->ChannelReady(channel,
						    ajaOutput->mOutputID)) {
				card->SetVideoFormat(videoFormat, false, false,
						     channel);
				card->SetRegisterWriteMode(
					NTV2_REGWRITE_SYNCTOFRAME, channel);
				card->SetFrameBufferFormat(channel,
							   pixelFormat);
			}
		}
	}

	// Configures crosspoint routing on AJA card
	ajaOutput->ClearConnections();
	NTV2XptConnections xpt_cnx;
	if (!aja::Routing::ConfigureOutputRoute(outputProps, NTV2_MODE_DISPLAY,
						card, xpt_cnx)) {
		blog(LOG_ERROR,
		     "aja_output_start: Error configuring output route!");
		return false;
	}
	ajaOutput->CacheConnections(xpt_cnx);
	aja::Routing::ConfigureOutputAudio(outputProps, card);

	const auto &formatDesc = outputProps.FormatDesc();
	struct video_scale_info scaler = {};
	scaler.format = aja::AJAPixelFormatToOBSVideoFormat(pixelFormat);
	scaler.width = formatDesc.GetRasterWidth();
	scaler.height = formatDesc.GetRasterHeight();
	// TODO(paulh): Find out what these scaler params actually do.
	// The colors are off when outputting the frames that OBS sends us.
	// but simply changing these values doesn't seem to have any effect.
	scaler.colorspace = VIDEO_CS_709;
	scaler.range = VIDEO_RANGE_PARTIAL;

	obs_output_set_video_conversion(ajaOutput->GetOBSOutput(), &scaler);

	struct audio_convert_info conversion = {};
	conversion.format = outputProps.AudioFormat();
	conversion.speakers = outputProps.SpeakerLayout();
	conversion.samples_per_sec = outputProps.audioSampleRate;

	obs_output_set_audio_conversion(ajaOutput->GetOBSOutput(), &conversion);

	if (!obs_output_begin_data_capture2(ajaOutput->GetOBSOutput())) {
		blog(LOG_ERROR,
		     "aja_output_start: Begin OBS data capture failed!");
		return false;
	}

	blog(LOG_INFO, "AJA Output started!");

	return true;
}

static void aja_output_stop(void *data, uint64_t ts)
{
	UNUSED_PARAMETER(ts);

	blog(LOG_INFO, "Stopping AJA Output...");

	auto ajaOutput = (AJAOutput *)data;
	if (!ajaOutput) {
		blog(LOG_ERROR, "aja_output_stop: Plugin instance is null!");
		return;
	}
	const std::string &cardID = ajaOutput->mCardID;
	auto &cardManager = aja::CardManager::Instance();
	cardManager.EnumerateCards();
	auto cardEntry = cardManager.GetCardEntry(cardID);
	if (!cardEntry) {
		blog(LOG_ERROR, "aja_output_stop: Card Entry not found for %s",
		     cardID.c_str());
		return;
	}
	CNTV2Card *card = ajaOutput->GetCard();
	if (!card) {
		blog(LOG_ERROR, "aja_output_stop: Card instance is null!");
		return;
	}

	auto outputProps = ajaOutput->GetOutputProps();
	if (!cardEntry->ReleaseOutputSelection(outputProps.ioSelect,
					       card->GetDeviceID(),
					       ajaOutput->mOutputID)) {
		blog(LOG_WARNING,
		     "aja_output_stop: Error releasing IOSelection %s from card ID %s",
		     aja::IOSelectionToString(outputProps.ioSelect).c_str(),
		     cardID.c_str());
	}

	ajaOutput->GenerateTestPattern(outputProps.videoFormat,
				       outputProps.pixelFormat,
				       NTV2_TestPatt_Black,
				       ajaOutput->mWriteCardFrame);

	obs_output_end_data_capture(ajaOutput->GetOBSOutput());
	auto audioSystem = outputProps.AudioSystem();
	card->StopAudioOutput(audioSystem);
	ajaOutput->ClearConnections();

	blog(LOG_INFO, "AJA Output stopped.");
}

static void aja_output_raw_video(void *data, struct video_data *frame)
{
	auto ajaOutput = (AJAOutput *)data;
	if (!ajaOutput)
		return;

	auto outputProps = ajaOutput->GetOutputProps();
	auto rasterBytes = outputProps.FormatDesc().GetTotalRasterBytes();
	ajaOutput->QueueVideoFrame(frame, rasterBytes);
}

static void aja_output_raw_audio(void *data, struct audio_data *frames)
{
	auto ajaOutput = (AJAOutput *)data;
	if (!ajaOutput)
		return;

	auto outputProps = ajaOutput->GetOutputProps();
	auto audioSize = outputProps.AudioSize();
	auto audioBytes = static_cast<ULWord>(frames->frames * audioSize);
	ajaOutput->QueueAudioFrames(frames, audioBytes);
}

static obs_properties_t *aja_output_get_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *device_list = obs_properties_add_list(
		props, kUIPropDevice.id, obs_module_text(kUIPropDevice.text),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_t *output_list = obs_properties_add_list(
		props, kUIPropOutput.id, obs_module_text(kUIPropOutput.text),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_t *vid_fmt_list = obs_properties_add_list(
		props, kUIPropVideoFormatSelect.id,
		obs_module_text(kUIPropVideoFormatSelect.text),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_properties_add_list(props, kUIPropPixelFormatSelect.id,
				obs_module_text(kUIPropPixelFormatSelect.text),
				OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_properties_add_list(props, kUIPropSDITransport.id,
				obs_module_text(kUIPropSDITransport.text),
				OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_properties_add_list(props, kUIPropSDITransport.id,
				obs_module_text(kUIPropSDITransport.text),
				OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_properties_add_list(props, kUIPropSDITransport4K.id,
				obs_module_text(kUIPropSDITransport4K.text),
				OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_properties_add_bool(props, kUIPropAutoStartOutput.id,
				obs_module_text(kUIPropAutoStartOutput.text));

	obs_property_set_modified_callback(vid_fmt_list,
					   aja_video_format_changed);
	obs_property_set_modified_callback(output_list,
					   aja_output_dest_changed);
	obs_property_set_modified_callback2(device_list,
					    aja_output_device_changed, data);
	return props;
}

static const char *aja_output_get_name(void *)
{
	return obs_module_text(kUIPropOutputModule.text);
}

static void aja_output_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, kUIPropOutput.id,
				 static_cast<long long>(IOSelection::Invalid));
	obs_data_set_default_int(
		settings, kUIPropVideoFormatSelect.id,
		static_cast<long long>(kDefaultAJAVideoFormat));
	obs_data_set_default_int(
		settings, kUIPropPixelFormatSelect.id,
		static_cast<long long>(kDefaultAJAPixelFormat));
	obs_data_set_default_int(
		settings, kUIPropSDITransport.id,
		static_cast<long long>(kDefaultAJASDITransport));
	obs_data_set_default_int(
		settings, kUIPropSDITransport4K.id,
		static_cast<long long>(kDefaultAJASDITransport4K));
}

void register_aja_output_info()
{
	struct obs_output_info aja_output_info = {};

	aja_output_info.id = kUIPropOutputModule.id;
	aja_output_info.flags = OBS_OUTPUT_AV;
	aja_output_info.get_name = aja_output_get_name;
	aja_output_info.create = aja_output_create;
	aja_output_info.destroy = aja_output_destroy;
	aja_output_info.start = aja_output_start;
	aja_output_info.stop = aja_output_stop;
	aja_output_info.raw_video = aja_output_raw_video;
	aja_output_info.raw_audio = aja_output_raw_audio;
	aja_output_info.update = aja_output_update;
	aja_output_info.get_defaults = aja_output_defaults;
	aja_output_info.get_properties = aja_output_get_properties;
	obs_register_output(&aja_output_info);
}
