#pragma once

class OBSBasic;

struct BasicOutputHandler {
	OBSOutput              fileOutput;
	OBSOutput              streamOutput;
	int                    activeRefs = 0;
	OBSBasic               *main;

	inline BasicOutputHandler(OBSBasic *main_) : main(main_) {}

	virtual ~BasicOutputHandler() {};

	virtual bool StartStreaming(obs_service_t *service) = 0;
	virtual bool StartRecording() = 0;
	virtual void StopStreaming() = 0;
	virtual void StopRecording() = 0;
	virtual bool StreamingActive() const = 0;
	virtual bool RecordingActive() const = 0;

	virtual void Update() = 0;

	inline bool Active() const {return !!activeRefs;}
};

BasicOutputHandler *CreateSimpleOutputHandler(OBSBasic *main);
BasicOutputHandler *CreateAdvancedOutputHandler(OBSBasic *main);
