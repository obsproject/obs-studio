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

#include <rtc/rtc.h>

#include "whip-utils.h"

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
	void ConfigureAudioTrack(std::string media_stream_id,
				 std::string cname);
	void ConfigureVideoTrack(std::string media_stream_id,
				 std::string cname);
	bool Init();
	bool Setup();
	bool Connect();
	void StartThread();
	void SendDelete();
	void StopThread(bool signal);

	void Send(void *data, uintptr_t size, uint64_t duration, int track);

	int GetAudioSpecificConfigOffset(uint8_t *extradata, size_t size);
	void LatmWriteStreamMuxConfig(PutBitContext *pb, uint8_t *extradata,
				      size_t size);

	obs_output_t *output;
	bool is_aac;

	std::string endpoint_url;
	std::string bearer_token;
	std::string resource_url;

	std::atomic<bool> running;

	std::mutex start_stop_mutex;
	std::thread start_stop_thread;

	uint32_t base_ssrc;
	int peer_connection;
	int audio_track;
	int video_track;

	std::atomic<size_t> total_bytes_sent;
	std::atomic<int> connect_time_ms;
	int64_t start_time_ns;
	int64_t last_audio_timestamp;
	int64_t last_video_timestamp;
};

void register_whip_output();
