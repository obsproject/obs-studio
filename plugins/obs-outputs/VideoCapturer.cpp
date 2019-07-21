#include "VideoCapturer.h"

VideoCapturer::VideoCapturer() {}

VideoCapturer::~VideoCapturer() = default;

void VideoCapturer::OnFrameCaptured(const webrtc::VideoFrame &frame)
{
	rtc::AdaptedVideoTrackSource::OnFrame(frame);
}
