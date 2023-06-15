#include "simulcast-output.h"

void SimulcastOutput::StartStreaming()
{
	streaming_ = true;
}

void SimulcastOutput::StopStreaming()
{
	streaming_ = false;
}

bool SimulcastOutput::IsStreaming()
{
	return streaming_;
}