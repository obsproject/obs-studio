#include "whip-output.h"
#include "webrtc-utils.h"

#define do_log(level, format, ...)                              \
	blog(level, "[obs-webrtc] [whip_output: '%s'] " format, \
	     obs_output_get_name(output), ##__VA_ARGS__)

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
		Send(packet->data, packet->size, duration, audio_track,
		     audio_sr_reporter);
		last_audio_timestamp = packet->dts_usec;
	} else if (video_track && packet->type == OBS_ENCODER_VIDEO) {
		int64_t duration = packet->dts_usec - last_video_timestamp;
		Send(packet->data, packet->size, duration, video_track,
		     video_sr_reporter);
		last_video_timestamp = packet->dts_usec;
	}
}

void WHIPOutput::ConfigureAudioTrack(std::string media_stream_id,
				     std::string cname)
{
	if (!obs_output_get_audio_encoder(output, 0)) {
		do_log(LOG_DEBUG,
		       "Not configuring audio track: Audio encoder not assigned");
		return;
	}

	auto media_stream_track_id = std::string(media_stream_id + "-audio");

	uint32_t ssrc = base_ssrc;

	rtc::Description::Audio audio_description(
		audio_mid, rtc::Description::Direction::SendOnly);
	audio_description.addOpusCodec(audio_payload_type);
	audio_description.addSSRC(ssrc, cname, media_stream_id,
				  media_stream_track_id);
	audio_track = peer_connection->addTrack(audio_description);

	auto rtp_config = std::make_shared<rtc::RtpPacketizationConfig>(
		ssrc, cname, audio_payload_type,
		rtc::OpusRtpPacketizer::DefaultClockRate);
	auto packetizer = std::make_shared<rtc::OpusRtpPacketizer>(rtp_config);
	audio_sr_reporter = std::make_shared<rtc::RtcpSrReporter>(rtp_config);
	auto nack_responder = std::make_shared<rtc::RtcpNackResponder>();

	packetizer->addToChain(audio_sr_reporter);
	packetizer->addToChain(nack_responder);
	audio_track->setMediaHandler(packetizer);
}

void WHIPOutput::ConfigureVideoTrack(std::string media_stream_id,
				     std::string cname)
{
	if (!obs_output_get_video_encoder(output)) {
		do_log(LOG_DEBUG,
		       "Not configuring video track: Video encoder not assigned");
		return;
	}

	auto media_stream_track_id = std::string(media_stream_id + "-video");
	std::shared_ptr<rtc::RtpPacketizer> packetizer;

	// More predictable SSRC values between audio and video
	uint32_t ssrc = base_ssrc + 1;

	rtc::Description::Video video_description(
		video_mid, rtc::Description::Direction::SendOnly);
	video_description.addSSRC(ssrc, cname, media_stream_id,
				  media_stream_track_id);

	auto rtp_config = std::make_shared<rtc::RtpPacketizationConfig>(
		ssrc, cname, video_payload_type,
		rtc::H264RtpPacketizer::defaultClockRate);

	const obs_encoder_t *encoder = obs_output_get_video_encoder2(output, 0);
	if (!encoder)
		return;

	const char *codec = obs_encoder_get_codec(encoder);
	if (strcmp("h264", codec) == 0) {
		video_description.addH264Codec(video_payload_type);
		packetizer = std::make_shared<rtc::H264RtpPacketizer>(
			rtc::H264RtpPacketizer::Separator::StartSequence,
			rtp_config, MAX_VIDEO_FRAGMENT_SIZE);
#ifdef ENABLE_HEVC
	} else if (strcmp("hevc", codec) == 0) {
		video_description.addH265Codec(video_payload_type);
		packetizer = std::make_shared<rtc::H265RtpPacketizer>(
			rtc::H265RtpPacketizer::Separator::StartSequence,
			rtp_config, MAX_VIDEO_FRAGMENT_SIZE);
#endif
	} else if (strcmp("av1", codec) == 0) {
		video_description.addAV1Codec(video_payload_type);
		packetizer = std::make_shared<rtc::AV1RtpPacketizer>(
			rtc::AV1RtpPacketizer::Packetization::TemporalUnit,
			rtp_config, MAX_VIDEO_FRAGMENT_SIZE);
	} else {
		do_log(LOG_ERROR, "Video codec not supported: %s", codec);
		return;
	}

	video_sr_reporter = std::make_shared<rtc::RtcpSrReporter>(rtp_config);
	packetizer->addToChain(video_sr_reporter);
	packetizer->addToChain(std::make_shared<rtc::RtcpNackResponder>(
		video_nack_buffer_size));

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

	endpoint_url = obs_service_get_connect_info(
		service, OBS_SERVICE_CONNECT_INFO_SERVER_URL);
	if (endpoint_url.empty()) {
		obs_output_signal_stop(output, OBS_OUTPUT_BAD_PATH);
		return false;
	}

	bearer_token = obs_service_get_connect_info(
		service, OBS_SERVICE_CONNECT_INFO_BEARER_TOKEN);

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

#if RTC_VERSION_MAJOR == 0 && RTC_VERSION_MINOR > 20 || RTC_VERSION_MAJOR > 1
	cfg.disableAutoGathering = true;
#endif

	peer_connection = std::make_shared<rtc::PeerConnection>(cfg);

	peer_connection->onStateChange([this](rtc::PeerConnection::State state) {
		switch (state) {
		case rtc::PeerConnection::State::New:
			do_log(LOG_INFO, "PeerConnection state is now: New");
			break;
		case rtc::PeerConnection::State::Connecting:
			do_log(LOG_INFO,
			       "PeerConnection state is now: Connecting");
			start_time_ns = os_gettime_ns();
			break;
		case rtc::PeerConnection::State::Connected:
			do_log(LOG_INFO,
			       "PeerConnection state is now: Connected");
			connect_time_ms =
				(int)((os_gettime_ns() - start_time_ns) /
				      1000000.0);
			do_log(LOG_INFO, "Connect time: %dms",
			       connect_time_ms.load());
			break;
		case rtc::PeerConnection::State::Disconnected:
			do_log(LOG_INFO,
			       "PeerConnection state is now: Disconnected");
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
		media_stream_id += signaling_media_id_valid_char
			[rand() % (sizeof(signaling_media_id_valid_char) - 1)];

		cname += signaling_media_id_valid_char
			[rand() % (sizeof(signaling_media_id_valid_char) - 1)];
	}

	ConfigureAudioTrack(media_stream_id, cname);
	ConfigureVideoTrack(media_stream_id, cname);

	peer_connection->setLocalDescription();

	return true;
}

void WHIPOutput::StartThread()
{
	if (!Init())
		return;

	if (!Setup())
		return;

	char error_buffer[CURL_ERROR_SIZE] = {};
	CURLcode curl_code;
	auto status = send_offer(bearer_token, endpoint_url, peer_connection,
				 resource_url, &curl_code, error_buffer);

	if (status != webrtc_network_status::Success) {
		if (status == webrtc_network_status::ConnectFailed) {
			do_log(LOG_WARNING, "Connect failed: %s",
			       error_buffer[0] ? error_buffer
					       : curl_easy_strerror(curl_code));
		} else if (status ==
			   webrtc_network_status::InvalidHTTPStatusCode) {
			do_log(LOG_ERROR,
			       "Connect failed: HTTP endpoint returned non-201 response code");
		} else if (status == webrtc_network_status::NoHTTPData) {
			do_log(LOG_ERROR,
			       "Connect failed: No data returned from HTTP endpoint request");
		} else if (status == webrtc_network_status::NoLocationHeader) {
			do_log(LOG_ERROR,
			       "WHIP server did not provide a resource URL via the Location header");
		} else if (status ==
			   webrtc_network_status::FailedToBuildResourceURL) {
			do_log(LOG_ERROR, "Failed to build Resource URL");
		} else if (status ==
			   webrtc_network_status::InvalidLocationHeader) {
			do_log(LOG_ERROR,
			       "WHIP server provided a invalid resource URL via the Location header");
		} else if (status == webrtc_network_status::InvalidAnswer) {
			do_log(LOG_WARNING,
			       "WHIP server responded with invalid SDP: %s",
			       error_buffer);

			struct dstr error_message;
			dstr_init_copy(&error_message,
				       obs_module_text("Error.InvalidSDP"));
			dstr_replace(&error_message, "%1", error_buffer);
			obs_output_set_last_error(output, error_message.array);
			dstr_free(&error_message);
		} else if (status ==
			   webrtc_network_status::SetRemoteDescriptionFailed) {
			do_log(LOG_WARNING,
			       "Failed to set remote description: %s",
			       error_buffer);

			struct dstr error_message;
			dstr_init_copy(
				&error_message,
				obs_module_text("Error.NoRemoteDescription"));
			dstr_replace(&error_message, "%1", error_buffer);
			obs_output_set_last_error(output, error_message.array);
			dstr_free(&error_message);
		}

		obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);

		peer_connection->close();
		peer_connection = nullptr;
		audio_track = nullptr;
		video_track = nullptr;
		return;
	}

	do_log(LOG_DEBUG, "WHIP Resource URL is: %s", resource_url.c_str());
	obs_output_begin_data_capture(output, 0);
	running = true;
}

void WHIPOutput::SendDelete()
{
	if (resource_url.empty()) {
		do_log(LOG_DEBUG,
		       "No resource URL available, not sending DELETE");
		return;
	}

	char error_buffer[CURL_ERROR_SIZE] = {};
	CURLcode curl_code;
	auto status = send_delete(bearer_token, resource_url, &curl_code,
				  error_buffer);
	if (status == webrtc_network_status::Success) {
		do_log(LOG_DEBUG,
		       "Successfully performed DELETE request for resource URL");
		resource_url.clear();
	} else if (status == webrtc_network_status::DeleteFailed) {
		do_log(LOG_WARNING,
		       "DELETE request for resource URL failed. Reason: %s",
		       error_buffer[0] ? error_buffer
				       : curl_easy_strerror(curl_code));
	} else if (status == webrtc_network_status::InvalidHTTPStatusCode) {
		do_log(LOG_WARNING,
		       "DELETE request for resource URL returned non-200 Status Code");
	}
}

void WHIPOutput::StopThread(bool signal)
{
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
	last_video_timestamp = 0;
}

void WHIPOutput::Send(void *data, uintptr_t size, uint64_t duration,
		      std::shared_ptr<rtc::Track> track,
		      std::shared_ptr<rtc::RtcpSrReporter> rtcp_sr_reporter)
{
	if (track == nullptr || !track->isOpen())
		return;

	std::vector<rtc::byte> sample{(rtc::byte *)data,
				      (rtc::byte *)data + size};

	auto rtp_config = rtcp_sr_reporter->rtpConfig;

	// Sample time is in microseconds, we need to convert it to seconds
	auto elapsed_seconds = double(duration) / (1000.0 * 1000.0);

	// Get elapsed time in clock rate
	uint32_t elapsed_timestamp =
		rtp_config->secondsToTimestamp(elapsed_seconds);

	// Set new timestamp
	rtp_config->timestamp = rtp_config->timestamp + elapsed_timestamp;

	// Get elapsed time in clock rate from last RTCP sender report
	auto report_elapsed_timestamp =
		rtp_config->timestamp -
		rtcp_sr_reporter->lastReportedTimestamp();

	// Check if last report was at least 1 second ago
	if (rtp_config->timestampToSeconds(report_elapsed_timestamp) > 1)
		rtcp_sr_reporter->setNeedsToReport();

	try {
		track->send(sample);
		total_bytes_sent += sample.size();
	} catch (const std::exception &e) {
		do_log(LOG_ERROR, "error: %s ", e.what());
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
	info.encoded_packet = [](void *priv_data,
				 struct encoder_packet *packet) {
		static_cast<WHIPOutput *>(priv_data)->Data(packet);
	};
	info.get_defaults = [](obs_data_t *) {
	};
	info.get_properties = [](void *) -> obs_properties_t * {
		return obs_properties_create();
	};
	info.get_total_bytes = [](void *priv_data) -> uint64_t {
		return (uint64_t) static_cast<WHIPOutput *>(priv_data)
			->GetTotalBytes();
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
