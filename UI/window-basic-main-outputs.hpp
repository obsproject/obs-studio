#pragma once

#include <memory>
#include <string>

#include <QFuture>

#include "qt-helpers.hpp"
#include "multitrack-video-output.hpp"

class OBSBasic;

struct BasicOutputHandler {
	OBSOutputAutoRelease fileOutput;
	OBSOutputAutoRelease streamOutput;
	OBSOutputAutoRelease replayBuffer;
	OBSOutputAutoRelease virtualCam;
	bool streamingActive = false;
	bool recordingActive = false;
	bool delayActive = false;
	bool replayBufferActive = false;
	bool virtualCamActive = false;
	OBSBasic *main;

	std::unique_ptr<MultitrackVideoOutput> multitrackVideo;
	bool multitrackVideoActive = false;

	OBSOutputAutoRelease StreamingOutput() const
	{
		return (multitrackVideo && multitrackVideoActive)
			       ? multitrackVideo->StreamingOutput()
			       : OBSOutputAutoRelease{
					 obs_output_get_ref(streamOutput)};
	}

	obs_view_t *virtualCamView = nullptr;
	video_t *virtualCamVideo = nullptr;
	obs_scene_t *vCamSourceScene = nullptr;
	obs_sceneitem_t *vCamSourceSceneItem = nullptr;

	std::string outputType;
	std::string lastError;

	std::string lastRecordingPath;

	OBSSignal startRecording;
	OBSSignal stopRecording;
	OBSSignal startReplayBuffer;
	OBSSignal stopReplayBuffer;
	OBSSignal startStreaming;
	OBSSignal stopStreaming;
	OBSSignal startVirtualCam;
	OBSSignal stopVirtualCam;
	OBSSignal deactivateVirtualCam;
	OBSSignal streamDelayStarting;
	OBSSignal streamStopping;
	OBSSignal recordStopping;
	OBSSignal recordFileChanged;
	OBSSignal replayBufferStopping;
	OBSSignal replayBufferSaved;

	inline BasicOutputHandler(OBSBasic *main_);

	virtual ~BasicOutputHandler(){};

	virtual FutureHolder<bool> SetupStreaming(obs_service_t *service) = 0;
	virtual bool StartStreaming(obs_service_t *service) = 0;
	virtual bool StartRecording() = 0;
	virtual bool StartReplayBuffer() { return false; }
	virtual bool StartVirtualCam();
	virtual void StopStreaming(bool force = false) = 0;
	virtual void StopRecording(bool force = false) = 0;
	virtual void StopReplayBuffer(bool force = false) { (void)force; }
	virtual void StopVirtualCam();
	virtual bool StreamingActive() const = 0;
	virtual bool RecordingActive() const = 0;
	virtual bool ReplayBufferActive() const { return false; }
	virtual bool VirtualCamActive() const;

	virtual void Update() = 0;
	virtual void SetupOutputs() = 0;

	virtual void UpdateVirtualCamOutputSource();
	virtual void DestroyVirtualCamView();
	virtual void DestroyVirtualCameraScene();

	inline bool Active() const
	{
		return streamingActive || recordingActive || delayActive ||
		       replayBufferActive || virtualCamActive ||
		       multitrackVideoActive;
	}

protected:
	void SetupAutoRemux(const char *&container);
	std::string GetRecordingFilename(const char *path,
					 const char *container, bool noSpace,
					 bool overwrite, const char *format,
					 bool ffmpeg);

	FutureHolder<std::optional<bool>>
	SetupMultitrackVideo(obs_service_t *service,
			     std::string audio_encoder_id,
			     std::optional<size_t> vod_track_mixer);
	OBSDataAutoRelease GenerateMultitrackVideoStreamDumpConfig();
};

BasicOutputHandler *CreateSimpleOutputHandler(OBSBasic *main);
BasicOutputHandler *CreateAdvancedOutputHandler(OBSBasic *main);
