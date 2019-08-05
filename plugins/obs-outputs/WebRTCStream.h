#ifndef _WEBRTCSTREAM_H_
#define _WEBRTCSTREAM_H_

#if WIN32
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "Secur32.lib")
#pragma comment(lib, "Msdmo.lib")
#pragma comment(lib, "dmoguids.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "amstrmid.lib")
#endif

#include "obs.h"

#include "AudioDeviceModuleWrapper.h"
#include "VideoCapturer.h"
#include "WebsocketClient.h"

#include "api/create_peerconnection_factory.h"
#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "api/scoped_refptr.h"
#include "api/set_remote_description_observer_interface.h"
#include "api/stats/rtc_stats_report.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"
#include "rtc_base/timestamp_aligner.h"

#include <string>
// clang-format off

class WebRTCStreamInterface
	: public WebsocketClient::Listener,
	  public webrtc::PeerConnectionObserver,
	  public webrtc::CreateSessionDescriptionObserver,
	  public webrtc::SetSessionDescriptionObserver,
	  public webrtc::SetRemoteDescriptionObserverInterface {
};

class WebRTCStream : public rtc::RefCountedObject<WebRTCStreamInterface> {
public:
	enum Type { Janus = 0, Wowza = 1, Millicast = 2 };

	WebRTCStream(obs_output_t *output);
	~WebRTCStream() override;

	// Close peer connection. Disconnect websocket connection.
	bool close(bool wait);
	bool start(Type type);
	bool stop();
	void setCodec(const std::string &new_codec);
	void onAudioFrame(audio_data *frame);
	void onVideoFrame(video_data *frame);

	uint64_t getBitrate();
	int getDroppedFrames();

	// Synchronously get stats
	rtc::scoped_refptr<const webrtc::RTCStatsReport> NewGetStats();

	//
	// WebsocketClient::Listener implementation.
	//
	void onConnected() override;
	void onDisconnected() override;
	void onLogged(int code) override;
	void onLoggedError(int code) override;
	void onOpened(const std::string &sdp) override;
	void onOpenedError(int code) override;
	void onRemoteIceCandidate(const std::string &sdpData) override;

	//
	// PeerConnectionObserver implementation.
	//
	void OnSignalingChange(
		webrtc::PeerConnectionInterface::SignalingState new_state) override {}
	void OnAddStream(
		rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {}
	void OnRemoveStream(
		rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {}
	void OnDataChannel(
		rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
	void OnRenegotiationNeeded() override {}
	void OnIceConnectionChange(
		webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
	void OnIceGatheringChange(
		webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
	void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override;
	void OnIceConnectionReceivingChange(bool receiving) override {}

	// CreateSessionDescriptionObserver
	void OnSuccess(webrtc::SessionDescriptionInterface *desc) override;

	// CreateSessionDescriptionObserver / SetSessionDescriptionObserver
	void OnFailure(const std::string &error) override;

	// SetSessionDescriptionObserver
	void OnSuccess() override;

	// SetRemoteDescriptionObserverInterface
	void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override;

protected:
	template<typename T> rtc::scoped_refptr<T> make_scoped_refptr(T *t)
	{
		return rtc::scoped_refptr<T>(t);
	}

private:
	Type type_;
	int audio_bitrate_;
	int video_bitrate_;
	std::string protocol_;
	std::string username_;
	std::string video_codec_;

	uint64_t audio_bytes_sent_;
	uint64_t video_bytes_sent_;
	uint64_t total_bytes_sent_;
	uint16_t frame_id_;
	uint32_t frames_dropped_;

	// Audio Wrapper
	rtc::scoped_refptr<AudioDeviceModuleWrapper> adm_;

	// Video Capturer
	rtc::scoped_refptr<VideoCapturer> videoCapturer_;
	rtc::TimestampAligner timestamp_aligner_;

	// PeerConnection
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc_;

	// WebRTC threads
	std::unique_ptr<rtc::Thread> network_;
	std::unique_ptr<rtc::Thread> worker_;
	std::unique_ptr<rtc::Thread> signaling_;

	// Websocket client
	WebsocketClient *client_;

	// OBS stream output
	obs_output_t *output_;
};

// clang-format on
#endif
