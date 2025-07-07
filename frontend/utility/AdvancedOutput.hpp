#pragma once

#include "BasicOutputHandler.hpp"

struct AdvancedOutput : BasicOutputHandler {
	OBSEncoder streamAudioEnc;
	OBSEncoder streamArchiveEnc;
	OBSEncoder streamTrack[MAX_AUDIO_MIXES];
	OBSEncoder recordTrack[MAX_AUDIO_MIXES];
	OBSEncoder videoStreaming;
	OBSEncoder videoRecording;

	bool ffmpegOutput;
	bool ffmpegRecording;
	bool useStreamEncoder;
	bool useStreamAudioEncoder;
	bool usesBitrate = false;

	AdvancedOutput(OBSBasic *main_);

	inline void UpdateStreamSettings();
	inline void UpdateRecordingSettings();
	inline void UpdateAudioSettings();
	virtual void Update() override;

	inline std::optional<size_t> VodTrackMixerIdx(obs_service_t *service);
	inline void SetupVodTrack(obs_service_t *service);

	inline void SetupStreaming();
	inline void SetupRecording();
	inline void SetupFFmpeg();
	void SetupOutputs() override;
	int GetAudioBitrate(size_t i, const char *id) const;

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

	bool allowsMultiTrack();
};
