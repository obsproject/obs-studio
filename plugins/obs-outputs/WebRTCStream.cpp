#include "WebRTCStream.h"

#include "media-io/video-io.h"

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video/i420_buffer.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "pc/rtc_stats_collector.h"
#include "rtc_base/checks.h"
#include "rtc_base/critical_section.h"
#include <libyuv.h>

#include <algorithm>
#include <chrono>
#include <iterator>
#include <memory>
#include <mutex>
#include <thread>

#define debug(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR, format, ##__VA_ARGS__)

class StatsCallback : public webrtc::RTCStatsCollectorCallback {
public:
	rtc::scoped_refptr<const webrtc::RTCStatsReport> report()
	{
		return report_;
	}
	bool called() const { return called_; }

protected:
	void OnStatsDelivered(
		const rtc::scoped_refptr<const webrtc::RTCStatsReport> &report)
		override
	{
		report_ = report;
		called_ = true;
	}

private:
	bool called_ = false;
	rtc::scoped_refptr<const webrtc::RTCStatsReport> report_;
};

class CustomLogger : public rtc::LogSink {
public:
	void OnLogMessage(const std::string &message) override
	{
		info("%s", message.c_str());
	}
};

CustomLogger logger;

WebRTCStream::WebRTCStream(obs_output_t *output)
{
	// rtc::LogMessage::ConfigureLogging("info");
	rtc::LogMessage::RemoveLogToStream(&logger);
	rtc::LogMessage::AddLogToStream(&logger,
					rtc::LoggingSeverity::LS_VERBOSE);

	frame_id = 0;
	pli_received = 0;
	audio_bytes_sent = 0;
	video_bytes_sent = 0;
	total_bytes_sent = 0;

	audio_bitrate = 128;
	video_bitrate = 2500;

	// Store output
	this->output = output;
	this->client = nullptr;

	// Create audio device module
	adm = new rtc::RefCountedObject<AudioDeviceModuleWrapper>();

	// Network thread
	network = rtc::Thread::CreateWithSocketServer();
	network->SetName("network", nullptr);
	network->Start();

	// Worker thread
	worker = rtc::Thread::Create();
	worker->SetName("worker", nullptr);
	worker->Start();

	// Signaling thread
	signaling = rtc::Thread::Create();
	signaling->SetName("signaling", nullptr);
	signaling->Start();

	factory = webrtc::CreatePeerConnectionFactory(
		network.get(), worker.get(), signaling.get(), adm,
		webrtc::CreateBuiltinAudioEncoderFactory(),
		webrtc::CreateBuiltinAudioDecoderFactory(),
		webrtc::CreateBuiltinVideoEncoderFactory(),
		webrtc::CreateBuiltinVideoDecoderFactory(), nullptr, nullptr);

	// Create video capture module
	videoCapturer = new rtc::RefCountedObject<VideoCapturer>();
}

WebRTCStream::~WebRTCStream()
{
	rtc::LogMessage::RemoveLogToStream(&logger);

	// Shutdown websocket connection and close Peer Connection
	close(false);

	// Free factories
	adm = nullptr;
	pc = nullptr;
	factory = nullptr;
	videoCapturer = nullptr;

	// Stop all threads
	if (!network->IsCurrent())
		network->Stop();
	if (!worker->IsCurrent())
		worker->Stop();
	if (!signaling->IsCurrent())
		signaling->Stop();

	network.release();
	worker.release();
	signaling.release();
}

bool WebRTCStream::start(WebRTCStream::Type type)
{
	this->type = type;

	obs_service_t *service = obs_output_get_service(output);
	if (!service)
		return false;

	// WebSocket URL sanity check
	if (type != WebRTCStream::Type::Millicast &&
	    !obs_service_get_url(service)) {
		warn("Invalid url");
		obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	obs_output_t *context = output;
	obs_encoder_t *aencoder = obs_output_get_audio_encoder(context, 0);
	if (aencoder) {
		obs_data_t *aparams = obs_encoder_get_settings(aencoder);
		if (aparams) {
			audio_bitrate =
				(int)obs_data_get_int(aparams, "bitrate");
			obs_data_release(aparams);
		}
	}
	obs_encoder_t *vencoder = obs_output_get_video_encoder(context);
	if (vencoder) {
		obs_data_t *vparams = obs_encoder_get_settings(vencoder);
		if (vparams) {
			video_bitrate =
				(int)obs_data_get_int(vparams, "bitrate");
			obs_data_release(vparams);
		}
	}

	// Shutdown websocket connection and close Peer Connection (just in case)
	if (close(false))
		obs_output_signal_stop(output, OBS_OUTPUT_ERROR);

	webrtc::PeerConnectionInterface::RTCConfiguration config;
	webrtc::PeerConnectionInterface::IceServer server;
	server.urls = {"stun:stun.l.google.com:19302"};
	config.servers.push_back(server);
	// config.bundle_policy = webrtc::PeerConnectionInterface::kBundlePolicyMaxBundle;
	// config.disable_ipv6 = true;
	// config.rtcp_mux_policy = webrtc::PeerConnectionInterface::kRtcpMuxPolicyRequire;
	// config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
	// config.set_cpu_adaptation(false);
	// config.set_suspend_below_min_bitrate(false);

	webrtc::PeerConnectionDependencies dependencies(this);

	pc = factory->CreatePeerConnection(config, std::move(dependencies));

	if (!pc.get()) {
		error("Error creating Peer Connection");
		return false;
	} else {
		info("PEER CONNECTION CREATED\n");
	}

	cricket::AudioOptions options;
	options.echo_cancellation.emplace(false); // default: true
	options.auto_gain_control.emplace(false); // default: true
	options.noise_suppression.emplace(false); // default: true
	options.highpass_filter.emplace(false);   // default: true
	options.stereo_swapping.emplace(false);
	options.audio_jitter_buffer_fast_accelerate.emplace(false);
	options.typing_detection.emplace(false); // default: true
	options.experimental_agc.emplace(false);
	options.extended_filter_aec.emplace(false);
	options.delay_agnostic_aec.emplace(false);
	options.experimental_ns.emplace(false);
	options.residual_echo_detector.emplace(false); // default: true
	// options.tx_agc_limiter.emplace(false);

	stream = factory->CreateLocalMediaStream("obs");

	audio_track = factory->CreateAudioTrack(
		"audio", factory->CreateAudioSource(options));
	// pc->AddTrack(audio_track, {"obs"});
	stream->AddTrack(audio_track);

	video_track = factory->CreateVideoTrack("video", videoCapturer);
	// pc->AddTrack(video_track, {"obs"});
	stream->AddTrack(video_track);

	// Add the stream to the peer connection
	if (!pc->AddStream(stream)) {
		warn("Adding stream to PeerConnection failed");
		// Close Peer Connection
		close(false);
		// Disconnect, this will call stop on main thread
		obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	client = createWebsocketClient(type);
	if (!client) {
		warn("Error creating Websocket client");
		// Close Peer Connection
		close(false);
		// Disconnect, this will call stop on main thread
		obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	url = obs_service_get_url(service) ? obs_service_get_url(service) : "";
	room = obs_service_get_room(service) ? obs_service_get_room(service)
					     : "";
	username = obs_service_get_username(service)
			   ? obs_service_get_username(service)
			   : "";
	password = obs_service_get_password(service)
			   ? obs_service_get_password(service)
			   : "";
	video_codec = obs_service_get_codec(service)
			      ? obs_service_get_codec(service)
			      : "";
	protocol = obs_service_get_protocol(service)
			   ? obs_service_get_protocol(service)
			   : "";

	info("Video codec:  %s\n",
	     video_codec.empty() ? "Automatic" : video_codec.c_str());
	info("Protocol:     %s\n",
	     protocol.empty() ? "Automatic" : protocol.c_str());

	if (type == WebRTCStream::Type::Janus) {
		info("Server Room: %s\nStream Key: %s\n", room.c_str(),
		     password.c_str());
	} else if (type == WebRTCStream::Type::Wowza) {
		info("Application Name: %s\nStream Name: %s\n", room.c_str(),
		     username.c_str());
	} else if (type == WebRTCStream::Type::Millicast) {
		info("Stream Name: %s\nPublishing Token: %s\n",
		     username.c_str(), password.c_str());
		url = "";
	} else if (type == WebRTCStream::Type::Evercast) {
		info("Server Room: %s\nStream Key: %s\n", room.c_str(),
		     password.c_str());
	}
	info("CONNECTING TO %s", url.c_str());

	// Connect to server
	if (!client->connect(url, room, username, password, this)) {
		warn("Error connecting to server");
		// Shutdown websocket connection and close Peer Connection
		close(false);
		// Disconnect, this will call stop on main thread
		obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}
	return true;
}

void WebRTCStream::onConnected()
{
	debug("WebRTCStream::onConnected");
}

void WebRTCStream::onLogged(int /* code */)
{
	debug("WebRTCStream::onLogged\nCreating offer...");
	webrtc::PeerConnectionInterface::RTCOfferAnswerOptions offer_options;
	// offer_options.voice_activity_detection = false;
	pc->CreateOffer(this, offer_options);
}

void WebRTCStream::OnSuccess(webrtc::SessionDescriptionInterface *desc)
{
	std::string sdp;
	desc->ToString(&sdp);
	std::string sdpCopy = sdp;

	info("Video codec:   %s",
	     video_codec.empty() ? "Automatic" : video_codec.c_str());
	info("Video bitrate: %d\n", video_bitrate);

	if (type == WebRTCStream::Type::Wowza) {
		info("Audio codec:   %s", audio_codec.c_str());
		info("Audio bitrate: %d\n", audio_bitrate);
		std::vector<int> audio_payloads;
		std::vector<int> video_payloads;
		// If codec setting is Automatic
		if (video_codec.empty()) {
			audio_codec = "";
			video_codec = "";
		}
		// Force specific video/audio payload
		SDPModif::forcePayload(sdpCopy, audio_payloads, video_payloads,
				       audio_codec, video_codec, 0, "42e01f",
				       0);
		// Constrain video bitrate
		SDPModif::bitrateMaxMinSDP(sdpCopy, video_bitrate,
					   video_payloads);
		// Enable stereo & constrain audio bitrate
		SDPModif::stereoSDP(sdpCopy, audio_bitrate);
	}

	info("OFFER:\n\n%s\n", sdp.c_str()); // original (unmodified)

	webrtc::SdpParseError error;
	// Create OFFER from sdpCopy
	std::unique_ptr<webrtc::SessionDescriptionInterface> offer =
		webrtc::CreateSessionDescription(webrtc::SdpType::kOffer,
						 sdpCopy, &error);

	info("SETTING LOCAL DESCRIPTION\n\n%s", sdpCopy.c_str());
	pc->SetLocalDescription(this, offer.release());

	// Send offer sdp to peer
	client->open(sdpCopy, video_codec, username);
}

void WebRTCStream::OnSuccess()
{
	info("SDP set\n");
}

void WebRTCStream::OnFailure(const std::string &error)
{
	warn("WebRTCStream::OnFailure [%s]", error.c_str());
	// Shutdown websocket connection and close Peer Connection
	close(false);
	// Disconnect, this will call stop on main thread
	obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::OnIceCandidate(const webrtc::IceCandidateInterface *candidate)
{
	std::string str;
	candidate->ToString(&str);
	// Send candidate to peer
	client->trickle(candidate->sdp_mid(), candidate->sdp_mline_index(), str,
			false);
}

void WebRTCStream::onRemoteIceCandidate(const std::string &sdpData)
{
	if (sdpData.empty()) {
		info("ICE COMPLETE\n");
		pc->AddIceCandidate(nullptr);
	} else {
		std::string s = sdpData;
		s.erase(remove(s.begin(), s.end(), '\"'), s.end());
		if (protocol.empty() ||
		    SDPModif::filterIceCandidates(s, protocol)) {
			const std::string candidate = s;
			info("Remote %s\n", candidate.c_str());
			const std::string sdpMid = "";
			int sdpMLineIndex = 0;
			webrtc::SdpParseError error;
			const webrtc::IceCandidateInterface *newCandidate =
				webrtc::CreateIceCandidate(sdpMid,
							   sdpMLineIndex,
							   candidate, &error);
			pc->AddIceCandidate(newCandidate);
		} else {
			info("Ignoring remote %s\n", s.c_str());
		}
	}
}

void WebRTCStream::onOpened(const std::string &sdp)
{
	info("ANSWER:\n\n%s\n", sdp.c_str());

	std::string sdpCopy = sdp;

	if (type != WebRTCStream::Type::Wowza) {
		// Constrain video bitrate
		SDPModif::bitrateSDP(sdpCopy, video_bitrate);
		// Enable stereo & constrain audio bitrate
		SDPModif::stereoSDP(sdpCopy, audio_bitrate);
	}

	// SetRemoteDescription observer
	srd_observer = make_scoped_refptr(this);
	webrtc::SdpParseError error;
	// Create ANSWER from sdpCopy
	std::unique_ptr<webrtc::SessionDescriptionInterface> answer =
		webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer,
						 sdpCopy, &error);

	info("SETTING REMOTE DESCRIPTION\n\n%s", sdpCopy.c_str());
	pc->SetRemoteDescription(std::move(answer), srd_observer);

	// Set audio conversion info
	audio_convert_info conversion;
	conversion.format = AUDIO_FORMAT_16BIT;
	conversion.samples_per_sec = 48000;
	conversion.speakers = SPEAKERS_STEREO;
	obs_output_set_audio_conversion(output, &conversion);

	info("Begin data capture...");
	obs_output_begin_data_capture(output, 0);
}

void WebRTCStream::OnSetRemoteDescriptionComplete(webrtc::RTCError error)
{
	if (!error.ok())
		warn("Error setting remote description: %s", error.message());
}

bool WebRTCStream::close(bool wait)
{
	if (!pc.get())
		return false;
	// Get pointer
	auto old = pc.release();
	// Close Peer Connection
	old->Close();
	// Shutdown websocket connection
	if (client) {
		client->disconnect(wait);
		delete (client);
		client = nullptr;
	}
	return true;
}

bool WebRTCStream::stop()
{
	info("WebRTCStream::stop");
	// Shutdown websocket connection and close Peer Connection
	close(true);
	// Disconnect, this will call stop on main thread
	obs_output_end_data_capture(output);
	return true;
}

void WebRTCStream::onDisconnected()
{
	info("WebRTCStream::onDisconnected");
	// Shutdown websocket connection and close Peer Connection
	close(false);
	// Disconnect, this will call stop on main thread
	obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::onLoggedError(int code)
{
	info("WebRTCStream::onLoggedError [code: %d]", code);
	// Shutdown websocket connection and close Peer Connection
	close(false);
	// Disconnect, this will call stop on main thread
	obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::onOpenedError(int code)
{
	info("WebRTCStream::onOpenedError [code: %d]", code);
	// Shutdown websocket connection and close Peer Connection
	close(false);
	// Disconnect, this will call stop on main thread
	obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::onAudioFrame(audio_data *frame)
{
	if (!frame)
		return;
	// Push it to the device
	adm->onIncomingData(frame->data[0], frame->frames);
}

void WebRTCStream::onVideoFrame(video_data *frame)
{
	if (!frame)
		return;
	if (!videoCapturer)
		return;

	// Calculate size
	int outputWidth = obs_output_get_width(output);
	int outputHeight = obs_output_get_height(output);
	auto videoType = webrtc::VideoType::kNV12;
	uint32_t size = outputWidth * outputHeight * 3 / 2;

	int stride_y = outputWidth;
	int stride_uv = (outputWidth + 1) / 2;
	int target_width = abs(outputWidth);
	int target_height = abs(outputHeight);

	// Convert frame
	rtc::scoped_refptr<webrtc::I420Buffer> buffer =
		webrtc::I420Buffer::Create(target_width, target_height,
					   stride_y, stride_uv, stride_uv);

	libyuv::RotationMode rotation_mode = libyuv::kRotate0;

	const int conversionResult = libyuv::ConvertToI420(
		frame->data[0], size, buffer.get()->MutableDataY(),
		buffer.get()->StrideY(), buffer.get()->MutableDataU(),
		buffer.get()->StrideU(), buffer.get()->MutableDataV(),
		buffer.get()->StrideV(), 0, 0, outputWidth, outputHeight,
		target_width, target_height, rotation_mode,
		ConvertVideoType(videoType));

	// not using the result yet, silence compiler
	(void)conversionResult;

	const int64_t obs_timestamp_us =
		(int64_t)frame->timestamp / rtc::kNumNanosecsPerMicrosec;

	// Align timestamps from OBS capturer with rtc::TimeMicros timebase
	const int64_t aligned_timestamp_us =
		timestamp_aligner_.TranslateTimestamp(obs_timestamp_us,
						      rtc::TimeMicros());

	// Create a webrtc::VideoFrame to pass to the capturer
	webrtc::VideoFrame video_frame =
		webrtc::VideoFrame::Builder()
			.set_video_frame_buffer(buffer)
			.set_rotation(webrtc::kVideoRotation_0)
			.set_timestamp_us(aligned_timestamp_us)
			.set_id(++frame_id)
			.build();

	// Send frame to video capturer
	videoCapturer->OnFrameCaptured(video_frame);
}

uint64_t WebRTCStream::getBitrate()
{
	rtc::scoped_refptr<const webrtc::RTCStatsReport> report = NewGetStats();

	std::vector<const webrtc::RTCOutboundRTPStreamStats *> send_stream_stats =
		report->GetStatsOfType<webrtc::RTCOutboundRTPStreamStats>();

	for (const auto &stat : send_stream_stats) {
		if (stat->kind.ValueToString() == "audio")
			audio_bytes_sent =
				std::stoll(stat->bytes_sent.ValueToJson());
		if (stat->kind.ValueToString() == "video") {
			video_bytes_sent =
				std::stoll(stat->bytes_sent.ValueToJson());
			pli_received = std::stoi(stat->pli_count.ValueToJson());
		}
	}
	total_bytes_sent = audio_bytes_sent + video_bytes_sent;
	return total_bytes_sent;
}

int WebRTCStream::getDroppedFrames()
{
	return pli_received;
}

// Synchronously get stats
rtc::scoped_refptr<const webrtc::RTCStatsReport> WebRTCStream::NewGetStats()
{
	rtc::CritScope lock(&crit_);

	rtc::scoped_refptr<StatsCallback> stats_callback =
		new rtc::RefCountedObject<StatsCallback>();

	pc->GetStats(stats_callback);

	while (!stats_callback->called()) {
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}

	return stats_callback->report();
}
