#include "SoundTouch-pipe.h"
#include <obs.h>
#include <assert.h>

SoundTouchWrapper::SoundTouchWrapper(ISoundTouchCb *cb) : callback(cb) {}

SoundTouchWrapper::~SoundTouchWrapper() {}

void SoundTouchWrapper::SetupStream(const SoundTouchStreamSettings &stream,
				    const SoundTouchEffectSettings &effect)
{
	//------------------ clear old catch --------------------
	clear();

	//------------------- init new state -------------------
	streamSettings = stream;
	effectSettings = effect;

	soundTouch.setSampleRate(stream.sampleRate);
	soundTouch.setChannels(stream.channels);

	soundTouch.setTempoChange(effect.newTempo);
	soundTouch.setPitch(effect.newPitch);
	soundTouch.setRateChange(effect.newRate);

	outputRatio = soundTouch.getInputOutputSampleRatio();
	firstTimestamp = -1;

	float *rb = new float[sizeof(float) * MAX_SAMPLES * stream.channels];
	readBuffer = std::shared_ptr<float>(rb, std::default_delete<float[]>());

	savedFramesPerChn = 0;
	saveBufferOffset = 0;
	saveBufferLength = sizeof(float) * MAX_SAMPLES * 10 * stream.channels;
	float *sb = new float[saveBufferLength];
	saveBuffer = std::shared_ptr<float>(sb, std::default_delete<float[]>());
}

void SoundTouchWrapper::PushAudio(const float *data, unsigned framesPerChn,
				  uint64_t timestamp)
{
	if (!data)
		return;

	soundTouch.putSamples(data, framesPerChn);

	if (firstTimestamp < 0) {
		firstTimestamp = timestamp;
	}

	SoundTouchPacketInfo info;
	info.ts = timestamp - firstTimestamp;
	info.framesPerChn_input = framesPerChn;
	info.framesPerChn_output = unsigned(outputRatio * (double)framesPerChn);

	infoQueue.push(info);
}

void SoundTouchWrapper::PopAudio()
{
	if (infoQueue.empty())
		return;

	SoundTouchPacketInfo info = infoQueue.front();

	while (true) {
		unsigned frames = soundTouch.receiveSamples(readBuffer.get(),
							    MAX_SAMPLES);
		if (frames <= 0)
			break;

		unsigned length =
			sizeof(float) * frames * streamSettings.channels;

		if ((length + saveBufferOffset) >= saveBufferLength) {
			assert(false);
			clear();
			return;
		}

		char *dest = (char *)saveBuffer.get() + saveBufferOffset;
		memmove(dest, readBuffer.get(), length);
		saveBufferOffset += length;
		savedFramesPerChn += frames;

		while (info.framesPerChn_output > 0 &&
		       savedFramesPerChn >= info.framesPerChn_output) {
			if (!PushResult(info)) {
				break;
			}
		}
	}
}

bool SoundTouchWrapper::PushResult(SoundTouchPacketInfo &info)
{
	uint64_t timestamp =
		firstTimestamp + uint64_t(outputRatio * double(info.ts));

	callback->OnPopAudio(saveBuffer.get(), info.framesPerChn_output,
			     timestamp);

	if (savedFramesPerChn == info.framesPerChn_output) {
		saveBufferOffset = 0;
		savedFramesPerChn = 0;
	} else {
		unsigned pushSize = sizeof(float) * info.framesPerChn_output *
				    streamSettings.channels;

		saveBufferOffset -= pushSize;
		savedFramesPerChn -= info.framesPerChn_output;

		char *src = (char *)saveBuffer.get() + pushSize;
		char *dest = (char *)saveBuffer.get();
		memmove(dest, src, saveBufferOffset);
	}

	infoQueue.pop();
	if (infoQueue.empty()) {
		memset(&info, 0, sizeof(info));
		return false;
	} else {
		info = infoQueue.front();
		return true;
	}
}

void SoundTouchWrapper::clear()
{
	firstTimestamp = -1;
	soundTouch.clear();

	while (!infoQueue.empty())
		infoQueue.pop();
}
