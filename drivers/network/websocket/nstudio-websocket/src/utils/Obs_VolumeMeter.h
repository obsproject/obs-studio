/*
obs-websocket
Copyright (C) 2016-2021 Stephane Lepin <stephane.lepin@gmail.com>
Copyright (C) 2020-2021 Kyle Manning <tt2468@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <thread>
#include <obs.hpp>

#include "Obs.h"
#include "Json.h"

namespace Utils {
	namespace Obs {
		namespace VolumeMeter {
			// Some code copied from https://github.com/obsproject/obs-studio/blob/master/libobs/obs-audio-controls.c
			// Keeps a running tally of the current audio levels, for a specific input
			class Meter {
			public:
				Meter(obs_source_t *input);
				~Meter();

				bool InputValid();
				obs_weak_source_t *GetWeakInput() { return _input; }
				json GetMeterData();

				std::atomic<enum obs_peak_meter_type> PeakMeterType;

			private:
				OBSWeakSourceAutoRelease _input;

				// All values in mul
				std::mutex _mutex;
				bool _muted;
				int _channels;
				float _magnitude[MAX_AUDIO_CHANNELS];
				float _peak[MAX_AUDIO_CHANNELS];
				float _previousSamples[MAX_AUDIO_CHANNELS][4];

				std::atomic<uint64_t> _lastUpdate;
				std::atomic<float> _volume;

				void ResetAudioLevels();
				void ProcessAudioChannels(const struct audio_data *data);
				void ProcessPeak(const struct audio_data *data);
				void ProcessMagnitude(const struct audio_data *data);

				static void InputAudioCaptureCallback(void *priv_data, obs_source_t *source,
								      const struct audio_data *data, bool muted);
				static void InputVolumeCallback(void *priv_data, calldata_t *cd);
			};

			// Maintains an array of active inputs
			class Handler {
				typedef std::function<void(std::vector<json> &)> UpdateCallback;
				typedef std::unique_ptr<Meter> MeterPtr;

			public:
				Handler(UpdateCallback cb, uint64_t updatePeriod = 50);
				~Handler();

			private:
				UpdateCallback _updateCallback;

				std::mutex _meterMutex;
				std::vector<MeterPtr> _meters;
				uint64_t _updatePeriod;

				std::mutex _mutex;
				std::condition_variable _cond;
				std::atomic<bool> _running;
				std::thread _updateThread;

				void UpdateThread();
				static void InputActivateCallback(void *priv_data, calldata_t *cd);
				static void InputDeactivateCallback(void *priv_data, calldata_t *cd);
			};
		}
	}
}
