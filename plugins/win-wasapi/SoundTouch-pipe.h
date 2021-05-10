#pragma once
#include <SoundTouch.h>
#include <memory>
#include <queue>

#define MAX_SAMPLES 48000

struct SoundTouchStreamSettings {
	unsigned sampleRate;
	unsigned channels;
};

struct SoundTouchEffectSettings {
	double newTempo; // [-50, 100]
	double newPitch; // [1-8, 1.0, 1+8]
	double newRate;  // [-50, 100]
};

class ISoundTouchCb {
public:
	virtual ~ISoundTouchCb() {}
	virtual void OnPopAudio(float *data, unsigned framesPerChn,
				uint64_t timestamp) = 0;
};

// It should be accessed in one thread because there is no mutex inner.
class SoundTouchWrapper {
	struct SoundTouchPacketInfo {
		int64_t ts;
		int64_t framesPerChn_input;
		int64_t framesPerChn_output;
	};

public:
	SoundTouchWrapper(ISoundTouchCb *cb);
	virtual ~SoundTouchWrapper();

	void SetupStream(const SoundTouchStreamSettings &stream,
			 const SoundTouchEffectSettings &effect);

	void PushAudio(const float *data, unsigned framesPerChn,
		       uint64_t timestamp);

	void PopAudio();

private:
	bool PushResult(SoundTouchPacketInfo &info);
	void clear();

private:
	ISoundTouchCb *callback;

	soundtouch::SoundTouch soundTouch;
	SoundTouchStreamSettings streamSettings;
	SoundTouchEffectSettings effectSettings;

	std::queue<SoundTouchPacketInfo> infoQueue;
	double outputRatio = 1.0;
	int64_t firstTimestamp = -1;

	std::shared_ptr<float> readBuffer;
	std::shared_ptr<float> saveBuffer;
	unsigned saveBufferLength;
	unsigned saveBufferOffset;
	unsigned savedFramesPerChn;
};
