#include "WebRTCStream.h"
#include "SdpMunger.h"

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
#include <thread>
#include <vector>
// clang-format off

#define debug(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR, format, ##__VA_ARGS__)

class StatsCallback : public webrtc::RTCStatsCollectorCallback {
public:
	bool called() const { return called_; }
	rtc::scoped_refptr<const webrtc::RTCStatsReport> report()
	{
		return report_;
	}
protected:
	void OnStatsDelivered(
		const rtc::scoped_refptr<const webrtc::RTCStatsReport> &report) override
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
	: audio_bitrate_(128),
	  video_bitrate_(2500),
	  audio_bytes_sent_(0),
	  video_bytes_sent_(0),
	  total_bytes_sent_(0),
	  frame_id_(0),
	  frames_dropped_(0),
	  adm_(AudioDeviceModuleWrapper::Create()),
	  videoCapturer_(VideoCapturer::Create()),
	  network_(rtc::Thread::CreateWithSocketServer()),
	  worker_(rtc::Thread::Create()),
	  signaling_(rtc::Thread::Create()),
	  client_(nullptr),
	  output_(output)
{
	rtc::LogMessage::AddLogToStream(&logger, rtc::LoggingSeverity::LS_VERBOSE);

	network_->SetName("network", nullptr);
	network_->Start();

	worker_->SetName("worker", nullptr);
	worker_->Start();

	signaling_->SetName("signaling", nullptr);
	signaling_->Start();

	factory_ = webrtc::CreatePeerConnectionFactory(
		network_.get(), worker_.get(), signaling_.get(), adm_,
		webrtc::CreateBuiltinAudioEncoderFactory(),
		webrtc::CreateBuiltinAudioDecoderFactory(),
		webrtc::CreateBuiltinVideoEncoderFactory(),
		webrtc::CreateBuiltinVideoDecoderFactory(), nullptr, nullptr);
}

WebRTCStream::~WebRTCStream()
{
	rtc::LogMessage::RemoveLogToStream(&logger);

	// Close peer connection and disconnect websocket connection
	close(false);

	adm_ = nullptr;
	factory_ = nullptr;
	pc_ = nullptr;
	videoCapturer_ = nullptr;

	if (!network_->IsCurrent())
		network_->Stop();
	if (!worker_->IsCurrent())
		worker_->Stop();
	if (!signaling_->IsCurrent())
		signaling_->Stop();

	network_.release();
	worker_.release();
	signaling_.release();
}

bool WebRTCStream::start(WebRTCStream::Type type)
{
	this->type_ = type;

	obs_service_t *service = obs_output_get_service(output_);
	if (!service)
		return false;

	// WebSocket URL sanity check
	if (type != WebRTCStream::Type::Millicast &&
	    !obs_service_get_url(service)) {
		warn("Invalid url");
		obs_output_signal_stop(output_, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	// Close peer connection and disconnect websocket connection (just in case)
	if (close(false))
		obs_output_signal_stop(output_, OBS_OUTPUT_ERROR);

	webrtc::PeerConnectionInterface::RTCConfiguration pc_config;
	webrtc::PeerConnectionInterface::IceServer server;
	server.urls = {"stun:stun.l.google.com:19302"};
	pc_config.servers.push_back(server);

	webrtc::PeerConnectionDependencies pc_dependencies(this);

	pc_ = factory_->CreatePeerConnection(pc_config, std::move(pc_dependencies));

	if (!pc_.get()) {
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
	options.typing_detection.emplace(false); // default: true
	options.experimental_agc.emplace(false);
	options.extended_filter_aec.emplace(false);
	options.delay_agnostic_aec.emplace(false);
	options.experimental_ns.emplace(false);
	options.residual_echo_detector.emplace(false); // default: true

	rtc::scoped_refptr<webrtc::MediaStreamInterface> stream =
		factory_->CreateLocalMediaStream("obs");

	rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track =
		factory_->CreateAudioTrack("audio", factory_->CreateAudioSource(options));
	// pc->AddTrack(audio_track, {"obs"}); // Unified Plan
	stream->AddTrack(audio_track);

	rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track =
		factory_->CreateVideoTrack("video", videoCapturer_);
	// pc->AddTrack(video_track, {"obs"}); // Unified Plan
	stream->AddTrack(video_track);

	// Add the stream to the peer connection
	if (!pc_->AddStream(stream)) {
		warn("Adding stream to PeerConnection failed");
		// Close Peer Connection
		close(false);
		obs_output_signal_stop(output_, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	client_ = createWebsocketClient(type);
	if (!client_) {
		warn("Error creating Websocket client");
		close(false);
		obs_output_signal_stop(output_, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	std::string url = obs_service_get_url(service)
			? obs_service_get_url(service)
			: "";
	std::string room = obs_service_get_room(service)
			 ? obs_service_get_room(service)
			 : "";
	username_ = obs_service_get_username(service)
		  ? obs_service_get_username(service)
		  : "";
	std::string password = obs_service_get_password(service)
			     ? obs_service_get_password(service)
			     : "";
	video_codec_ = obs_service_get_codec(service)
		     ? obs_service_get_codec(service)
		     : "";
	protocol_ = obs_service_get_protocol(service)
		  ? obs_service_get_protocol(service)
		  : "";

	info("Video codec:      %s",
	     video_codec_.empty() ? "Automatic" : video_codec_.c_str());
	info("Protocol:         %s",
	     protocol_.empty() ? "Automatic" : protocol_.c_str());

	if (type == WebRTCStream::Type::Janus) {
		info("Server Room:      %s\nStream Key:       %s\n",
		     room.c_str(), password.c_str());
	} else if (type == WebRTCStream::Type::Wowza) {
		info("Application Name: %s\nStream Name:      %s\n",
		     room.c_str(), username_.c_str());
	} else if (type == WebRTCStream::Type::Millicast) {
		info("Stream Name:      %s\nPublishing Token: %s\n",
		     username_.c_str(), password.c_str());
		url = "Millicast";
	}
	info("CONNECTING TO %s", url.c_str());

	// Connect to server
	if (!client_->connect(url, room, username_, password, this)) {
		warn("Error connecting to server");
		close(false);
		obs_output_signal_stop(output_, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}
	return true;
}

void WebRTCStream::onConnected()
{
	info("WebRTCStream::onConnected");
}

void WebRTCStream::onLogged(int /* code */)
{
	info("WebRTCStream::onLogged\nCreating offer...");
	webrtc::PeerConnectionInterface::RTCOfferAnswerOptions offer_options;
	offer_options.voice_activity_detection = false;
	pc_->CreateOffer(this, offer_options);
}

void WebRTCStream::OnSuccess(webrtc::SessionDescriptionInterface *desc)
{
	info("WebRTCStream::OnSuccess\n");
	std::string sdp;
	desc->ToString(&sdp);
	std::string audio_codec = "opus";
	obs_output_t *context = output_;

	obs_encoder_t *aencoder = obs_output_get_audio_encoder(context, 0);
	obs_data_t *asettings = obs_encoder_get_settings(aencoder);
	audio_bitrate_ = (int)obs_data_get_int(asettings, "bitrate");
	obs_data_release(asettings);

	obs_encoder_t *vencoder = obs_output_get_video_encoder(context);
	obs_data_t *vsettings = obs_encoder_get_settings(vencoder);
	video_bitrate_ = (int)obs_data_get_int(vsettings, "bitrate");
	obs_data_release(vsettings);

	info("Audio codec:      %s", audio_codec.c_str());
	info("Audio bitrate:    %d\n", audio_bitrate_);
	info("Video codec:      %s",
	     video_codec_.empty() ? "Automatic" : video_codec_.c_str());
	info("Video bitrate:    %d\n", video_bitrate_);
	info("OFFER:\n\n%s\n", sdp.c_str());

	std::string sdpCopy = sdp;
	if (type_ == WebRTCStream::Type::Wowza) {
		std::vector<int> audio_payloads;
		std::vector<int> video_payloads;
		// If codec setting is Automatic
		if (video_codec_.empty()) {
			audio_codec = "";
			video_codec_ = "";
		}
		// Force specific video/audio payload
		SdpMunger::ForcePayload(sdpCopy, audio_payloads, video_payloads,
					audio_codec, video_codec_, 0, "42e01f", 0);
		// Constrain video bitrate
		SdpMunger::ConstrainVideoBitrateMaxMin(sdpCopy, video_bitrate_,
						       video_payloads);
		// Enable stereo & constrain audio bitrate
		SdpMunger::EnableStereo(sdpCopy, audio_bitrate_);
	}

	info("SETTING LOCAL DESCRIPTION\n\n");
	pc_->SetLocalDescription(this, desc);

	info("Sending OFFER (SDP) to remote peer:\n\n%s", sdpCopy.c_str());
	client_->open(sdpCopy, video_codec_, username_);
}

void WebRTCStream::OnSuccess()
{
	info("SDP set\n");
}

void WebRTCStream::OnFailure(const std::string &error)
{
	warn("WebRTCStream::OnFailure [%s]", error.c_str());
	close(false);
	obs_output_signal_stop(output_, OBS_OUTPUT_ERROR);
}

void WebRTCStream::OnIceCandidate(const webrtc::IceCandidateInterface *candidate)
{
	std::string str;
	candidate->ToString(&str);
	// Send candidate to peer
	client_->trickle(candidate->sdp_mid(), candidate->sdp_mline_index(), str, false);
}

void WebRTCStream::onRemoteIceCandidate(const std::string &sdpData)
{
	if (sdpData.empty()) {
		info("ICE COMPLETE\n");
		pc_->AddIceCandidate(nullptr);
	} else {
		std::string s = sdpData;
		s.erase(remove(s.begin(), s.end(), '\"'), s.end());
		if (protocol_.empty() ||
		    SdpMunger::FilterIceCandidates(s, protocol_)) {
			info("Remote %s\n", s.c_str());
			const std::string candidate = s;
			const std::string sdpMid = "";
			int sdpMLineIndex = 0;
			webrtc::SdpParseError error;
			const webrtc::IceCandidateInterface *newCandidate =
				webrtc::CreateIceCandidate(sdpMid,
							   sdpMLineIndex,
							   candidate, &error);
			pc_->AddIceCandidate(newCandidate);
		} else {
			info("Ignoring remote %s\n", s.c_str());
		}
	}
}

void WebRTCStream::onOpened(const std::string &sdp)
{
	info("ANSWER:\n\n%s\n", sdp.c_str());

	std::string sdpCopy = sdp;

	if (type_ != WebRTCStream::Type::Wowza) {
		// Constrain video bitrate
		SdpMunger::ConstrainVideoBitrate(sdpCopy, video_bitrate_);
		// Enable stereo & constrain audio bitrate
		SdpMunger::EnableStereo(sdpCopy, audio_bitrate_);
	}

	// Create ANSWER from sdpCopy
	webrtc::SdpParseError error;
	std::unique_ptr<webrtc::SessionDescriptionInterface> answer =
		webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer,
						 sdpCopy, &error);

	info("SETTING REMOTE DESCRIPTION\n\n%s", sdpCopy.c_str());
	pc_->SetRemoteDescription(std::move(answer), make_scoped_refptr(this));

	audio_convert_info conversion;
	conversion.format = AUDIO_FORMAT_16BIT;
	conversion.samples_per_sec = 48000;
	conversion.speakers = SPEAKERS_STEREO;
	obs_output_set_audio_conversion(output_, &conversion);

	info("Begin data capture...");
	obs_output_begin_data_capture(output_, 0);
}

void WebRTCStream::OnSetRemoteDescriptionComplete(webrtc::RTCError error)
{
	if (error.ok()) {
		info("Remote Description set\n");
	} else {
		warn("Error setting Remote Description: %s\n", error.message());
		close(false);
		obs_output_signal_stop(output_, OBS_OUTPUT_ERROR);
	}
}

bool WebRTCStream::close(bool wait)
{
	if (!pc_.get())
		return false;
	// Get pointer
	auto old = pc_.release();
	// Close Peer Connection
	old->Close();
	// Shutdown websocket connection
	if (client_) {
		client_->disconnect(wait);
		delete (client_);
		client_ = nullptr;
	}
	return true;
}

bool WebRTCStream::stop()
{
	info("WebRTCStream::stop");
	close(true);
	obs_output_end_data_capture(output_);
	return true;
}

void WebRTCStream::onDisconnected()
{
	info("WebRTCStream::onDisconnected");
	close(false);
	obs_output_signal_stop(output_, OBS_OUTPUT_ERROR);
}

void WebRTCStream::onLoggedError(int code)
{
	info("WebRTCStream::onLoggedError [code: %d]", code);
	close(false);
	obs_output_signal_stop(output_, OBS_OUTPUT_ERROR);
}

void WebRTCStream::onOpenedError(int code)
{
	info("WebRTCStream::onOpenedError [code: %d]", code);
	close(false);
	obs_output_signal_stop(output_, OBS_OUTPUT_ERROR);
}

void WebRTCStream::setCodec(const std::string &new_codec)
{
	video_codec_ = new_codec;
}

void WebRTCStream::onAudioFrame(audio_data *frame)
{
	if (!frame)
		return;
	// Push it to the device
	adm_->onIncomingData(frame->data[0], frame->frames);
}

void WebRTCStream::onVideoFrame(video_data *frame)
{
	if (!frame)
		return;
	if (!videoCapturer_)
		return;

	int outputWidth = static_cast<int>(obs_output_get_width(output_));
	int outputHeight = static_cast<int>(obs_output_get_height(output_));
	auto videoType = webrtc::VideoType::kNV12;
	uint32_t size = outputWidth * outputHeight * 3 / 2;

	int stride_y = outputWidth;
	int stride_uv = (outputWidth + 1) / 2;
	int target_width = abs(outputWidth);
	int target_height = abs(outputHeight);

	libyuv::RotationMode rotation_mode = libyuv::kRotate0;

	rtc::scoped_refptr<webrtc::I420Buffer> buffer =
		webrtc::I420Buffer::Create(target_width, target_height,
					   stride_y, stride_uv, stride_uv);

	// Convert frame
	const int conversionResult = libyuv::ConvertToI420(
		frame->data[0], size, buffer.get()->MutableDataY(),
		buffer.get()->StrideY(), buffer.get()->MutableDataU(),
		buffer.get()->StrideU(), buffer.get()->MutableDataV(),
		buffer.get()->StrideV(), 0, 0, outputWidth, outputHeight,
		target_width, target_height, rotation_mode,
		ConvertVideoType(videoType));

	// not using the result yet, silence compiler
	(void)conversionResult;

	const int64_t obs_timestamp_ns = static_cast<int64_t>(frame->timestamp);

	const int64_t obs_timestamp_us =
		obs_timestamp_ns / rtc::kNumNanosecsPerMicrosec;

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
			.set_id(++frame_id_)
			.build();

	// Send frame to video capturer
	videoCapturer_->OnFrameCaptured(video_frame);
}

uint64_t WebRTCStream::getBitrate()
{
	rtc::scoped_refptr<const webrtc::RTCStatsReport> report = NewGetStats();

	if (report) {
		std::vector<const webrtc::RTCMediaStreamTrackStats *>
			media_stream_track_stats = report->GetStatsOfType<
				webrtc::RTCMediaStreamTrackStats>();
		std::vector<const webrtc::RTCOutboundRTPStreamStats *>
			outbound_stream_stats = report->GetStatsOfType<
				webrtc::RTCOutboundRTPStreamStats>();

		for (const auto &stat : media_stream_track_stats) {
			if (stat->kind.ValueToString() == "video")
				frames_dropped_ = *stat->frames_dropped;
		}

		for (const auto &stat : outbound_stream_stats) {
			if (stat->kind.ValueToString() == "audio") {
				uint64_t temp = *stat->bytes_sent;
				if (temp > audio_bytes_sent_)
					audio_bytes_sent_ = temp;
			}
			if (stat->kind.ValueToString() == "video") {
				uint64_t temp = *stat->bytes_sent;
				if (temp > video_bytes_sent_)
					video_bytes_sent_ = temp;
			}
		}
	}

	total_bytes_sent_ = audio_bytes_sent_ + video_bytes_sent_;
	return total_bytes_sent_;
}

int WebRTCStream::getDroppedFrames()
{
	return frames_dropped_;
}

rtc::scoped_refptr<const webrtc::RTCStatsReport> WebRTCStream::NewGetStats()
{
	rtc::scoped_refptr<StatsCallback> stats_callback =
		new rtc::RefCountedObject<StatsCallback>();

	pc_->GetStats(stats_callback);

	// Wait for stats report to be delivered
	for (int64_t i = 0; i < 1000 && !stats_callback->called(); i++) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
	}

	return stats_callback->called() ? stats_callback->report() : nullptr;
}

// clang-format on
