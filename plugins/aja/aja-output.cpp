#include "aja-card-manager.hpp"
#include "aja-common.hpp"
#include "aja-ui-props.hpp"
#include "aja-output.hpp"
#include "aja-routing.hpp"

#include <obs-module.h>

#include <ajabase/common/timer.h>
#include <ajabase/system/systemtime.h>

#include <ajantv2/includes/ntv2card.h>
#include <ajantv2/includes/ntv2devicefeatures.h>

#include <atomic>
#include <stdlib.h>

static constexpr double kAVPrerollSec = 0.1;
static constexpr uint32_t kNumCardFrames = 3;

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

AJAOutput::AJAOutput(CNTV2Card *card, const std::string &cardID,
		     const std::string &outputID, UWord deviceIndex,
		     const NTV2DeviceID deviceID)
	: mCardID{cardID},
	  mOutputID{outputID},
	  mDeviceIndex{deviceIndex},
	  mDeviceID{deviceID},
	  mFrameTimes{},
	  mAudioPrerollSec{kAVPrerollSec},
	  mAudioPrerollBytes{0},
	  mMinVideoQueueSize{0},
	  mMinAudioQueueSize{0},
	  mAudioPlayCursor{0},
	  mAudioWriteCursor{0},
	  mAudioWrapAddress{0},
	  mNumCardFrames{0},
	  mFirstCardFrame{0},
	  mCurrentCardFrame{0},
	  mLastCardFrame{0},
	  mVideoFrameCount{0},
	  mFirstVideoTS{0},
	  mFirstAudioTS{0},
	  mLastVideoTS{0},
	  mLastAudioTS{0},
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
	  mOBSOutput{nullptr}
{
	mVideoQueue = std::make_unique<VideoQueue>();
	mAudioQueue = std::make_unique<AudioQueue>();

#ifdef AJA_OUTPUT_PROFILE
	mVideoQueueTimes = new AJARunTimeAverage(60);
	mAudioQueueTimes = new AJARunTimeAverage(60);
#endif
}

AJAOutput::~AJAOutput()
{
	if (mVideoQueue)
		mVideoQueue.reset();
	if (mAudioQueue)
		mAudioQueue.reset();

#ifdef AJA_OUTPUT_PROFILE
	if (mVideoQueueTimes) {
		delete mVideoQueueTimes;
		mVideoQueueTimes = nullptr;
	}
	if (mAudioQueueTimes) {
		delete mAudioQueueTimes;
		mAudioQueueTimes = nullptr;
	}
#endif
}

CNTV2Card *AJAOutput::GetCard()
{
	return mCard;
}

void AJAOutput::Initialize(const OutputProps &props)
{
	const auto &audioSystem = props.AudioSystem();

	// Store the address to the end of the card's audio buffer.
	mCard->GetAudioWrapAddress(mAudioWrapAddress, audioSystem);

	// Specify the frame indices for the "on-air" frames on the card.
	// Starts at frame index corresponding to the output Channel * numFrames
	calculate_card_frame_indices(kNumCardFrames, mCard->GetDeviceID(),
				     props.Channel(), props.videoFormat,
				     props.pixelFormat);

	mCard->SetOutputFrame(props.Channel(), mCurrentCardFrame);

	mCard->WaitForOutputVerticalInterrupt(props.Channel());

	const auto &cardFrameRate =
		GetNTV2FrameRateFromVideoFormat(props.videoFormat);

	ULWord fpsNum = 0;
	ULWord fpsDen = 0;
	GetFramesPerSecond(cardFrameRate, fpsNum, fpsDen);
	mFrameTimes.cardFrameTime =
		(1000000000ULL / (uint64_t)(fpsNum / fpsDen));
	mFrameTimes.cardFps = (double)(fpsNum / fpsDen);
	mFrameTimes.obsFps = obs_get_active_fps();
	if (mFrameTimes.obsFps < 1.0)
		mFrameTimes.obsFps = 30.0;
	mFrameTimes.obsFrameTime =
		(1000000000ULL / (uint64_t)mFrameTimes.obsFps);

	mMinVideoQueueSize = (uint32_t)(kAVPrerollSec * mFrameTimes.obsFps);

	double prerollSamples =
		mAudioPrerollSec * (double)props.audioSampleRate;
	mAudioPrerollBytes =
		(uint32_t)prerollSamples * (uint32_t)props.AudioSize();
	mMinAudioQueueSize = (uint32_t)prerollSamples / AUDIO_OUTPUT_FRAMES;

	SetOutputProps(props);
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

void AJAOutput::GenerateTestPattern(NTV2VideoFormat vf, NTV2PixelFormat pf,
				    NTV2TestPatternSelect pattern)
{
	NTV2VideoFormat vid_fmt = vf;
	NTV2PixelFormat pix_fmt = pf;

	if (vid_fmt == NTV2_FORMAT_UNKNOWN)
		vid_fmt = NTV2_FORMAT_720p_5994;
	if (pix_fmt == NTV2_FBF_INVALID)
		pix_fmt = kDefaultAJAPixelFormat;

	NTV2FormatDesc fd(vid_fmt, pix_fmt, NTV2_VANCMODE_OFF);

	auto bufSize = fd.GetTotalRasterBytes();

	// Raster size changed, regenerate pattern
	if (bufSize != mTestPattern.size()) {
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

	auto outputChannel = mOutputProps.Channel();

	mCard->SetOutputFrame(outputChannel, mCurrentCardFrame);

	mCard->DMAWriteFrame(
		mCurrentCardFrame,
		reinterpret_cast<ULWord *>(&mTestPattern.data()[0]),
		static_cast<ULWord>(mTestPattern.size()));
}

void AJAOutput::QueueVideoFrame(struct video_data *frame, size_t size)
{
	const std::lock_guard<std::mutex> lock(mVideoLock);

	VideoFrame vf;
	vf.frame = *frame;
	vf.frameNum = mVideoFrameCount;
	vf.size = size;
	vf.frame = *frame;

	if (mVideoQueue->size() > kVideoQueueMaxSize) {
		auto &front = mVideoQueue->front();
		free_video_frame(&front.frame);
		mVideoQueue->pop_front();
	}

	copy_video_data(frame, &vf.frame, size);

	mVideoQueue->push_back(vf);

	if (++mVideoFrameCount > UINT64_MAX)
		mVideoFrameCount = 0;

#ifdef AJA_OUTPUT_PROFILE
	blog(LOG_DEBUG, "Video Q: %zu", mVideoQueue->size());
#endif
}

void AJAOutput::QueueAudioFrames(struct audio_data *frames, size_t size)
{
	const std::lock_guard<std::mutex> lock(mAudioLock);

	AudioFrames af;
	af.frames = *frames;
	af.offset = 0;
	af.size = size;
	af.frames = *frames;

	if (mAudioQueue->size() > kAudioQueueMaxSize) {
		auto &front = mAudioQueue->front();
		free_audio_data(&front.frames);
		mAudioQueue->pop_front();
	}

	copy_audio_data(frames, &af.frames, size);

	mAudioQueue->push_back(af);

#ifdef AJA_OUTPUT_PROFILE
	blog(LOG_DEBUG, "Audio Q: %zu", mAudioQueue->size());
#endif
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

bool AJAOutput::PrerolledAudio()
{
	return (mAudioWriteCursor && mAudioPrerollBytes &&
		mAudioWriteCursor >= mAudioPrerollBytes);
}

bool AJAOutput::HaveEnoughAudio(size_t needAudioSize)
{
	bool ok = false;

	if (mAudioQueue->size() > 0) {
		size_t available = 0;
		for (size_t i = 0; i < mAudioQueue->size(); i++) {
			AudioFrames af = mAudioQueue->at(i);
			available += af.size - af.offset;
			if (available >= needAudioSize) {
				ok = true;
				break;
			}
		}
	}

	return ok;
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
void AJAOutput::DMAAudioFromQueue(NTV2AudioSystem audioSys)
{
	bool restartAudio = false;

	auto &audioFront = mAudioQueue->front();

	size_t samplesLeft = audioFront.size;

	if (mAudioWriteCursor < mAudioPlayCursor &&
	    mAudioWriteCursor + samplesLeft > mAudioPlayCursor) {

		blog(LOG_DEBUG,
		     "AJAOutput::DMAAudioFromQueue: Writing %zu bytes at write cursor %d will overtake play cursor at %d",
		     samplesLeft, mAudioWriteCursor, mAudioPlayCursor);

		restartAudio = true;
	}

	size_t index = 0;
	while (samplesLeft > 0) {
		AudioFrames &af = mAudioQueue->front();

		if (!mFirstAudioTS)
			mFirstAudioTS = af.frames.timestamp;

		mLastAudioTS = af.frames.timestamp;

		if (af.frames.data[0] && af.size > 0) {
			size_t available = af.size - af.offset;
			if (available < samplesLeft) {
				dma_audio_samples(audioSys,
						  (uint32_t *)&af.frames
							  .data[0][af.offset],
						  available);
				free_audio_data(&af.frames);
				mAudioQueue->pop_front();
				index += available;
				samplesLeft -= available;
			} else {
				dma_audio_samples(audioSys,
						  (uint32_t *)&af.frames
							  .data[0][af.offset],
						  samplesLeft);
				af.offset += samplesLeft;
				index += samplesLeft;
				samplesLeft -= samplesLeft;
				if (af.offset == af.size) {
					free_audio_data(&af.frames);
					mAudioQueue->pop_front();
				}
			}
		}
	}

	if (restartAudio) {
		blog(LOG_DEBUG,
		     "AJAOutput::DMAAudioFromQueue: Restarting AJA audio system output");

		if (mAudioPrerollSec < 2.0) {
			mAudioPrerollSec += 0.0333;
			blog(LOG_DEBUG,
			     "AJAOutput::DMAAudioFromQueue: Audio pre-roll is now %f seconds",
			     mAudioPrerollSec);
		}

		double prerollSamples =
			mAudioPrerollSec * (double)mOutputProps.audioSampleRate;

		mAudioPrerollBytes = (uint32_t)prerollSamples *
				     (uint32_t)mOutputProps.AudioSize();

		mCard->StopAudioOutput(audioSys);
		mAudioPlayCursor = 0;
		mAudioWriteCursor = 0;
		mAudioStarted = false;
	}
}

// lock video queue before calling
void AJAOutput::DMAVideoFromQueue()
{
	auto &vf = mVideoQueue->front();

	auto data = vf.frame.data[0];

	auto outputChannel = mOutputProps.Channel();

	increment_card_frame();

	auto result = mCard->DMAWriteFrame(mCurrentCardFrame,
					   reinterpret_cast<ULWord *>(data),
					   (ULWord)vf.size);
	if (!result)
		blog(LOG_DEBUG,
		     "AJAOutput::DMAVideoFromQueue: Failed ot write video frame!");

	result = mCard->SetOutputFrame(outputChannel, mCurrentCardFrame);
	if (!result) {
		blog(LOG_DEBUG,
		     "AJAOutput::DMAVideoFromQueue: Failed to set output frame index %d on card!",
		     mCurrentCardFrame);
	}

	if (!mFirstVideoTS)
		mFirstVideoTS = vf.frame.timestamp;

	mLastVideoTS = vf.frame.timestamp;

	free_video_frame(&vf.frame);
	mVideoQueue->pop_front();
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

	// Reserve N framebuffers in card DRAM.
	if (mFirstCardFrame < totalCardFrames &&
	    (mFirstCardFrame + numFrames) < totalCardFrames) {
		mNumCardFrames = numFrames;
		mCurrentCardFrame = mFirstCardFrame;
		mLastCardFrame = mCurrentCardFrame + numFrames;
	}
	// otherwise just grab 2 frames to ping-pong between
	else {
		mNumCardFrames = 2;
		mCurrentCardFrame = channelIndex * 2;
		mLastCardFrame = mCurrentCardFrame + 2;
	}
}

void AJAOutput::increment_card_frame()
{
	if (++mCurrentCardFrame > mLastCardFrame)
		mCurrentCardFrame = mFirstCardFrame;
}

// Perform DMA of audio samples to AJA card while taking into account wrapping around the
// ends of the card's audio buffer (size set to 4MB in Routing::ConfigureOutputAudio).
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
			if (!result) {
				blog(LOG_DEBUG,
				     "AJAOutput::dma_audio_samples: "
				     "failed to write bytes at end of buffer (address = %d)",
				     mAudioWriteCursor);
			}
		}

		// ...transfer remaining bytes at the front of the card audio buffer.
		if (size - remainingBuffer > 0) {
			result = mCard->DMAWriteAudio(
				audioSys, audioDataRemain, 0,
				(uint32_t)size - remainingBuffer);
			if (!result) {
				blog(LOG_DEBUG,
				     "AJAOutput::dma_audio_samples "
				     "failed to write bytes at front of buffer (address = %d)",
				     mAudioWriteCursor);
			}
		}

		mAudioWriteCursor = (uint32_t)size - remainingBuffer;
	}
	//	No wrap, so just do a linear DMA from the buffer...
	else {
		if (size > 0) {
			result = mCard->DMAWriteAudio(audioSys, data,
						      mAudioWriteCursor,
						      (ULWord)size);
			if (!result) {
				blog(LOG_DEBUG,
				     "AJAOutput::dma_audio_samples "
				     "failed to write bytes to buffer (address = %d)",
				     mAudioWriteCursor);
			}
		}

		mAudioWriteCursor += (uint32_t)size;
	}
}

void AJAOutput::adjust_initial_card_av_sync()
{
	const std::lock_guard<std::mutex> lock(mVideoLock);
	for (size_t vdx = 0; vdx < mVideoQueue->size(); vdx++) {
		auto &vf = mVideoQueue->front();
		if (vf.frame.timestamp <= mLastAudioTS) {
			blog(LOG_DEBUG,
			     "AJAOutput::adjust_initial_card_av_sync: "
			     "Pop video TS %lld (want audio TS %lld)",
			     vf.frame.timestamp, mLastAudioTS);

			free_video_frame(&vf.frame);
			mVideoQueue->pop_front();
		}
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

	uint64_t videoFrameCount = 0;

	AJATimer videoTimer(AJATimerPrecisionNanoseconds);
	AJATimer audioTimer(AJATimerPrecisionNanoseconds);

	uint64_t videoFrameTimeNanos = ajaOutput->mFrameTimes.cardFrameTime;
	if (ajaOutput->mFrameTimes.obsFrameTime >
	    ajaOutput->mFrameTimes.cardFrameTime)
		videoFrameTimeNanos = ajaOutput->mFrameTimes.obsFrameTime;

	uint64_t audioFrameTimeNanos = (1000000000ULL / props.audioSampleRate);

	audioTimer.Start();

	// thread loop
	while (ajaOutput->ThreadRunning()) {
		// Audio Pre-roll
		if (!ajaOutput->mAudioStarted && ajaOutput->PrerolledAudio()) {
			blog(LOG_DEBUG,
			     "AJAOutput::OutputThread: Audio Preroll complete");

			ajaOutput->adjust_initial_card_av_sync();

			card->StartAudioOutput(audioSystem);

			ajaOutput->mAudioStarted = true;

			videoTimer.Start();
		}

		card->ReadAudioLastOut(ajaOutput->mAudioPlayCursor,
				       audioSystem);

		// Audio DMA
		{
			const std::lock_guard<std::mutex> lock(
				ajaOutput->mAudioLock);
			if (audioTimer.ElapsedTime() >= audioFrameTimeNanos) {
				if (ajaOutput->AudioQueueSize() > 0) {
					ajaOutput->DMAAudioFromQueue(
						audioSystem);
				}
				audioTimer.Start();
			}
		}

		// Video DMA
		{
			const std::lock_guard<std::mutex> lock(
				ajaOutput->mVideoLock);
			if (videoTimer.ElapsedTime() >= videoFrameTimeNanos) {
				if (ajaOutput->VideoQueueSize() > 0) {
					ajaOutput->DMAVideoFromQueue();
					videoFrameCount++;
				}
				videoTimer.Start();
			}
		}
	}

	ajaOutput->mAudioStarted = false;

	blog(LOG_INFO,
	     "AJAOutput::OutputThread: Thread stopped. Played %lld video frames",
	     videoFrameCount);
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
	obs_property_t *sdi_4k_list =
		obs_properties_get(props, kUIPropSDI4KTransport.id);

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
	populate_video_format_list(deviceID, vid_fmt_list, videoFormatChannel1);

	obs_property_list_clear(pix_fmt_list);
	populate_pixel_format_list(deviceID, pix_fmt_list);

	obs_property_list_clear(sdi_4k_list);
	populate_sdi_4k_transport_list(sdi_4k_list);

	return true;
}

bool aja_output_dest_changed(obs_properties_t *props, obs_property_t *list,
			     obs_data_t *settings)
{
	UNUSED_PARAMETER(props);

	blog(LOG_DEBUG, "AJA Output Dest Changed");

	const char *cardID = obs_data_get_string(settings, kUIPropDevice.id);

	auto &cardManager = aja::CardManager::Instance();

	cardManager.EnumerateCards();

	auto cardEntry = cardManager.GetCardEntry(cardID);
	if (!cardEntry) {
		blog(LOG_ERROR,
		     "aja_output_dest_changed: Card entry not found for %s",
		     cardID);
		return false;
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
	if (outputProps.ioSelect == IOSelection::Invalid) {
		blog(LOG_DEBUG,
		     "aja_output_create: Select a valid AJA Output IOSelection!");
		return nullptr;
	}

	outputProps.videoFormat = static_cast<NTV2VideoFormat>(
		obs_data_get_int(settings, kUIPropVideoFormatSelect.id));
	outputProps.pixelFormat = static_cast<NTV2PixelFormat>(
		obs_data_get_int(settings, kUIPropPixelFormatSelect.id));
	outputProps.sdi4kTransport = static_cast<SDI4KTransport>(
		obs_data_get_int(settings, kUIPropSDI4KTransport.id));

	outputProps.audioNumChannels = kDefaultAudioChannels;
	outputProps.audioSampleSize = kDefaultAudioSampleSize;
	outputProps.audioSampleRate = kDefaultAudioSampleRate;

	if (NTV2_IS_4K_VIDEO_FORMAT(outputProps.videoFormat) &&
	    outputProps.sdi4kTransport == SDI4KTransport::Squares) {
		if (outputProps.ioSelect == IOSelection::SDI1_2)
			outputProps.ioSelect = IOSelection::SDI1_2_Squares;
		else if (outputProps.ioSelect == IOSelection::SDI3_4)
			outputProps.ioSelect = IOSelection::SDI3_4_Squares;
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
				card->SetFrameBufferFormat(channel,
							   pixelFormat);
			}
		}
	}

	// Configures crosspoint routing on AJA card
	if (!Routing::ConfigureOutputRoute(outputProps, NTV2_MODE_DISPLAY,
					   card)) {
		blog(LOG_ERROR,
		     "aja_output_start: Error configuring output route!");
		return false;
	}

	Routing::ConfigureOutputAudio(outputProps, card);

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

	if (!obs_output_begin_data_capture(ajaOutput->GetOBSOutput(), 0)) {
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

	auto outputProps = ajaOutput->GetOutputProps();
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

	if (!cardEntry->ReleaseOutputSelection(outputProps.ioSelect,
					       card->GetDeviceID(),
					       ajaOutput->mOutputID)) {
		blog(LOG_WARNING,
		     "aja_output_stop: Error releasing IOSelection %s from card ID %s",
		     aja::IOSelectionToString(outputProps.ioSelect).c_str(),
		     cardID.c_str());
	}

	auto audioSystem = outputProps.AudioSystem();

	ajaOutput->GenerateTestPattern(outputProps.videoFormat,
				       outputProps.pixelFormat,
				       NTV2_TestPatt_Black);

	obs_output_end_data_capture(ajaOutput->GetOBSOutput());

	card->StopAudioOutput(audioSystem);

	ajaOutput->mVideoFrameCount = 0;

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

#ifdef AJA_OUTPUT_PROFILE
	// OBS sends video every 1/<OBS BASE FRAMERATE> msec
	if (ajaOutput->mVideoLastTime) {
		uint64_t elapsedTime = AJATime::GetSystemNanoseconds() -
				       ajaOutput->mVideoLastTime;
		uint64_t avgNanos = ajaOutput->mVideoQueueTimes->MarkAverage(
			(int64_t)elapsedTime);
		blog(LOG_DEBUG, "aja_output_raw_video avg: %lld (%fms)",
		     avgNanos, (double)avgNanos / 1000000);
	}

	ajaOutput->mVideoLastTime = AJATime::GetSystemNanoseconds();
#endif
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

#ifdef AJA_OUTPUT_PROFILE
	// OBS sends 1024 audio samples every ~21ms @ 48000hz
	if (ajaOutput->mAudioLastTime) {
		uint64_t elapsedTime = AJATime::GetSystemNanoseconds() -
				       ajaOutput->mAudioLastTime;
		uint64_t avgNanos = ajaOutput->mAudioQueueTimes->MarkAverage(
			(int64_t)elapsedTime);
		blog(LOG_DEBUG, "aja_output_raw_audio avg: %lld (%fms)",
		     avgNanos, (double)avgNanos / 1000000);
	}

	ajaOutput->mAudioLastTime = AJATime::GetSystemNanoseconds();
#endif
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

	obs_properties_add_list(props, kUIPropSDI4KTransport.id,
				obs_module_text(kUIPropSDI4KTransport.text),
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

// NOTE(paulh): Drop-down defaults are set on a clean launch in aja-output-ui code.
// Otherwise we load the settings stored in the ajaOutputProps/ajaPreviewOutputProps.json configs.
void aja_output_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, kUIPropAutoStartOutput.id, false);
}

struct obs_output_info create_aja_output_info()
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
	aja_output_info.get_defaults = aja_output_get_defaults;
	aja_output_info.get_properties = aja_output_get_properties;

	return aja_output_info;
}
