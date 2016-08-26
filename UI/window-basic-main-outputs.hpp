#pragma once

class OBSBasic;

struct BasicOutputHandler {
	OBSOutput              fileOutput;
	OBSOutput              streamOutput;
	bool                   streamingActive = false;
	bool                   recordingActive = false;
	bool                   delayActive = false;
	OBSBasic               *main;

	OBSSignal              startRecording;
	OBSSignal              stopRecording;
	OBSSignal              startStreaming;
	OBSSignal              stopStreaming;
	OBSSignal              streamDelayStarting;
	OBSSignal              streamStopping;
	OBSSignal              recordStopping;

	inline BasicOutputHandler(OBSBasic *main_) : main(main_) {}

	virtual ~BasicOutputHandler() {};

	virtual bool StartStreaming(obs_service_t *service) = 0;
	virtual bool StartRecording() = 0;
	virtual void StopStreaming() = 0;
	virtual void ForceStopStreaming() = 0;
	virtual void StopRecording() = 0;
	virtual bool StreamingActive() const = 0;
	virtual bool RecordingActive() const = 0;

	virtual void Update() = 0;

	inline bool Active() const
	{
		return streamingActive || recordingActive || delayActive;
	}
};

BasicOutputHandler *CreateSimpleOutputHandler(OBSBasic *main);
BasicOutputHandler *CreateAdvancedOutputHandler(OBSBasic *main);
