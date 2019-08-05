#include "VideoCapturer.h"

#include "api/video/i420_buffer.h"
#include "rtc_base/ref_counted_object.h"

#include <algorithm>
// clang-format off

VideoCapturer::VideoCapturer() {}

VideoCapturer::~VideoCapturer() = default;

rtc::scoped_refptr<VideoCapturer> VideoCapturer::Create()
{
	return new rtc::RefCountedObject<VideoCapturer>();
}

void VideoCapturer::OnFrameCaptured(const webrtc::VideoFrame &frame)
{
	int adapted_width;
	int adapted_height;
	int crop_width;
	int crop_height;
	int crop_x;
	int crop_y;

	if (!AdaptFrame(frame.width(), frame.height(), frame.timestamp_us(),
			&adapted_width, &adapted_height, &crop_width,
			&crop_height, &crop_x, &crop_y)) {
		// Drop frame in order to respect frame rate constraint.
		return;
	}

	if (adapted_width != frame.width() || adapted_height != frame.height()) {
		// Video adapter has requested a down-scale. Allocate a new buffer and
		// return scaled version.
		rtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer =
			webrtc::I420Buffer::Create(adapted_width,
						   adapted_height);

		scaled_buffer->ScaleFrom(*frame.video_frame_buffer()->ToI420());

		OnFrame(webrtc::VideoFrame::Builder()
				.set_video_frame_buffer(scaled_buffer)
				.set_rotation(webrtc::kVideoRotation_0)
				.set_timestamp_us(frame.timestamp_us())
				.set_id(frame.id())
				.build());
	} else {
		// No adaptations needed, just return the frame as is.
		OnFrame(frame);
	}
}

bool VideoCapturer::is_screencast() const
{
	return false;
}

absl::optional<bool> VideoCapturer::needs_denoising() const
{
	return false;
}

webrtc::MediaSourceInterface::SourceState VideoCapturer::state() const
{
	return webrtc::MediaSourceInterface::SourceState::kLive;
}

bool VideoCapturer::remote() const
{
	return false;
}

// clang-format on
