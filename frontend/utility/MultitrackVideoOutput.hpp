#pragma once

#include <obs.hpp>
#include <util/config-file.h>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#define NOMINMAX

class QString;
class QWidget;

void StreamStartHandler(void *arg, calldata_t *);
void StreamStopHandler(void *arg, calldata_t *data);

void RecordingStartHandler(void *arg, calldata_t *data);
void RecordingStopHandler(void *arg, calldata_t *);

bool MultitrackVideoDeveloperModeEnabled();

struct MultitrackVideoOutput {
public:
	void PrepareStreaming(QWidget *parent, const char *service_name, obs_service_t *service,
			      const std::optional<std::string> &rtmp_url, const QString &stream_key,
			      const char *audio_encoder_id, std::optional<uint32_t> maximum_aggregate_bitrate,
			      std::optional<uint32_t> maximum_video_tracks, std::optional<std::string> custom_config,
			      obs_data_t *dump_stream_to_file_config, size_t main_audio_mixer,
			      std::optional<size_t> vod_track_mixer, std::optional<bool> use_rtmps);
	signal_handler_t *StreamingSignalHandler();
	void StartedStreaming();
	void StopStreaming();
	bool HandleIncompatibleSettings(QWidget *parent, config_t *config, obs_service_t *service, bool &useDelay,
					bool &enableNewSocketLoop, bool &enableDynBitrate);

	OBSOutputAutoRelease StreamingOutput()
	{
		const std::lock_guard current_lock{current_mutex};
		return current ? obs_output_get_ref(current->output_) : nullptr;
	}

	bool RestartOnError() { return restart_on_error; }

private:
	struct OBSOutputObjects {
		OBSOutputAutoRelease output_;
		std::shared_ptr<obs_encoder_group_t> video_encoder_group_;
		std::vector<OBSEncoderAutoRelease> audio_encoders_;
		OBSServiceAutoRelease multitrack_video_service_;
		OBSSignal start_signal, stop_signal;
	};

	std::optional<OBSOutputObjects> take_current();
	std::optional<OBSOutputObjects> take_current_stream_dump();

	static void ReleaseOnMainThread(std::optional<OBSOutputObjects> objects);

	std::mutex current_mutex;
	std::optional<OBSOutputObjects> current;

	std::mutex current_stream_dump_mutex;
	std::optional<OBSOutputObjects> current_stream_dump;

	bool restart_on_error = false;

	friend void StreamStartHandler(void *arg, calldata_t *data);
	friend void StreamStopHandler(void *arg, calldata_t *data);
	friend void RecordingStartHandler(void *arg, calldata_t *data);
	friend void RecordingStopHandler(void *arg, calldata_t *);
};
