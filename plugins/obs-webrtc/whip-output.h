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
#include <algorithm>
#include <chrono>
#include <condition_variable>

#include <rtc/rtc.hpp>
#include <vector>
#include <rtc/candidate.hpp>

struct videoLayerState {
	uint16_t sequenceNumber;
	uint32_t rtpTimestamp;
	int64_t lastVideoTimestamp;
	uint32_t ssrc;
	std::string rid;
};

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
	void ConfigureAudioTrack(std::string media_stream_id, std::string cname);
	void ConfigureVideoTrack(std::string media_stream_id, std::string cname);
	bool Init();
	bool Setup();
	bool Connect();
	void StartThread();
	void SendDelete();
	void StopThread(bool signal);
	void ParseLinkHeader(std::string linkHeader, std::vector<rtc::IceServer> &iceServers);
	bool FetchIceServersViaOptions(std::vector<rtc::IceServer> &iceServers);
	void Send(void *data, uintptr_t size, uint64_t duration, std::shared_ptr<rtc::Track> track,
		  std::shared_ptr<rtc::RtcpSrReporter> rtcp_sr_reporter);
	void SendTrickleCandidate(const rtc::Candidate &candidate);
	void SendEndOfCandidates();
	void SendTrickleIcePatch(const std::string &sdp_frag);

	obs_output_t *output;

	std::string endpoint_url;
	std::string bearer_token;
	std::string resource_url;
	std::mutex resource_etag_mutex;
	std::string resource_etag;

	std::mutex ice_gathering_mutex;
	std::condition_variable ice_gathering_cv;
	std::atomic<bool> ice_gathering_complete;
	std::atomic<bool> has_first_candidate;
	std::atomic<bool> offer_sent;
	bool has_ice_servers;

	// Trickle ICE support (RFC 8840)
	std::string ice_ufrag;
	std::string ice_pwd;
	std::string first_mid;
	std::vector<rtc::Candidate> pending_candidates; // Queued until POST completes
	std::mutex pending_candidates_mutex;
	std::atomic<bool> post_response_gather_started; // Distinguishes pre-offer vs final gather

	std::atomic<bool> running;

	std::mutex start_stop_mutex;
	std::thread start_stop_thread;

	uint32_t base_ssrc;
	std::shared_ptr<rtc::PeerConnection> peer_connection;
	std::shared_ptr<rtc::Track> audio_track;
	std::shared_ptr<rtc::Track> video_track;
	std::shared_ptr<rtc::RtcpSrReporter> audio_sr_reporter;
	std::shared_ptr<rtc::RtcpSrReporter> video_sr_reporter;

	std::map<obs_encoder_t *, std::shared_ptr<videoLayerState>> videoLayerStates;

	std::atomic<size_t> total_bytes_sent;
	std::atomic<int> connect_time_ms;
	int64_t start_time_ns;
	int64_t last_audio_timestamp;
};

void register_whip_output();
