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
#include <chrono>

extern "C" {
#include "libavcodec/avcodec.h"
}

#include <rtc/rtc.hpp>

class WHEPSource {
public:
	WHEPSource(obs_data_t *settings, obs_source_t *source);
	~WHEPSource();

	obs_properties_t *GetProperties();
	void Update(obs_data_t *settings);
	void MaybeSendPLI();

private:
	bool Init();
	void SetupPeerConnection();
	bool Connect();
	void StartThread();
	void SendDelete();
	void Stop();
	void StopThread();

	AVCodecContext *CreateVideoAVCodecDecoder();
	AVCodecContext *CreateAudioAVCodecDecoder();

	void OnFrameAudio(rtc::binary &msg, const rtc::FrameInfo &frame_info);
	void OnFrameVideo(rtc::binary &msg, const rtc::FrameInfo &frame_info);

	obs_source_t *source = nullptr;

	std::string endpoint_url;
	std::string resource_url;
	std::string bearer_token;

	std::shared_ptr<rtc::PeerConnection> peer_connection;
	std::shared_ptr<rtc::Track> audio_track;
	std::shared_ptr<rtc::Track> video_track;

	std::shared_ptr<AVCodecContext> video_av_codec_context;
	std::shared_ptr<AVCodecContext> audio_av_codec_context;
	std::shared_ptr<AVPacket> av_packet;
	std::shared_ptr<AVFrame> av_frame;

	std::condition_variable stop_cv;
	std::mutex stop_cv_mutex;
	std::mutex start_stop_mutex;
	std::thread start_stop_thread;

	uint64_t last_audio_rtp_timestamp = 0;
	uint64_t last_video_rtp_timestamp = 0;
	uint64_t last_audio_pts = 0;
	uint64_t last_video_pts = 0;

	std::atomic<std::chrono::system_clock::time_point> last_frame;
	std::chrono::system_clock::time_point last_pli;
};

void register_whep_source();
