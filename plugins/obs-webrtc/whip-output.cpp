#include "whip-output.h"
#include "whip-utils.h"

#include <array>
#include <regex>
#include <sstream>
#include <obs.hpp>

/*
 * Sets the maximum size for a video fragment. Effective range is
 * 576-1470, with a lower value equating to more packets created,
 * but also better network compatability.
 */
static uint16_t MAX_VIDEO_FRAGMENT_SIZE = 1200;

const int signaling_media_id_length = 16;
const char signaling_media_id_valid_char[] = "0123456789"
					     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					     "abcdefghijklmnopqrstuvwxyz";

const std::string user_agent = generate_user_agent();

const char *audio_mid = "0";
const uint8_t audio_payload_type = 111;

const char *video_mid = "1";
const uint8_t video_payload_type = 96;

// ~3 seconds of 8.5 Megabit video
const int video_nack_buffer_size = 4000;

const std::string rtpHeaderExtUriMid = "urn:ietf:params:rtp-hdrext:sdes:mid";
const std::string rtpHeaderExtUriRid = "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id";

WHIPOutput::WHIPOutput(obs_data_t *, obs_output_t *output)
	: output(output),
	  endpoint_url(),
	  bearer_token(),
	  resource_url(),
	  resource_etag_mutex(),
	  resource_etag(),
	  ice_gathering_mutex(),
	  ice_gathering_cv(),
	  ice_gathering_complete(false),
	  has_first_candidate(false),
	  trickle_enabled(false),
	  has_ice_servers(false),
	  ice_ufrag(),
	  ice_pwd(),
	  trickle_sdp_mutex(),
	  trickle_media_sections(),
	  trickle_bundle_mids(),
	  pending_candidates(),
	  pending_candidates_mutex(),
	  running(false),
	  start_stop_mutex(),
	  start_stop_thread(),
	  base_ssrc(generate_random_u32()),
	  peer_connection(nullptr),
	  audio_track(nullptr),
	  video_track(nullptr),
	  total_bytes_sent(0),
	  connect_time_ms(0),
	  start_time_ns(0),
	  last_audio_timestamp(0)
{
}

WHIPOutput::~WHIPOutput()
{
	Stop();

	std::lock_guard<std::mutex> l(start_stop_mutex);
	if (start_stop_thread.joinable()) {
		start_stop_thread.join();
	}
}

bool WHIPOutput::Start()
{
	std::lock_guard<std::mutex> l(start_stop_mutex);

	for (uint32_t idx = 0; idx < MAX_OUTPUT_VIDEO_ENCODERS; idx++) {
		auto encoder = obs_output_get_video_encoder2(output, idx);
		if (encoder == nullptr) {
			break;
		}

		auto v = std::make_shared<videoLayerState>();
		// base_ssrc is ssrc for audio track. We do `+ 1` for the video, then idx for each Simulcast layer.
		v->ssrc = base_ssrc + 1 + idx;
		v->rid = std::to_string(idx);
		videoLayerStates[encoder] = v;
	}

	if (!obs_output_can_begin_data_capture(output, 0)) {
		return false;
	}
	if (!obs_output_initialize_encoders(output, 0)) {
		return false;
	}

	if (start_stop_thread.joinable()) {
		start_stop_thread.join();
	}
	start_stop_thread = std::thread(&WHIPOutput::StartThread, this);

	return true;
}

void WHIPOutput::Stop(bool signal)
{
	std::lock_guard<std::mutex> l(start_stop_mutex);
	if (start_stop_thread.joinable()) {
		start_stop_thread.join();
	}

	start_stop_thread = std::thread(&WHIPOutput::StopThread, this, signal);
}

void WHIPOutput::Data(struct encoder_packet *packet)
{
	if (!packet) {
		Stop(false);
		obs_output_signal_stop(output, OBS_OUTPUT_ENCODE_ERROR);
		return;
	}

	if (audio_track && packet->type == OBS_ENCODER_AUDIO) {
		int64_t duration = packet->dts_usec - last_audio_timestamp;
		Send(packet->data, packet->size, duration, audio_track, audio_sr_reporter);
		last_audio_timestamp = packet->dts_usec;
	} else if (video_track && packet->type == OBS_ENCODER_VIDEO) {
		auto rtp_config = video_sr_reporter->rtpConfig;
		auto videoLayerState = videoLayerStates[packet->encoder];
		if (videoLayerState == nullptr) {
			Stop(false);
			obs_output_signal_stop(output, OBS_OUTPUT_ENCODE_ERROR);
			return;
		}

		rtp_config->sequenceNumber = videoLayerState->sequenceNumber;
		rtp_config->ssrc = videoLayerState->ssrc;
		rtp_config->rid = videoLayerState->rid;
		rtp_config->timestamp = videoLayerState->rtpTimestamp;
		int64_t duration = packet->dts_usec - videoLayerState->lastVideoTimestamp;

		Send(packet->data, packet->size, duration, video_track, video_sr_reporter);

		videoLayerState->sequenceNumber = rtp_config->sequenceNumber;
		videoLayerState->lastVideoTimestamp = packet->dts_usec;
		videoLayerState->rtpTimestamp = rtp_config->timestamp;
	}
}

void WHIPOutput::ConfigureAudioTrack(std::string media_stream_id, std::string cname)
{
	if (!obs_output_get_audio_encoder(output, 0)) {
		do_log(LOG_DEBUG, "Not configuring audio track: Audio encoder not assigned");
		return;
	}

	auto media_stream_track_id = std::string(media_stream_id + "-audio");

	uint32_t ssrc = base_ssrc;

	rtc::Description::Audio audio_description(audio_mid, rtc::Description::Direction::SendOnly);
	audio_description.addOpusCodec(audio_payload_type);
	audio_description.addSSRC(ssrc, cname, media_stream_id, media_stream_track_id);
	audio_track = peer_connection->addTrack(audio_description);

	auto rtp_config = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, cname, audio_payload_type,
									rtc::OpusRtpPacketizer::DefaultClockRate);
	auto packetizer = std::make_shared<rtc::OpusRtpPacketizer>(rtp_config);
	audio_sr_reporter = std::make_shared<rtc::RtcpSrReporter>(rtp_config);
	auto nack_responder = std::make_shared<rtc::RtcpNackResponder>();

	packetizer->addToChain(audio_sr_reporter);
	packetizer->addToChain(nack_responder);
	audio_track->setMediaHandler(packetizer);
}

void WHIPOutput::ConfigureVideoTrack(std::string media_stream_id, std::string cname)
{
	if (!obs_output_get_video_encoder(output)) {
		do_log(LOG_DEBUG, "Not configuring video track: Video encoder not assigned");
		return;
	}

	auto media_stream_track_id = std::string(media_stream_id + "-video");
	std::shared_ptr<rtc::RtpPacketizer> packetizer;

	// More predictable SSRC values between audio and video
	uint32_t ssrc = base_ssrc + 1;

	rtc::Description::Video video_description(video_mid, rtc::Description::Direction::SendOnly);
	video_description.addSSRC(ssrc, cname, media_stream_id, media_stream_track_id);

	video_description.addExtMap(rtc::Description::Entry::ExtMap(1, rtpHeaderExtUriMid));
	video_description.addExtMap(rtc::Description::Entry::ExtMap(2, rtpHeaderExtUriRid));

	if (videoLayerStates.size() >= 2) {
		std::vector<std::pair<int, std::string>> sortedRids;

		for (const auto &[encoder, state] : videoLayerStates) {
			sortedRids.push_back({std::stoi(state->rid), state->rid});
		}

		std::sort(sortedRids.begin(), sortedRids.end(),
			  [](const auto &a, const auto &b) { return a.first < b.first; });

		for (const auto &[_, rid] : sortedRids) {
			video_description.addRid(rid);
		}
	}

	auto rtp_config = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, cname, video_payload_type,
#if RTC_VERSION_MAJOR == 0 && RTC_VERSION_MINOR > 22 || RTC_VERSION_MAJOR > 0
									rtc::H264RtpPacketizer::ClockRate);
#else
									rtc::H264RtpPacketizer::defaultClockRate);
#endif

	rtp_config->midId = 1;
	rtp_config->ridId = 2;
	rtp_config->mid = video_mid;

	const obs_encoder_t *encoder = obs_output_get_video_encoder2(output, 0);
	if (!encoder) {
		return;
	}

	OBSDataAutoRelease settings = obs_encoder_get_settings(encoder);
	auto video_bitrate = (int)obs_data_get_int(settings, "bitrate");

	const char *codec = obs_encoder_get_codec(encoder);
	if (strcmp("h264", codec) == 0) {
		video_description.addH264Codec(video_payload_type);
		packetizer = std::make_shared<rtc::H264RtpPacketizer>(rtc::H264RtpPacketizer::Separator::StartSequence,
								      rtp_config, MAX_VIDEO_FRAGMENT_SIZE);
#ifdef ENABLE_HEVC
	} else if (strcmp("hevc", codec) == 0) {
		video_description.addH265Codec(video_payload_type);
		packetizer = std::make_shared<rtc::H265RtpPacketizer>(rtc::H265RtpPacketizer::Separator::StartSequence,
								      rtp_config, MAX_VIDEO_FRAGMENT_SIZE);
#endif
	} else if (strcmp("av1", codec) == 0) {
		video_description.addAV1Codec(video_payload_type);
		packetizer = std::make_shared<rtc::AV1RtpPacketizer>(rtc::AV1RtpPacketizer::Packetization::TemporalUnit,
								     rtp_config, MAX_VIDEO_FRAGMENT_SIZE);
	} else {
		do_log(LOG_ERROR, "Video codec not supported: %s", codec);
		return;
	}

	video_sr_reporter = std::make_shared<rtc::RtcpSrReporter>(rtp_config);
	packetizer->addToChain(video_sr_reporter);
	packetizer->addToChain(std::make_shared<rtc::RtcpNackResponder>(video_nack_buffer_size));

	if (video_bitrate != 0) {
		packetizer->addToChain(std::make_shared<rtc::PacingHandler>(static_cast<double>(video_bitrate * 10000),
									    std::chrono::milliseconds(5)));
	}

	video_track = peer_connection->addTrack(video_description);
	video_track->setMediaHandler(packetizer);
}

/**
 * @brief Store connect info provided by the service.
 *
 * @return bool
 */
bool WHIPOutput::Init()
{
	obs_service_t *service = obs_output_get_service(output);
	if (!service) {
		obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
		return false;
	}

	endpoint_url = obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_SERVER_URL);
	if (endpoint_url.empty()) {
		obs_output_signal_stop(output, OBS_OUTPUT_BAD_PATH);
		return false;
	}

	bearer_token = obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_BEARER_TOKEN);

	return true;
}

/**
 * @brief Fetch ICE servers via OPTIONS request to WHIP endpoint.
 *
 * Per WHIP spec, the endpoint may provide STUN/TURN servers via Link headers
 * in response to an OPTIONS request. This allows ICE gathering to begin
 * before the offer is sent, enabling P2P connections behind NAT.
 *
 * @param iceServers Vector to populate with discovered ICE servers
 * @return bool True if request succeeded (even if no ICE servers found)
 */
bool WHIPOutput::FetchIceServersViaOptions(std::vector<rtc::IceServer> &iceServers)
{
	struct curl_slist *headers = nullptr;
	headers = curl_slist_append(headers, "Accept: application/sdp");
	headers = curl_slist_append(headers, user_agent.c_str());

	if (!bearer_token.empty()) {
		auto bearer_token_header = std::string("Authorization: Bearer ") + bearer_token;
		headers = curl_slist_append(headers, bearer_token_header.c_str());
	}

	std::vector<std::string> http_headers;

	CURL *c = curl_easy_init();
	curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(c, CURLOPT_URL, endpoint_url.c_str());
	curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, "OPTIONS");
	curl_easy_setopt(c, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(c, CURLOPT_TIMEOUT, 5L);
	curl_easy_setopt(c, CURLOPT_HEADERFUNCTION, curl_header_function);
	curl_easy_setopt(c, CURLOPT_HEADERDATA, (void *)&http_headers);

	CURLcode res = curl_easy_perform(c);
	curl_easy_cleanup(c);
	curl_slist_free_all(headers);

	if (res != CURLE_OK) {
		do_log(LOG_DEBUG, "OPTIONS request failed: %s (will proceed without pre-configured ICE servers)",
		       curl_easy_strerror(res));
		return false;
	}

	for (auto &http_header : http_headers) {
		auto value = value_for_header("link", http_header);
		if (value.empty())
			continue;

		value = trim_string(value);
		for (auto end = value.find(","); end != std::string::npos; end = value.find(",")) {
			this->ParseLinkHeader(trim_string(value.substr(0, end)), iceServers);
			value = trim_string(value.substr(end + 1));
		}
		if (!value.empty()) {
			this->ParseLinkHeader(value, iceServers);
		}
	}

	if (!iceServers.empty()) {
		do_log(LOG_INFO, "Discovered %zu ICE server(s) via OPTIONS request", iceServers.size());
	}

	return true;
}

/**
 * @brief Set up the PeerConnection and media tracks.
 *
 * @return bool
 */
bool WHIPOutput::Setup()
{
	rtc::Configuration cfg;

	// Fetch ICE servers via OPTIONS request (per WHIP spec section 4.4)
	std::vector<rtc::IceServer> iceServers;
	FetchIceServersViaOptions(iceServers);
	has_ice_servers = !iceServers.empty();
	if (has_ice_servers) {
		cfg.iceServers = iceServers;
	}

#if RTC_VERSION_MAJOR == 0 && RTC_VERSION_MINOR > 20 || RTC_VERSION_MAJOR > 0
	// Enable auto-gathering if we have ICE servers from OPTIONS
	cfg.disableAutoGathering = iceServers.empty();
#endif

	ice_gathering_complete = false;
	has_first_candidate = false;
	trickle_enabled = false;
	trickle_stop = false;
	ice_ufrag.clear();
	ice_pwd.clear();
	resource_url.clear();
	{
		std::lock_guard<std::mutex> lock(resource_etag_mutex);
		resource_etag.clear();
	}
	first_mid.clear();
	{
		std::lock_guard<std::mutex> lock(trickle_sdp_mutex);
		trickle_media_sections.clear();
		trickle_bundle_mids.clear();
	}
	{
		std::lock_guard<std::mutex> lock(pending_candidates_mutex);
		pending_candidates.clear();
	}

	peer_connection = std::make_shared<rtc::PeerConnection>(cfg);

	// Track when we receive our first ICE candidate
	peer_connection->onLocalCandidate([this](rtc::Candidate candidate) {
		{
			std::lock_guard<std::mutex> lock(ice_gathering_mutex);
			if (!has_first_candidate) {
				has_first_candidate = true;
				first_mid = candidate.mid(); // Saved for end-of-candidates signal
				ice_gathering_cv.notify_one();
			}
		}
		// HTTP must not block the PeerConnection callback thread. The worker
		// starts after the answer is applied and sends candidates in order.
		{
			std::lock_guard<std::mutex> lock(pending_candidates_mutex);
			if (!trickle_stop) {
				pending_candidates.push_back(candidate);
			}
		}
		pending_candidates_cv.notify_one();
	});

	// Set up async ICE gathering completion notification
	peer_connection->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
		if (state == rtc::PeerConnection::GatheringState::Complete) {
			{
				std::lock_guard<std::mutex> lock(pending_candidates_mutex);
				ice_gathering_complete = true;
			}
			pending_candidates_cv.notify_one();
		}
	});

	peer_connection->onStateChange([this](rtc::PeerConnection::State state) {
		switch (state) {
		case rtc::PeerConnection::State::New:
			do_log(LOG_INFO, "PeerConnection state is now: New");
			break;
		case rtc::PeerConnection::State::Connecting:
			do_log(LOG_INFO, "PeerConnection state is now: Connecting");
			start_time_ns = os_gettime_ns();
			break;
		case rtc::PeerConnection::State::Connected:
			do_log(LOG_INFO, "PeerConnection state is now: Connected");
			connect_time_ms = (int)((os_gettime_ns() - start_time_ns) / 1000000.0);
			do_log(LOG_INFO, "Connect time: %dms", connect_time_ms.load());
			break;
		case rtc::PeerConnection::State::Disconnected:
			do_log(LOG_INFO, "PeerConnection state is now: Disconnected");
			Stop(false);
			obs_output_signal_stop(output, OBS_OUTPUT_DISCONNECTED);
			break;
		case rtc::PeerConnection::State::Failed:
			do_log(LOG_INFO, "PeerConnection state is now: Failed");
			Stop(false);
			obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
			break;
		case rtc::PeerConnection::State::Closed:
			do_log(LOG_INFO, "PeerConnection state is now: Closed");
			break;
		}
	});

	std::string media_stream_id, cname;
	media_stream_id.reserve(signaling_media_id_length);
	cname.reserve(signaling_media_id_length);

	for (int i = 0; i < signaling_media_id_length; ++i) {
		media_stream_id += signaling_media_id_valid_char[rand() % (sizeof(signaling_media_id_valid_char) - 1)];

		cname += signaling_media_id_valid_char[rand() % (sizeof(signaling_media_id_valid_char) - 1)];
	}

	ConfigureAudioTrack(media_stream_id, cname);
	ConfigureVideoTrack(media_stream_id, cname);

	peer_connection->setLocalDescription();

	return true;
}

// Given a Link header extract URL/Username/Credential and create rtc::IceServer
// <turn:turn.example.net>; username="user"; credential="myPassword";
//
// https://www.ietf.org/archive/id/draft-ietf-wish-whip-13.html#section-4.4
void WHIPOutput::ParseLinkHeader(std::string val, std::vector<rtc::IceServer> &iceServers)
{
	std::string url, username, password, rel;
	const std::regex ice_url_scheme("^<(stun|stuns|turn|turns):", std::regex_constants::icase);

	auto extractUrl = [](std::string input) -> std::string {
		auto head = input.find("<") + 1;
		auto tail = input.find(">");

		if (head == std::string::npos || tail == std::string::npos) {
			return "";
		}
		return input.substr(head, tail - head);
	};

	auto extractValue = [](std::string input) -> std::string {
		auto head = input.find("\"") + 1;
		auto tail = input.find_last_of("\"");

		if (head == std::string::npos || tail == std::string::npos) {
			return "";
		}
		return input.substr(head, tail - head);
	};

	while (true) {
		std::string token = val;
		auto pos = token.find(";");
		if (pos != std::string::npos) {
			token = val.substr(0, pos);
		}
		token = trim_string(token);

		if (std::regex_search(token, ice_url_scheme)) {
			url = extractUrl(token);
		} else if (token.find("rel=") != std::string::npos) {
			rel = extractValue(token);
		} else if (token.find("username=") != std::string::npos) {
			username = extractValue(token);
		} else if (token.find("credential=") != std::string::npos) {
			password = extractValue(token);
		}

		if (pos == std::string::npos) {
			break;
		}
		val.erase(0, pos + 1);
		val = trim_string(val);
	}

	if (!rel.empty()) {
		const std::regex ice_server_rel("(^|\\s)ice-server(\\s|$)", std::regex_constants::icase);
		if (!std::regex_search(rel, ice_server_rel)) {
			return;
		}
	}

	if (url.empty()) {
		return;
	}

	try {
		auto iceServer = rtc::IceServer(url);
		iceServer.username = username;
		iceServer.password = password;
		iceServers.push_back(iceServer);
	} catch (const std::invalid_argument &err) {
		do_log(LOG_WARNING, "Failed to construct ICE Server from %s: %s", val.c_str(), err.what());
	}
}

bool WHIPOutput::Connect()
{
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/sdp");
	if (!bearer_token.empty()) {
		auto bearer_token_header = std::string("Authorization: Bearer ") + bearer_token;
		headers = curl_slist_append(headers, bearer_token_header.c_str());
	}
	// Use the complete candidate list in the WHIP answer. Advertising reverse
	// trickle is unsafe without a channel for candidates arriving after our last PATCH.

	std::string read_buffer;
	std::vector<std::string> http_headers;
	std::string response_etag;

#if RTC_VERSION_MAJOR == 0 && RTC_VERSION_MINOR > 20 || RTC_VERSION_MAJOR > 0
	// Smart waiting: if we have ICE servers, wait for first candidate OR 150ms.
	// This gets us at least host candidates quickly, and likely some STUN
	// candidates too. Any candidates gathered after offer is sent will be
	// trickled via PATCH.
	if (has_ice_servers) {
		std::unique_lock<std::mutex> lock(ice_gathering_mutex);
		if (!has_first_candidate) {
			// 150ms balances latency vs. candidate coverage; typically enough for host + STUN
			auto timeout = std::chrono::milliseconds(150);
			ice_gathering_cv.wait_for(lock, timeout, [this] { return has_first_candidate.load(); });
		}
	}
#endif

	auto offer_sdp = std::string(peer_connection->localDescription().value());

#if defined(_WIN32) || defined(__APPLE__)
	// The Windows/macOS Mbed TLS dependency rejects current Chrome's DTLS ClientHello.
	// VDO.Ninja supports the passive role, so offer active for this endpoint only.
	// Keep normal actpass negotiation for other WHIP services (RFC 9725, 4.4.4).
	CURLU *endpoint = curl_url();
	char *endpoint_host = nullptr;
	if (endpoint && curl_url_set(endpoint, CURLUPART_URL, endpoint_url.c_str(), 0) == CURLUE_OK &&
	    curl_url_get(endpoint, CURLUPART_HOST, &endpoint_host, 0) == CURLUE_OK &&
	    astrcmpi(endpoint_host, "whip.vdo.ninja") == 0) {
		const std::string actpass = "a=setup:actpass\r\n";
		for (auto pos = offer_sdp.find(actpass); pos != std::string::npos; pos = offer_sdp.find(actpass)) {
			offer_sdp.replace(pos, actpass.size(), "a=setup:active\r\n");
		}
	}
	curl_free(endpoint_host);
	curl_url_cleanup(endpoint);
#endif

	// Extract ICE credentials for trickle PATCH requests
	std::regex re_ufrag("a=ice-ufrag:([^\\r\\n]+)");
	std::regex re_pwd("a=ice-pwd:([^\\r\\n]+)");
	std::smatch match;
	if (std::regex_search(offer_sdp, match, re_ufrag)) {
		ice_ufrag = match[1];
	}
	if (std::regex_search(offer_sdp, match, re_pwd)) {
		ice_pwd = match[1];
	}
	UpdateTrickleSdpMetadata(offer_sdp);

#ifdef DEBUG_SDP
	do_log(LOG_DEBUG, "Offer SDP:\n%s", offer_sdp.c_str());
#endif

	// Add user-agent to our requests
	headers = curl_slist_append(headers, user_agent.c_str());

	char error_buffer[CURL_ERROR_SIZE] = {};

	CURL *c = curl_easy_init();
	curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curl_writefunction);
	curl_easy_setopt(c, CURLOPT_WRITEDATA, (void *)&read_buffer);
	curl_easy_setopt(c, CURLOPT_HEADERFUNCTION, curl_header_function);
	curl_easy_setopt(c, CURLOPT_HEADERDATA, (void *)&http_headers);
	curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(c, CURLOPT_URL, endpoint_url.c_str());
	curl_easy_setopt(c, CURLOPT_POST, 1L);
	curl_easy_setopt(c, CURLOPT_COPYPOSTFIELDS, offer_sdp.c_str());
	curl_easy_setopt(c, CURLOPT_TIMEOUT, 8L);
	curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(c, CURLOPT_UNRESTRICTED_AUTH, 1L);
	curl_easy_setopt(c, CURLOPT_ERRORBUFFER, error_buffer);

	auto doCleanup = [&](bool connectFailed) {
		curl_easy_cleanup(c);
		curl_slist_free_all(headers);
		if (connectFailed) {
			obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
		}
	};

	auto displayError = [&](const char *what, const char *errorMessage) {
		struct dstr error_message;
		dstr_init_copy(&error_message, obs_module_text(errorMessage));
		dstr_replace(&error_message, "%1", what);
		obs_output_set_last_error(output, error_message.array);
		dstr_free(&error_message);
	};

	CURLcode res = curl_easy_perform(c);
	if (res != CURLE_OK) {
		do_log(LOG_ERROR, "Connect failed: %s", error_buffer[0] ? error_buffer : curl_easy_strerror(res));
		doCleanup(true);
		return false;
	}

	long response_code;
	curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &response_code);
	if (response_code != 201) {
		do_log(LOG_ERROR, "Connect failed: HTTP endpoint returned response code %ld", response_code);
		doCleanup(false);
		obs_output_signal_stop(output, OBS_OUTPUT_INVALID_STREAM);
		return false;
	}

	if (read_buffer.empty()) {
		do_log(LOG_ERROR, "Connect failed: No data returned from HTTP endpoint request");
		doCleanup(true);
		return false;
	}

	long redirect_count = 0;
	curl_easy_getinfo(c, CURLINFO_REDIRECT_COUNT, &redirect_count);

	std::string last_location_header;
	size_t location_header_count = 0;
	for (auto &http_header : http_headers) {
		auto location_value = value_for_header("location", http_header);
		if (!location_value.empty()) {
			location_header_count++;
			last_location_header = location_value;
		}

		auto etag_value = value_for_header("etag", http_header);
		if (!etag_value.empty()) {
			response_etag = etag_value;
		}
	}

	if (location_header_count < static_cast<size_t>(redirect_count) + 1) {
		do_log(LOG_ERROR, "WHIP server did not provide a resource URL via the Location header");
		doCleanup(true);
		return false;
	}

	CURLU *url_builder = curl_url();

	// Parse Link headers to extract STUN/TURN server configuration URLs
	std::vector<rtc::IceServer> iceServers;
	for (auto &http_header : http_headers) {
		auto value = value_for_header("link", http_header);
		if (value.empty()) {
			continue;
		}

		value = trim_string(value);

		// Parse multiple links separated by ','
		for (auto end = value.find(","); end != std::string::npos; end = value.find(",")) {
			this->ParseLinkHeader(trim_string(value.substr(0, end)), iceServers);
			value = trim_string(value.substr(end + 1));
		}
		if (!value.empty()) {
			this->ParseLinkHeader(value, iceServers);
		}
	}

	// If Location header doesn't start with `http` it is a relative URL.
	// Construct a absolute URL using the host of the effective URL
	if (last_location_header.find("http") != 0) {
		char *effective_url = nullptr;
		curl_easy_getinfo(c, CURLINFO_EFFECTIVE_URL, &effective_url);
		if (effective_url == nullptr) {
			do_log(LOG_ERROR, "Failed to build Resource URL");
			doCleanup(true);
			return false;
		}

		curl_url_set(url_builder, CURLUPART_URL, effective_url, 0);
		curl_url_set(url_builder, CURLUPART_PATH, last_location_header.c_str(), 0);
		curl_url_set(url_builder, CURLUPART_QUERY, "", 0);
	} else {
		curl_url_set(url_builder, CURLUPART_URL, last_location_header.c_str(), 0);
	}

	char *url = nullptr;
	CURLUcode rc = curl_url_get(url_builder, CURLUPART_URL, &url, CURLU_NO_DEFAULT_PORT);
	if (rc) {
		do_log(LOG_ERROR, "WHIP server provided a invalid resource URL via the Location header");
		doCleanup(true);
		return false;
	}

	resource_url = url;
	curl_free(url);
	do_log(LOG_DEBUG, "WHIP Resource URL is: %s", resource_url.c_str());
	curl_url_cleanup(url_builder);
	{
		std::lock_guard<std::mutex> lock(resource_etag_mutex);
		resource_etag = response_etag;
	}
	trickle_enabled = !response_etag.empty();
	if (!trickle_enabled) {
		do_log(LOG_WARNING, "WHIP response did not include ETag; disabling trickle ICE PATCH for this session");
	}

#ifdef DEBUG_SDP
	do_log(LOG_DEBUG, "Answer SDP:\n%s", read_buffer.c_str());
#endif

	auto response = std::string(read_buffer);
	response.erase(0, response.find("v=0"));

	// If we are sending multiple layers assert that the remote accepted them all
	if (videoLayerStates.size() != 1) {
		auto layersAccepted = simulcast_layers_in_answer(response);
		if (videoLayerStates.size() != layersAccepted) {
			do_log(LOG_ERROR, "WHIP only accepted %lu layers", layersAccepted);
			displayError(std::to_string(layersAccepted).c_str(), "Error.SimulcastLayersRejected");
			doCleanup(true);
			return false;
		}
	}

	try {
		rtc::Description answer(response, "answer");
		peer_connection->setRemoteDescription(answer);
	} catch (const std::invalid_argument &err) {
		do_log(LOG_ERROR, "WHIP server responded with invalid SDP: %s", err.what());
		doCleanup(true);
		struct dstr error_message;
		dstr_init_copy(&error_message, obs_module_text("Error.InvalidSDP"));
		dstr_replace(&error_message, "%1", err.what());
		obs_output_set_last_error(output, error_message.array);
		dstr_free(&error_message);
		return false;
	} catch (const std::exception &err) {
		do_log(LOG_ERROR, "Failed to set remote description: %s", err.what());
		doCleanup(true);
		struct dstr error_message;
		dstr_init_copy(&error_message, obs_module_text("Error.NoRemoteDescription"));
		dstr_replace(&error_message, "%1", err.what());
		obs_output_set_last_error(output, error_message.array);
		dstr_free(&error_message);
		return false;
	}
	doCleanup(false);

#if RTC_VERSION_MAJOR == 0 && RTC_VERSION_MINOR > 20 || RTC_VERSION_MAJOR > 0
	// libdatachannel only permits one gathering pass. If OPTIONS did not
	// start it, use the servers from POST; otherwise drain the existing pass.
	if (!has_ice_servers) {
		peer_connection->gatherLocalCandidates(iceServers);
	}
#endif

	return true;
}

void WHIPOutput::StartThread()
{
	if (!Init()) {
		return;
	}

	if (!Setup()) {
		return;
	}

	if (!Connect()) {
		peer_connection->close();
		peer_connection = nullptr;
		audio_track = nullptr;
		video_track = nullptr;
		SendDelete();
		return;
	}

	if (trickle_enabled) {
		trickle_thread = std::thread(&WHIPOutput::TrickleThread, this);
	}
	obs_output_begin_data_capture(output, 0);
	running = true;
}

void WHIPOutput::SendDelete()
{
	if (resource_url.empty()) {
		do_log(LOG_DEBUG, "No resource URL available, not sending DELETE");
		return;
	}

	struct curl_slist *headers = NULL;
	if (!bearer_token.empty()) {
		auto bearer_token_header = std::string("Authorization: Bearer ") + bearer_token;
		headers = curl_slist_append(headers, bearer_token_header.c_str());
	}

	// Add user-agent to our requests
	headers = curl_slist_append(headers, user_agent.c_str());

	char error_buffer[CURL_ERROR_SIZE] = {};

	CURL *c = curl_easy_init();
	curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(c, CURLOPT_URL, resource_url.c_str());
	curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, "DELETE");
	curl_easy_setopt(c, CURLOPT_TIMEOUT, 8L);
	curl_easy_setopt(c, CURLOPT_ERRORBUFFER, error_buffer);

	auto doCleanup = [&]() {
		curl_easy_cleanup(c);
		curl_slist_free_all(headers);
	};

	CURLcode res = curl_easy_perform(c);
	if (res != CURLE_OK) {
		do_log(LOG_WARNING, "DELETE request for resource URL failed: %s",
		       error_buffer[0] ? error_buffer : curl_easy_strerror(res));
		doCleanup();
		return;
	}

	long response_code;
	curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &response_code);
	if (response_code < 200 || response_code >= 300) {
		do_log(LOG_WARNING, "DELETE request for resource URL failed. HTTP Code: %ld", response_code);
		doCleanup();
		return;
	}

	do_log(LOG_DEBUG, "Successfully performed DELETE request for resource URL");
	resource_url.clear();
	{
		std::lock_guard<std::mutex> lock(resource_etag_mutex);
		resource_etag.clear();
	}
	trickle_enabled = false;
	doCleanup();
}

void WHIPOutput::StopThread(bool signal)
{
	StopTrickle();

	if (peer_connection != nullptr) {
		peer_connection->close();
		peer_connection = nullptr;
		audio_track = nullptr;
		video_track = nullptr;
	}

	SendDelete();

	/*
	 * "signal" exists because we have to preserve the "running" state
	 * across reconnect attempts. If we don't emit a signal if
	 * something calls obs_output_stop() and it's reconnecting, you'll
	 * desync the UI, as the output will be "stopped" and not
	 * "reconnecting", but the "stop" signal will have never been
	 * emitted.
	 */
	if (running && signal) {
		obs_output_signal_stop(output, OBS_OUTPUT_SUCCESS);
		running = false;
	}

	total_bytes_sent = 0;
	connect_time_ms = 0;
	start_time_ns = 0;
	last_audio_timestamp = 0;
	videoLayerStates.clear();
}

void WHIPOutput::Send(void *data, uintptr_t size, uint64_t duration, std::shared_ptr<rtc::Track> track,
		      std::shared_ptr<rtc::RtcpSrReporter> rtcp_sr_reporter)
{
	if (track == nullptr || !track->isOpen()) {
		return;
	}

	std::vector<rtc::byte> sample{(rtc::byte *)data, (rtc::byte *)data + size};

	auto rtp_config = rtcp_sr_reporter->rtpConfig;

	// Sample time is in microseconds, we need to convert it to seconds
	auto elapsed_seconds = double(duration) / (1000.0 * 1000.0);

	// Get elapsed time in clock rate
	uint32_t elapsed_timestamp = rtp_config->secondsToTimestamp(elapsed_seconds);

	// Set new timestamp
	rtp_config->timestamp = rtp_config->timestamp + elapsed_timestamp;

#if RTC_VERSION_MAJOR == 0 && RTC_VERSION_MINOR < 23
	// Get elapsed time in clock rate from last RTCP sender report
	auto report_elapsed_timestamp = rtp_config->timestamp - rtcp_sr_reporter->lastReportedTimestamp();

	// Check if last report was at least 1 second ago
	if (rtp_config->timestampToSeconds(report_elapsed_timestamp) > 1) {
		rtcp_sr_reporter->setNeedsToReport();
	}
#endif

	try {
		track->send(sample);
		total_bytes_sent += sample.size();
	} catch (const std::exception &e) {
		do_log(LOG_ERROR, "error: %s ", e.what());
	}
}

void WHIPOutput::UpdateTrickleSdpMetadata(const std::string &offer_sdp)
{
	std::vector<trickleMediaSection> media_sections;
	std::vector<std::string> bundle_mids;
	std::istringstream offer_stream(offer_sdp);
	std::string line;
	size_t current_section = static_cast<size_t>(-1);

	while (std::getline(offer_stream, line)) {
		line = trim_string(line);
		if (line.empty())
			continue;

		if (line.rfind("a=group:BUNDLE ", 0) == 0) {
			std::istringstream mids_stream(line.substr(std::string("a=group:BUNDLE ").size()));
			std::string mid;
			while (mids_stream >> mid) {
				bundle_mids.push_back(mid);
			}
			continue;
		}

		if (line.rfind("m=", 0) == 0) {
			media_sections.push_back({"", line});
			current_section = media_sections.size() - 1;
			continue;
		}

		if (current_section != static_cast<size_t>(-1) && line.rfind("a=mid:", 0) == 0) {
			media_sections[current_section].mid = line.substr(std::string("a=mid:").size());
		}
	}

	if (bundle_mids.empty()) {
		for (const auto &section : media_sections) {
			if (!section.mid.empty()) {
				bundle_mids.push_back(section.mid);
			}
		}
	}

	std::lock_guard<std::mutex> lock(trickle_sdp_mutex);
	trickle_media_sections = std::move(media_sections);
	trickle_bundle_mids = std::move(bundle_mids);
}

bool WHIPOutput::BuildTrickleSdpFragment(const std::string &mid, const std::string &candidate_line,
					 bool end_of_candidates, std::string &sdp_frag)
{
	std::vector<trickleMediaSection> media_sections;
	std::vector<std::string> bundle_mids;
	{
		std::lock_guard<std::mutex> lock(trickle_sdp_mutex);
		media_sections = trickle_media_sections;
		bundle_mids = trickle_bundle_mids;
	}

	if (media_sections.empty()) {
		do_log(LOG_WARNING, "Unable to build trickle SDP fragment: no local media sections available");
		return false;
	}

	trickleMediaSection selected_section = media_sections.front();
	if (!mid.empty()) {
		auto it = std::find_if(media_sections.begin(), media_sections.end(),
				       [&mid](const trickleMediaSection &section) { return section.mid == mid; });
		if (it != media_sections.end()) {
			selected_section = *it;
		}
	}

	if (selected_section.mline.empty()) {
		do_log(LOG_WARNING, "Unable to build trickle SDP fragment: no pseudo m= section available");
		return false;
	}

	if (!bundle_mids.empty()) {
		sdp_frag.append("a=group:BUNDLE ");
		for (size_t i = 0; i < bundle_mids.size(); i++) {
			if (i != 0) {
				sdp_frag.append(" ");
			}
			sdp_frag.append(bundle_mids[i]);
		}
		sdp_frag.append("\r\n");
	}

	sdp_frag.append(selected_section.mline + "\r\n");
	if (!selected_section.mid.empty()) {
		sdp_frag.append("a=mid:" + selected_section.mid + "\r\n");
	}
	sdp_frag.append("a=ice-ufrag:" + ice_ufrag + "\r\n");
	sdp_frag.append("a=ice-pwd:" + ice_pwd + "\r\n");
	if (!candidate_line.empty()) {
		sdp_frag.append("a=" + candidate_line + "\r\n");
	}
	if (end_of_candidates) {
		sdp_frag.append("a=end-of-candidates\r\n");
	}

	return true;
}

void WHIPOutput::SendTrickleIcePatch(const std::string &sdp_frag)
{
	if (!trickle_enabled || trickle_stop) {
		return;
	}

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/trickle-ice-sdpfrag");
	if (!bearer_token.empty()) {
		auto bearer_token_header = std::string("Authorization: Bearer ") + bearer_token;
		headers = curl_slist_append(headers, bearer_token_header.c_str());
	}

	std::string etag;
	{
		std::lock_guard<std::mutex> lock(resource_etag_mutex);
		etag = resource_etag;
	}
	if (!etag.empty()) {
		auto if_match_header = std::string("If-Match: ") + etag;
		headers = curl_slist_append(headers, if_match_header.c_str());
	}
	headers = curl_slist_append(headers, user_agent.c_str());

	std::array<char, CURL_ERROR_SIZE> error_buffer = {};
	std::vector<std::string> http_headers;
	std::string patch_response_body;

	CURL *c = curl_easy_init();
	curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(c, CURLOPT_URL, resource_url.c_str());
	curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, "PATCH");
	curl_easy_setopt(c, CURLOPT_COPYPOSTFIELDS, sdp_frag.c_str());
	curl_easy_setopt(c, CURLOPT_TIMEOUT, 8L);
	curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(c, CURLOPT_UNRESTRICTED_AUTH, 1L);
	curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curl_writefunction);
	curl_easy_setopt(c, CURLOPT_WRITEDATA, (void *)&patch_response_body);
	curl_easy_setopt(c, CURLOPT_HEADERFUNCTION, curl_header_function);
	curl_easy_setopt(c, CURLOPT_HEADERDATA, (void *)&http_headers);
	curl_easy_setopt(c, CURLOPT_ERRORBUFFER, error_buffer.data());
	curl_easy_setopt(c, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(c, CURLOPT_XFERINFODATA, this);
	curl_easy_setopt(
		c, CURLOPT_XFERINFOFUNCTION, +[](void *data, curl_off_t, curl_off_t, curl_off_t, curl_off_t) -> int {
			return static_cast<WHIPOutput *>(data)->trickle_stop ? 1 : 0;
		});

	CURLcode res = curl_easy_perform(c);
	if (trickle_stop) {
		curl_easy_cleanup(c);
		curl_slist_free_all(headers);
		return;
	}
	if (res != CURLE_OK) {
		do_log(LOG_WARNING, "Trickle ICE PATCH failed: %s",
		       error_buffer[0] ? error_buffer.data() : curl_easy_strerror(res));
	} else {
		long response_code = 0;
		curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &response_code);
		if (response_code < 200 || response_code >= 300) {
			do_log(LOG_WARNING, "Trickle ICE PATCH returned HTTP %ld", response_code);
			if (response_code == 412 || response_code == 428) {
				trickle_enabled = false;
				do_log(LOG_WARNING, "Disabling trickle ICE PATCH after HTTP %ld", response_code);
			}
		} else {
			for (auto &http_header : http_headers) {
				auto value = value_for_header("etag", http_header);
				if (!value.empty()) {
					std::lock_guard<std::mutex> lock(resource_etag_mutex);
					resource_etag = value;
				}
			}
			// Apply any remote ICE candidates the server piggybacked on this response
			if (!patch_response_body.empty()) {
				bool has_trickle_body = false;
				for (auto &http_header : http_headers) {
					auto ct = value_for_header("content-type", http_header);
					if (ct.find("application/trickle-ice-sdpfrag") != std::string::npos) {
						has_trickle_body = true;
						break;
					}
				}
				if (has_trickle_body) {
					ApplyIncomingRemoteCandidates(patch_response_body);
				}
			}
		}
	}

	curl_easy_cleanup(c);
	curl_slist_free_all(headers);
}

void WHIPOutput::ApplyIncomingRemoteCandidates(const std::string &sdp_frag)
{
	std::string current_mid;
	std::istringstream stream(sdp_frag);
	std::string line;

	while (std::getline(stream, line)) {
		line = trim_string(line);
		if (line.empty())
			continue;

		if (line.rfind("a=mid:", 0) == 0) {
			current_mid = line.substr(6);
		} else if (line.rfind("a=candidate:", 0) == 0) {
			// rtc::Candidate expects the string without the "a=" prefix
			std::string cand_str = line.substr(2);
			try {
				rtc::Candidate remote_cand(cand_str, current_mid);
				if (peer_connection) {
					peer_connection->addRemoteCandidate(remote_cand);
					do_log(LOG_DEBUG, "Added remote candidate (mid=%s): %s",
					       current_mid.c_str(), cand_str.c_str());
				}
			} catch (const std::exception &e) {
				do_log(LOG_WARNING, "Failed to add remote candidate: %s", e.what());
			}
		}
		// libdatachannel has no API for a remote end-of-candidates indication.
	}
}

void WHIPOutput::TrickleThread()
{
	while (!trickle_stop && trickle_enabled) {
		std::vector<rtc::Candidate> candidates;
		bool complete = false;
		{
			std::unique_lock<std::mutex> lock(pending_candidates_mutex);
			pending_candidates_cv.wait(lock, [this] {
				return trickle_stop || !pending_candidates.empty() || ice_gathering_complete;
			});
			if (trickle_stop) {
				return;
			}
			candidates.swap(pending_candidates);
			complete = ice_gathering_complete;
		}
		for (const auto &candidate : candidates) {
			if (trickle_stop || !trickle_enabled) {
				return;
			}
			SendTrickleCandidate(candidate);
		}
		if (complete) {
			SendEndOfCandidates();
			return;
		}
	}
}

void WHIPOutput::StopTrickle()
{
	{
		std::lock_guard<std::mutex> lock(pending_candidates_mutex);
		trickle_stop = true;
	}
	pending_candidates_cv.notify_one();
	if (trickle_thread.joinable()) {
		trickle_thread.join();
	}
}

void WHIPOutput::SendTrickleCandidate(const rtc::Candidate &candidate)
{
	// Guard: credentials not yet extracted from offer SDP
	if (resource_url.empty() || ice_ufrag.empty() || ice_pwd.empty()) {
		return;
	}

	std::string sdp_frag;
	std::string mid = candidate.mid();
	if (!BuildTrickleSdpFragment(mid, candidate.candidate(), false, sdp_frag)) {
		return;
	}

	do_log(LOG_DEBUG, "Trickle ICE candidate (mid=%s): %s", mid.c_str(), candidate.candidate().c_str());
	SendTrickleIcePatch(sdp_frag);
}

void WHIPOutput::SendEndOfCandidates()
{
	// Guard: credentials not yet extracted from offer SDP
	if (resource_url.empty() || ice_ufrag.empty() || ice_pwd.empty()) {
		return;
	}

	std::string sdp_frag;
	std::string mid;
	{
		std::lock_guard<std::mutex> lock(ice_gathering_mutex);
		mid = first_mid;
	}
	if (!BuildTrickleSdpFragment(mid, "", true, sdp_frag)) {
		return;
	}

	do_log(LOG_DEBUG, "Sending end-of-candidates");
	SendTrickleIcePatch(sdp_frag);
}

void register_whip_output()
{
	const uint32_t base_flags = OBS_OUTPUT_ENCODED | OBS_OUTPUT_SERVICE | OBS_OUTPUT_MULTI_TRACK_AV;

	const char *audio_codecs = "opus";
#ifdef ENABLE_HEVC
	const char *video_codecs = "h264;hevc;av1";
#else
	const char *video_codecs = "h264;av1";
#endif

	struct obs_output_info info = {};
	info.id = "whip_output";
	info.flags = OBS_OUTPUT_AV | base_flags;
	info.get_name = [](void *) -> const char * {
		return obs_module_text("Output.Name");
	};
	info.create = [](obs_data_t *settings, obs_output_t *output) -> void * {
		return new WHIPOutput(settings, output);
	};
	info.destroy = [](void *priv_data) {
		delete static_cast<WHIPOutput *>(priv_data);
	};
	info.start = [](void *priv_data) -> bool {
		return static_cast<WHIPOutput *>(priv_data)->Start();
	};
	info.stop = [](void *priv_data, uint64_t) {
		static_cast<WHIPOutput *>(priv_data)->Stop();
	};
	info.encoded_packet = [](void *priv_data, struct encoder_packet *packet) {
		static_cast<WHIPOutput *>(priv_data)->Data(packet);
	};
	info.get_defaults = [](obs_data_t *) {
	};
	info.get_properties = [](void *) -> obs_properties_t * {
		return obs_properties_create();
	};
	info.get_total_bytes = [](void *priv_data) -> uint64_t {
		return (uint64_t)static_cast<WHIPOutput *>(priv_data)->GetTotalBytes();
	};
	info.get_connect_time_ms = [](void *priv_data) -> int {
		return static_cast<WHIPOutput *>(priv_data)->GetConnectTime();
	};
	info.encoded_video_codecs = video_codecs;
	info.encoded_audio_codecs = audio_codecs;
	info.protocols = "WHIP";

	obs_register_output(&info);

	info.id = "whip_output_video";
	info.flags = OBS_OUTPUT_VIDEO | base_flags;
	info.encoded_audio_codecs = nullptr;
	obs_register_output(&info);

	info.id = "whip_output_audio";
	info.flags = OBS_OUTPUT_AUDIO | base_flags;
	info.encoded_video_codecs = nullptr;
	info.encoded_audio_codecs = audio_codecs;
	obs_register_output(&info);
}
