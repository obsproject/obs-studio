#pragma once

#include <obs-module.h>
#include <util/curl/curl-helper.h>
#include <util/platform.h>
#include <util/base.h>
#include <util/dstr.h>

#include <string>
#include <atomic>
#include <mutex>
#include <thread>

#include <rtc/rtc.hpp>

class WHIPOutput {
public:
	WHIPOutput(obs_data_t *settings, obs_output_t *output);
	~WHIPOutput();

	bool Start();
	void Stop(bool signal = true);
	void Data(struct encoder_packet *packet);

	inline size_t GetTotalBytes() { return total_bytes_sent; }

	inline int GetConnectTime() { return connect_time_ms; }

private:
	void ConfigureAudioTracks(std::string media_stream_id,
				  std::string cname);
	void ConfigureVideoTrack(std::string media_stream_id,
				 std::string cname);
	bool Init();
	bool Setup();
	bool Connect();
	void StartThread();
	void SendDelete();
	void StopThread(bool signal);

	void Send(void *data, uintptr_t size, uint64_t duration,
		  std::shared_ptr<rtc::Track> track,
		  std::shared_ptr<rtc::RtcpSrReporter> rtcp_sr_reporter);

	class TrackAndRTCP {
	public:
		TrackAndRTCP(std::shared_ptr<rtc::Track> track,
			     std::shared_ptr<rtc::RtcpSrReporter> sr_reporter)
			: track(track),
			  sr_reporter(sr_reporter),
			  last_timestamp(0)
		{
		}

		std::shared_ptr<rtc::Track> track;
		std::shared_ptr<rtc::RtcpSrReporter> sr_reporter;
		int64_t last_timestamp;
	};

	obs_output_t *output;
	bool is_av1;
	int num_audio_tracks;

	std::string endpoint_url;
	std::string bearer_token;
	std::string resource_url;

	std::atomic<bool> running;

	std::mutex start_stop_mutex;
	std::thread start_stop_thread;

	uint32_t base_ssrc;
	std::shared_ptr<rtc::PeerConnection> peer_connection;
	std::vector<std::shared_ptr<TrackAndRTCP>> audio_tracks;
	std::shared_ptr<rtc::Track> video_track;
	std::shared_ptr<rtc::RtcpSrReporter> video_sr_reporter;

	std::atomic<size_t> total_bytes_sent;
	std::atomic<int> connect_time_ms;
	int64_t start_time_ns;
	int64_t last_video_timestamp;
};

void register_whip_output();
