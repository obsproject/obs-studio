#ifndef _OBS_VIDEO_CAPTURER_
#define _OBS_VIDEO_CAPTURER_

#include "api/scoped_refptr.h"
#include "media/base/adapted_video_track_source.h"

class VideoCapturer : public rtc::AdaptedVideoTrackSource {
public:
	VideoCapturer();
	~VideoCapturer() override;

	static rtc::scoped_refptr<VideoCapturer> Create();

	void OnFrameCaptured(const webrtc::VideoFrame &frame);

	// VideoTrackSourceInterface API
	bool is_screencast() const override;
	absl::optional<bool> needs_denoising() const override;

	// MediaSourceInterface API
	webrtc::MediaSourceInterface::SourceState state() const override;
	bool remote() const override;
};

#endif
