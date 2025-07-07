#pragma once

#include "BasicOutputHandler.hpp"

struct SimpleOutput : BasicOutputHandler {
	OBSEncoder audioStreaming;
	OBSEncoder videoStreaming;

	// Vertical Stream Encoders (Simple Mode)
	OBSEncoder videoStreaming_v;
	// Audio for vertical stream will use main audioStreaming or audioArchive if VOD track is different
	// No separate audio encoder object needed here unless audio processing differs significantly.

	OBSEncoder audioRecording;
	OBSEncoder audioArchive;
	OBSEncoder videoRecording;
	// Vertical Recording Encoders (Simple Mode)
	// OBSEncoder videoRecording_v; // if vertical recording is added
	// OBSEncoder audioRecording_v; // if vertical recording is added
	OBSEncoder audioTrack[MAX_AUDIO_MIXES];

	std::string videoEncoder;
	std::string videoQuality;
	bool usingRecordingPreset = false;
	bool recordingConfigured = false;
	bool ffmpegOutput = false;
	bool lowCPUx264 = false;

	SimpleOutput(OBSBasic *main_);

	int CalcCRF(int crf);

	void UpdateRecordingSettings_x264_crf(int crf);
	void UpdateRecordingSettings_qsv11(int crf, bool av1);
	void UpdateRecordingSettings_nvenc(int cqp);
	void UpdateRecordingSettings_nvenc_hevc_av1(int cqp);
	void UpdateRecordingSettings_amd_cqp(int cqp);
	void UpdateRecordingSettings_apple(int quality);
#ifdef ENABLE_HEVC
	void UpdateRecordingSettings_apple_hevc(int quality);
#endif
	void UpdateRecordingSettings();
	void UpdateRecordingAudioSettings();
	virtual void Update() override;

	void SetupOutputs() override;
	int GetAudioBitrate() const;

	void LoadRecordingPreset_Lossy(const char *encoder);
	void LoadRecordingPreset_Lossless();
	void LoadRecordingPreset();

	void LoadStreamingPreset_Lossy(const char *encoder);

	void UpdateRecording();
	bool ConfigureRecording(bool useReplayBuffer);

	bool IsVodTrackEnabled(obs_service_t *service);
	void SetupVodTrack(obs_service_t *service);

	virtual std::shared_future<void> SetupStreaming(obs_service_t *service,
							SetupStreamingContinuation_t continuation) override;
	virtual bool StartStreaming(obs_service_t *service) override;
	virtual bool StartRecording() override;
	virtual bool StartReplayBuffer() override;
	virtual void StopStreaming(bool force) override;
	virtual void StopRecording(bool force) override;
	virtual void StopReplayBuffer(bool force) override;
	virtual bool StreamingActive() const override;
	virtual bool RecordingActive() const override;
	virtual bool ReplayBufferActive() const override;

	// Vertical Output Control
	virtual bool StartVerticalStreaming(obs_service_t *service) override;
	virtual void StopVerticalStreaming(bool force = false) override;
	virtual bool VerticalStreamingActive() const override;
	// virtual bool StartVerticalRecording() override; // If implementing
	// virtual void StopVerticalRecording(bool force = false) override; // If implementing
	// virtual bool VerticalRecordingActive() const override; // If implementing
};
