#pragma once

#include <util/circlebuf.h>

#include "decklink-device.hpp"

const char *bmd_video_connection_to_name(BMDVideoConnection connection);

const char *bmd_audio_connection_to_name(BMDAudioConnection connection);

class RollingAverage {
	public:
		RollingAverage(size_t capacity = 2048);
		~RollingAverage();

		void SubmitSample(int64_t sample);
		int64_t GetAverage();

		inline size_t GetCapacity()
		{
			return capacity;
		}

		inline size_t GetSize()
		{
			return size;
		}

	private:
		size_t capacity;
		size_t size;

		int64_t sum;

		struct circlebuf samples;
};
