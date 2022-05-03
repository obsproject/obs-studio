#include "util.hpp"

const char *bmd_video_connection_to_name(BMDVideoConnection connection)
{
	switch (connection) {
	case bmdVideoConnectionSDI:
		return "SDI";
	case bmdVideoConnectionHDMI:
		return "HDMI";
	case bmdVideoConnectionOpticalSDI:
		return "Optical SDI";
	case bmdVideoConnectionComponent:
		return "Component";
	case bmdVideoConnectionComposite:
		return "Composite";
	case bmdVideoConnectionSVideo:
		return "S-Video";
	default:
		return "Unknown";
	}
}

const char *bmd_audio_connection_to_name(BMDAudioConnection connection)
{
	switch (connection) {
	case bmdAudioConnectionEmbedded:
		return "Embedded";
	case bmdAudioConnectionAESEBU:
		return "AES/EBU";
	case bmdAudioConnectionAnalog:
		return "Analog";
	case bmdAudioConnectionAnalogXLR:
		return "Analog XLR";
	case bmdAudioConnectionAnalogRCA:
		return "Analog RCA";
	case bmdAudioConnectionMicrophone:
		return "Microphone";
	case bmdAudioConnectionHeadphones:
		return "Headphones";
	default:
		return "Unknown";
	}
}

RollingAverage::RollingAverage(size_t capacity) :
	capacity(capacity),
	size(0),
	sum(0)
{
	samples = {0};

	//circlebuf_reserve(&samples, sizeof(int64_t) * capacity);
}

RollingAverage::~RollingAverage()
{
	circlebuf_free(&samples);
}

void RollingAverage::SubmitSample(int64_t sample)
{
	if (size == capacity) {
		int64_t out = 0;
		circlebuf_pop_front(&samples, &out, sizeof(out));

		size--;
		sum -= out;
	}

	circlebuf_push_back(&samples, &sample, sizeof(sample));

	size++;
	sum += sample;
}

int64_t RollingAverage::GetAverage()
{
	return size ? (sum / (int64_t)size) : 0;
}
