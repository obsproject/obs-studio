// GOOOOOOOOOOOOOOOD
#include "whip-output.h"
#include "whip-utils.h"

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

WHIPOutput::WHIPOutput(obs_data_t *, obs_output_t *output)
	: output(output),
	  endpoint_url(),
	  bearer_token(),
	  resource_url(),
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
	  last_audio_timestamp(0),
	  last_video_timestamp(0)
{
}

WHIPOutput::~WHIPOutput()
{
	Stop();

	std::lock_guard<std::mutex> l(start_stop_mutex);
	if (start_stop_thread.joinable())
		start_stop_thread.join();
}

bool WHIPOutput::Start()
{
	std::lock_guard<std::mutex> l(start_stop_mutex);

	if (!obs_output_can_begin_data_capture(output, 0))
		return false;
	if (!obs_output_initialize_encoders(output, 0))
		return false;

	if (start_stop_thread.joinable())
		start_stop_thread.join();
	start_stop_thread = std::thread(&WHIPOutput::StartThread, this);

	return true;
}

void WHIPOutput::Stop(bool signal)
{
	std::lock_guard<std::mutex> l(start_stop_mutex);
	if (start_stop_thread.joinable())
		start_stop_thread.join();

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
		int64_t duration = packet->dts_usec - last_video_timestamp;
		Send(packet->data, packet->size, duration, video_track, video_sr_reporter);
		last_video_timestamp = packet->dts_usec;
	}
}

void WHIPOutput::ConfigureAudioTrack(std::string media_stream_id, std::string cname)
{
	if (!obs_output_get_audio_encoder(output, 0)) {
		do_log(LOG_INFO, "Not configuring audio track: Audio encoder not assigned");
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
		do_log(LOG_INFO, "Not configuring video track: Video encoder not assigned");
		return;
	}

	auto media_stream_track_id = std::string(media_stream_id + "-video");
	std::shared_ptr<rtc::RtpPacketizer> packetizer;

	// More predictable SSRC values between audio and video
	uint32_t ssrc = base_ssrc + 1;

	rtc::Description::Video video_description(video_mid, rtc::Description::Direction::SendOnly);
	video_description.addSSRC(ssrc, cname, media_stream_id, media_stream_track_id);

	auto rtp_config = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, cname, video_payload_type,
									rtc::H264RtpPacketizer::defaultClockRate);

	const obs_encoder_t *encoder = obs_output_get_video_encoder2(output, 0);
	if (!encoder)
		return;

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
		do_log(LOG_INFO, "Video codec not supported: %s", codec);
		return;
	}

	video_sr_reporter = std::make_shared<rtc::RtcpSrReporter>(rtp_config);
	packetizer->addToChain(video_sr_reporter);
	packetizer->addToChain(std::make_shared<rtc::RtcpNackResponder>(video_nack_buffer_size));

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
 * @brief Set up the PeerConnection and media tracks.
 *
 * @return bool
 */
bool WHIPOutput::Setup()
{
	rtc::Configuration cfg;
	
	// Add default STUN servers if using VDO.Ninja
	if (endpoint_url.find("https://whip.vdo.ninja") == 0) {
		cfg.iceServers = {
			rtc::IceServer("stun:stun.l.google.com:19302"),
			rtc::IceServer("stun:stun.cloudflare.com:3478")
		};
		
		 // non-privileged ports, else 1024 is valid start
		cfg.portRangeBegin = 49152;
		cfg.portRangeEnd = 65535;

		// Look for port if specified by user; specific port easier to whitelist
		size_t port_pos;
		port_pos = endpoint_url.find("?port=");
		if (port_pos == std::string::npos) {
			port_pos = endpoint_url.find("&port=");
		}

		if (port_pos != std::string::npos) {
			std::string port_str = endpoint_url.substr(port_pos + 6);
			size_t end_pos = port_str.find_first_of("&?");
			if (end_pos != std::string::npos) {
				port_str = port_str.substr(0, end_pos);
			}
			
			try {
				int port = std::stoi(port_str);
				if (port >= 1024 && port <= 65535) {
					cfg.portRangeBegin = port;
					cfg.portRangeEnd = port;
				}
			} catch (...) {
				do_log(LOG_WARNING, "Invalid port parameter in URL, using default port range");
			}
		}
	}
	
	peer_connection = std::make_shared<rtc::PeerConnection>(cfg);
	
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

	peer_connection->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
		switch (state) {
		case rtc::PeerConnection::GatheringState::New:
			do_log(LOG_INFO, "ICE gathering state: New");
			break;
		case rtc::PeerConnection::GatheringState::InProgress:
			do_log(LOG_INFO, "ICE gathering state: In Progress");
			break;
		case rtc::PeerConnection::GatheringState::Complete:
			do_log(LOG_INFO, "ICE gathering state: Complete");
			break;
		}
	});

	peer_connection->onIceStateChange([this](rtc::PeerConnection::IceState state) {
		switch (state) {
		case rtc::PeerConnection::IceState::New:
			do_log(LOG_INFO, "ICE state: New");
			break;
		case rtc::PeerConnection::IceState::Checking:
			do_log(LOG_INFO, "ICE state: Checking");
			break;
		case rtc::PeerConnection::IceState::Connected:
			do_log(LOG_INFO, "ICE state: Connected");
			break;
		case rtc::PeerConnection::IceState::Failed:
			do_log(LOG_INFO, "ICE state: Failed");
			break;
		case rtc::PeerConnection::IceState::Disconnected:
			do_log(LOG_INFO, "ICE state: Disconnected");
			break;
		case rtc::PeerConnection::IceState::Closed:
			do_log(LOG_INFO, "ICE state: Closed");
			break;
		}
	});

	peer_connection->onLocalCandidate([this](rtc::Candidate candidate) {
		do_log(LOG_INFO, "Got ICE candidate: %s", candidate.candidate().c_str());
		if (trickle_endpoint.empty()) {
			pending_candidates.push_back(candidate.candidate());
		} else {
			SendTrickleCandidate(candidate.candidate());
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

bool is_ice_server_link(const std::string& rel_value) {
	// Convert to lowercase for case-insensitive comparison
	std::string lower_rel = rel_value;
	std::transform(lower_rel.begin(), lower_rel.end(), lower_rel.begin(), ::tolower);
	
	// Some implementations might use variations of the rel value
	return lower_rel == "ice-server" || 
		   lower_rel == "iceserver" || 
		   lower_rel == "stun" || 
		   lower_rel == "turn";
}

// Given a Link header extract URL/Username/Credential and create rtc::IceServer
// Link: <stun:stun.example.net>; rel="ice-server"
// Link: <turn:turn.example.net?transport=udp>; rel="ice-server";
// 		username="user"; credential="myPassword"; credential-type="password"
// Link: <turn:turn.example.net?transport=tcp>; rel="ice-server";
// 		username="user"; credential="myPassword"; credential-type="password"
// Link: <turns:turn.example.net?transport=tcp>; rel="ice-server";
// 		username="user"; credential="myPassword"; credential-type="password"
//
// draft-ietf-wish-whip-16 (Expires 22 February 2025)
// - https://datatracker.ietf.org/doc/html/draft-ietf-wish-whip#section-4.6
void WHIPOutput::ParseLinkHeader(std::string val, std::vector<rtc::IceServer> &iceServers)
{
	std::string url, username, password;
	bool hasCredentials = false;
	bool hasCredentialType = false;

	auto extractUrl = [](std::string input) -> std::string {
		auto head = input.find("<") + 1;
		auto tail = input.find(">");
		if (head == std::string::npos || tail == std::string::npos) {
			// Try without brackets as some implementations might omit them
			return input;
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

	std::istringstream stream(val);
	std::string part;
	bool foundRel = false;
	
	while (std::getline(stream, part, ';')) {
		part.erase(0, part.find_first_not_of(" \t\r\n"));
		part.erase(part.find_last_not_of(" \t\r\n") + 1);

		if (part.empty()) continue;

		if (part.find("<") != std::string::npos || part.find("http") != std::string::npos) {
			url = extractUrl(part);
			do_log(LOG_INFO, "Found URL in Link header: %s", url.c_str());
		} else if (part.find("rel=") != std::string::npos) {
			auto rel = extractValue(part);
			if (rel == "trickle-ice") {
				if (url[0] == '/') {
					// Use the base URL from resource_url
					size_t pos = resource_url.find("://");
					if (pos != std::string::npos) {
						pos = resource_url.find("/", pos + 3);
						if (pos != std::string::npos) {
							trickle_endpoint = resource_url.substr(0, pos) + url;
						}
					}
				} else if (url.find("http") != 0) {
					trickle_endpoint = resource_url.substr(0, resource_url.find_last_of('/') + 1) + url;
				} else {
					trickle_endpoint = url;
				}
				do_log(LOG_INFO, "Setting trickle endpoint: %s", trickle_endpoint.c_str());
	
				// Send any pending candidates
				for (const auto& candidate : pending_candidates) {
					SendTrickleCandidate(candidate);
				}
				pending_candidates.clear();
			}
		} else if (part.find("username=") != std::string::npos) {
			username = extractValue(part);
			hasCredentials = true;
		} else if (part.find("credential=") != std::string::npos) {
			password = extractValue(part);
			hasCredentials = true;
		} else if (part.find("credential-type=") != std::string::npos) {
			auto type = extractValue(part);
			// Accept both "password" and "token" as valid types, but only use "password"
			hasCredentialType = (type == "password");
			if (type != "password") {
				do_log(LOG_WARNING, "Unsupported credential-type: %s (only 'password' is supported)", type.c_str());
			}
		}
	}

	// Create server if we have a URL and either:
	// 1. It's identified as an ICE server via rel type, or
	// 2. It has credentials (implying it's a TURN server)
	if (!url.empty() && (foundRel || hasCredentials)) {
		try {
			auto server = rtc::IceServer(url);
			if (!username.empty() && !password.empty()) {
				if (!hasCredentialType) {
					do_log(LOG_WARNING, "ICE server has credentials but missing credential-type=password");
				}
				// We'll still try to use the credentials even if credential-type is missing
				// as some implementations might omit it
				server.username = username;
				server.password = password;
			}
			iceServers.push_back(server);
			do_log(LOG_INFO, "Added ICE server: %s", url.c_str());
		} catch (const std::invalid_argument &err) {
			do_log(LOG_WARNING, "Failed to parse ICE server: %s", err.what());
		}
	}
}

void WHIPOutput::OnIceCandidate(const std::string &candidate, const std::string &mid) {
	if (!running)
		return;
		
	do_log(LOG_INFO, "Got ICE candidate: %s", candidate.c_str());
	
	if (trickle_endpoint.empty()) {
		do_log(LOG_WARNING, "No trickle endpoint available");
		return;
	}
	
	SendTrickleCandidate(candidate);
}

bool WHIPOutput::SendTrickleCandidate(const std::string &candidate) {
	if (trickle_endpoint.empty()) {
		do_log(LOG_DEBUG, "No trickle endpoint available");
		return false;
	}
		
	std::stringstream sdp;
	sdp << "a=" << candidate << "\r\n";
	
	CURL *curl = curl_easy_init();
	if (!curl) {
		do_log(LOG_WARNING, "Failed to initialize CURL for trickle candidate");
		return false;
	}
	
	struct curl_slist *headers = nullptr;
	if (!bearer_token.empty()) {
		std::string auth = "Authorization: Bearer " + bearer_token;
		headers = curl_slist_append(headers, auth.c_str());
	}
	
	headers = curl_slist_append(headers, "Content-Type: application/trickle-ice-sdpfrag");
	
	curl_easy_setopt(curl, CURLOPT_URL, trickle_endpoint.c_str());
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sdp.str().c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
	
	CURLcode res = curl_easy_perform(curl);
	
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	
	return res == CURLE_OK;
}

bool WHIPOutput::Connect() {
	std::vector<rtc::IceServer> iceServers;
	std::string read_buffer;
	std::vector<std::string> http_headers;
	
	if (!peer_connection || !peer_connection->localDescription()) {
		do_log(LOG_INFO, "No peer connection or local description available");
		return false;
	}

	// Wait for ICE gathering to complete
	int gather_timeout = 0;
	while (peer_connection->gatheringState() != rtc::PeerConnection::GatheringState::Complete && 
		   gather_timeout < 50) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		gather_timeout++;
	}
	
	std::string initial_offer_sdp = std::string(*peer_connection->localDescription());
	
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/sdp");
	headers = curl_slist_append(headers, "Accept: application/sdp");
	
	if (!bearer_token.empty()) {
		auto bearer_token_header = std::string("Authorization: Bearer ") + bearer_token;
		headers = curl_slist_append(headers, bearer_token_header.c_str());
	}
	headers = curl_slist_append(headers, user_agent.c_str());

	auto cleanup_headers = [headers]() {
		if (headers) curl_slist_free_all(headers);
	};

	std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), curl_easy_cleanup);
	if (!curl) {
		do_log(LOG_INFO, "Failed to initialize CURL");
		cleanup_headers();
		return false;
	}

	char error_buffer[CURL_ERROR_SIZE] = {};
	curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, curl_writefunction);
	curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, (void *)&read_buffer);
	curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION, curl_header_function);
	curl_easy_setopt(curl.get(), CURLOPT_HEADERDATA, (void *)&http_headers);
	curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl.get(), CURLOPT_URL, endpoint_url.c_str());
	curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
	curl_easy_setopt(curl.get(), CURLOPT_COPYPOSTFIELDS, initial_offer_sdp.c_str());
	curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 8L);
	curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl.get(), CURLOPT_UNRESTRICTED_AUTH, 1L);
	curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error_buffer);

	CURLcode res = curl_easy_perform(curl.get());
	if (res != CURLE_OK) {
		do_log(LOG_INFO, "Connect failed: %s", error_buffer[0] ? error_buffer : curl_easy_strerror(res));
		cleanup_headers();
		obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	long response_code;
	char* effective_url = nullptr;
	curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response_code);
	curl_easy_getinfo(curl.get(), CURLINFO_EFFECTIVE_URL, &effective_url);

	if (!(response_code == 200 || response_code == 201)) {
		do_log(LOG_INFO, "Connect failed: HTTP endpoint returned response code %ld", response_code);
		cleanup_headers();
		obs_output_signal_stop(output, OBS_OUTPUT_INVALID_STREAM);
		return false;
	}

	// Set resource URL first
	for (const auto &http_header : http_headers) {
		auto location = value_for_header("location", http_header);
		if (!location.empty()) {
			CURLU *url_builder = curl_url();
			if (location.find("http") != 0 && effective_url) {
				curl_url_set(url_builder, CURLUPART_URL, effective_url, 0);
				curl_url_set(url_builder, CURLUPART_PATH, location.c_str(), 0);
			} else {
				curl_url_set(url_builder, CURLUPART_URL, location.c_str(), 0);
			}
			
			char *url = nullptr;
			CURLUcode rc = curl_url_get(url_builder, CURLUPART_URL, &url, 0);
			if (rc == CURLUE_OK) {
				resource_url = url;
				curl_free(url);
			}
			curl_url_cleanup(url_builder);
			do_log(LOG_INFO, "WHIP Resource URL is: %s", resource_url.c_str());
			break;
		}
	}

	// Now process Link headers
	for (const auto &http_header : http_headers) {
		std::string link_value = value_for_header("link", http_header);
		if (!link_value.empty()) {
			ParseLinkHeader(link_value, iceServers);
		}
	}

	// Process response SDP
	try {
		auto response_str = std::string(read_buffer);
		size_t sdp_start = response_str.find("v=0");
		if (sdp_start == std::string::npos) {
			static const std::vector<std::string> sdp_fields = {"v=", "o=", "s=", "m="};
			for (const auto& field : sdp_fields) {
				sdp_start = response_str.find(field);
				if (sdp_start != std::string::npos) break;
			}
		}

		response_str = response_str.substr(sdp_start);
		do_log(LOG_INFO, "Received SDP answer: %s", response_str.c_str());
		
		rtc::Description answer(response_str, "answer");
		peer_connection->setRemoteDescription(answer);
		return true;
	} catch (const std::exception &err) {
		do_log(LOG_INFO, "Failed to set remote description: %s", err.what());
		return false;
	}
}

void WHIPOutput::TransferCallbacks(std::shared_ptr<rtc::PeerConnection> newConnection) {
	newConnection->onStateChange([this](rtc::PeerConnection::State state) {
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

	newConnection->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
		switch (state) {
		case rtc::PeerConnection::GatheringState::New:
			do_log(LOG_INFO, "ICE gathering state: New");
			break;
		case rtc::PeerConnection::GatheringState::InProgress:
			do_log(LOG_INFO, "ICE gathering state: In Progress");
			break;
		case rtc::PeerConnection::GatheringState::Complete:
			do_log(LOG_INFO, "ICE gathering state: Complete");
			break;
		}
	});

	newConnection->onLocalCandidate([this](rtc::Candidate candidate) {
		std::string typeStr;
		switch(candidate.type()) {
			case rtc::Candidate::Type::Host: typeStr = "host"; break;
			case rtc::Candidate::Type::ServerReflexive: typeStr = "srflx"; break;
			case rtc::Candidate::Type::PeerReflexive: typeStr = "prflx"; break;
			case rtc::Candidate::Type::Relayed: typeStr = "relay"; break;
			default: typeStr = "unknown";
		}
		
		do_log(LOG_INFO, "New local ICE candidate: type=%s address=%s port=%d", 
			   typeStr.c_str(),
			   candidate.address().value_or("unknown").c_str(),
			   candidate.port());
	});
}

void WHIPOutput::TransferAudioTrack(std::shared_ptr<rtc::PeerConnection> newConnection) {
	if (!audio_track) {
		do_log(LOG_INFO, "No audio track to transfer");
		return;
	}

	do_log(LOG_INFO, "Transferring audio track (SSRC: %u)", base_ssrc);
	
	try {
		auto description = audio_track->description();
		audio_track = newConnection->addTrack(description);
		
		if (!audio_track) {
			do_log(LOG_INFO, "Failed to create new audio track");
			return;
		}
		
		auto rtp_config = std::make_shared<rtc::RtpPacketizationConfig>(
			base_ssrc,
			description.mid(),
			audio_payload_type,
			rtc::OpusRtpPacketizer::DefaultClockRate);
			
		auto packetizer = std::make_shared<rtc::OpusRtpPacketizer>(rtp_config);
		audio_sr_reporter = std::make_shared<rtc::RtcpSrReporter>(rtp_config);
		auto nack_responder = std::make_shared<rtc::RtcpNackResponder>();

		packetizer->addToChain(audio_sr_reporter);
		packetizer->addToChain(nack_responder);
		audio_track->setMediaHandler(packetizer);
		do_log(LOG_INFO, "Audio track transfer complete");
	} catch (const std::exception &e) {
		do_log(LOG_INFO, "Failed to set up audio track: %s", e.what());
	}
}

void WHIPOutput::TransferVideoTrack(std::shared_ptr<rtc::PeerConnection> newConnection) {
	if (!video_track) {
		do_log(LOG_INFO, "No video track to transfer");
		return;
	}

	do_log(LOG_INFO, "Transferring video track (SSRC: %u)", base_ssrc + 1);
	auto description = video_track->description();
	video_track = newConnection->addTrack(description);
	
	if (!video_track) {
		do_log(LOG_INFO, "Failed to create new video track");
		return;
	}
	
	auto rtp_config = std::make_shared<rtc::RtpPacketizationConfig>(
		base_ssrc + 1,
		description.mid(),
		video_payload_type,
		rtc::H264RtpPacketizer::defaultClockRate);
		
	const obs_encoder_t *encoder = obs_output_get_video_encoder2(output, 0);
	if (!encoder) {
		do_log(LOG_INFO, "No video encoder found");
		return;
	}

	const char *codec = obs_encoder_get_codec(encoder);
	std::shared_ptr<rtc::RtpPacketizer> packetizer;

	try {
		if (strcmp("h264", codec) == 0) {
			packetizer = std::make_shared<rtc::H264RtpPacketizer>(
				rtc::H264RtpPacketizer::Separator::StartSequence,
				rtp_config, MAX_VIDEO_FRAGMENT_SIZE);
		} else if (strcmp("av1", codec) == 0) {
			packetizer = std::make_shared<rtc::AV1RtpPacketizer>(
				rtc::AV1RtpPacketizer::Packetization::TemporalUnit,
				rtp_config, MAX_VIDEO_FRAGMENT_SIZE);
		} else {
			do_log(LOG_INFO, "Unsupported video codec: %s", codec);
			return;
		}

		if (packetizer) {
			video_sr_reporter = std::make_shared<rtc::RtcpSrReporter>(rtp_config);
			packetizer->addToChain(video_sr_reporter);
			packetizer->addToChain(std::make_shared<rtc::RtcpNackResponder>(video_nack_buffer_size));
			video_track->setMediaHandler(packetizer);
			do_log(LOG_INFO, "Video track transfer complete");
		}
	} catch (const std::exception &e) {
		do_log(LOG_INFO, "Failed to set up video track: %s", e.what());
	}
}

void WHIPOutput::StartThread()
{
	if (!Init())
		return;

	if (!Setup())
		return;

	if (!Connect()) {
		peer_connection->close();
		peer_connection = nullptr;
		audio_track = nullptr;
		video_track = nullptr;
		return;
	}

	obs_output_begin_data_capture(output, 0);
	running = true;
}

void WHIPOutput::SendDelete()
{
	if (resource_url.empty()) {
		do_log(LOG_INFO, "No resource URL available, not sending DELETE");
		return;
	}

	// Set up CURL headers
	struct curl_slist *headers = NULL;
	if (!bearer_token.empty()) {
		auto bearer_token_header = std::string("Authorization: Bearer ") + bearer_token;
		headers = curl_slist_append(headers, bearer_token_header.c_str());
	}

	// Add user-agent to our requests
	headers = curl_slist_append(headers, user_agent.c_str());

	// Use RAII for headers cleanup
	auto cleanup_headers = [headers]() {
		if (headers) {
			curl_slist_free_all(headers);
		}
	};

	char error_buffer[CURL_ERROR_SIZE] = {};

	// Use RAII for CURL cleanup
	std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), curl_easy_cleanup);
	if (!curl) {
		do_log(LOG_INFO, "Failed to initialize CURL for DELETE request");
		cleanup_headers();
		return;
	}

	curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl.get(), CURLOPT_URL, resource_url.c_str());
	curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, "DELETE");
	curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 8L);
	curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error_buffer);

	CURLcode res = curl_easy_perform(curl.get());
	if (res != CURLE_OK) {
		do_log(LOG_WARNING, "DELETE request for resource URL failed: %s",
			   error_buffer[0] ? error_buffer : curl_easy_strerror(res));
		cleanup_headers();
		return;
	}

	long response_code;
	curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response_code);
	if (!(response_code >= 200 && response_code < 300)) {
		if (response_code == 401 || response_code == 403) {
			do_log(LOG_INFO, "Authentication failed: HTTP endpoint returned %ld", response_code);
			obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
		} else if (response_code >= 500) {
			do_log(LOG_INFO, "Server error: HTTP endpoint returned %ld", response_code);
			obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
		} else {
			do_log(LOG_INFO, "Connect failed: HTTP endpoint returned response code %ld", response_code);
			obs_output_signal_stop(output, OBS_OUTPUT_INVALID_STREAM);
		}
		cleanup_headers();
		return;
	}

	do_log(LOG_INFO, "Successfully performed DELETE request for resource URL");
	resource_url.clear();
	cleanup_headers();
}

void WHIPOutput::StopThread(bool signal)
{
	if (peer_connection) {
		do_log(LOG_INFO, "Closing peer connection");
		peer_connection->close();
		peer_connection = nullptr;
	}

	audio_track = nullptr;
	video_track = nullptr;

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
	last_video_timestamp = 0;
}

void WHIPOutput::Send(void *data, uintptr_t size, uint64_t duration, 
					 std::shared_ptr<rtc::Track> track,
					 std::shared_ptr<rtc::RtcpSrReporter> rtcp_sr_reporter)
{
	if (!track || !rtcp_sr_reporter) {
		return;
	}

	if (!track->isOpen()) {
		static bool logged_closed = false;
		if (!logged_closed) {
			do_log(LOG_INFO, "Track is not open");
			logged_closed = true;
		}
		return;
	}

	if (!peer_connection || 
		peer_connection->state() != rtc::PeerConnection::State::Connected) {
		static bool logged_not_connected = false;
		if (!logged_not_connected) {
			do_log(LOG_INFO, "PeerConnection not ready");
			logged_not_connected = true;
		}
		return;
	}
	
	auto rtp_config = rtcp_sr_reporter->rtpConfig;

	// Sample time is in microseconds, we need to convert it to seconds
	auto elapsed_seconds = double(duration) / (1000.0 * 1000.0);

	// Get elapsed time in clock rate
	uint32_t elapsed_timestamp = rtp_config->secondsToTimestamp(elapsed_seconds);

	// Set new timestamp
	rtp_config->timestamp = rtp_config->timestamp + elapsed_timestamp;

	// Get elapsed time in clock rate from last RTCP sender report
	auto report_elapsed_timestamp = rtp_config->timestamp - rtcp_sr_reporter->lastReportedTimestamp();

	// Check if last report was at least 1 second ago
	if (rtp_config->timestampToSeconds(report_elapsed_timestamp) > 1)
		rtcp_sr_reporter->setNeedsToReport();

	try {
		std::vector<rtc::byte> sample{(rtc::byte *)data, (rtc::byte *)data + size};
		track->send(sample);
		total_bytes_sent += sample.size();

		static uint64_t last_log = 0;
		uint64_t now = os_gettime_ns();
		if (now - last_log > 10000000000) { // Every 10 seconds
			do_log(LOG_INFO, "Media stats: sent=%zu bytes, connected=%d", 
				   total_bytes_sent.load(),
				   peer_connection->state() == rtc::PeerConnection::State::Connected);
			last_log = now;
		}
	} catch (const std::exception &e) {
		do_log(LOG_INFO, "Failed to send media: %s", e.what());
	}
}

void register_whip_output()
{
	const uint32_t base_flags = OBS_OUTPUT_ENCODED | OBS_OUTPUT_SERVICE;

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
		return (uint64_t) static_cast<WHIPOutput *>(priv_data)->GetTotalBytes();
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
