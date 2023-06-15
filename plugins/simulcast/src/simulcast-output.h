#pragma once

class SimulcastOutput {
public:
	void StartStreaming();
	void StopStreaming();
	bool IsStreaming();

private:
	bool streaming_ = false;
};
