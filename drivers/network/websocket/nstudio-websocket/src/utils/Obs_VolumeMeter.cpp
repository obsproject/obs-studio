/*
obs-websocket
Copyright (C) 2014 by Leonhard Oelke <leonhard@in-verted.de>
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

#include <cmath>
#include <algorithm>

#include "Obs.h"
#include "Obs_VolumeMeter.h"
#include "Obs_VolumeMeter_Helpers.h"
#include "../obs-websocket.h"

Utils::Obs::VolumeMeter::Meter::Meter(obs_source_t *input)
	: PeakMeterType(SAMPLE_PEAK_METER),
	  _input(obs_source_get_weak_source(input)),
	  _channels(0),
	  _lastUpdate(0),
	  _volume(obs_source_get_volume(input))
{
	signal_handler_t *sh = obs_source_get_signal_handler(input);
	signal_handler_connect(sh, "volume", Meter::InputVolumeCallback, this);

	obs_source_add_audio_capture_callback(input, Meter::InputAudioCaptureCallback, this);

	blog_debug("[Utils::Obs::VolumeMeter::Meter::Meter] Meter created for input: %s", obs_source_get_name(input));
}

Utils::Obs::VolumeMeter::Meter::~Meter()
{
	OBSSourceAutoRelease input = obs_weak_source_get_source(_input);
	if (!input) {
		blog(LOG_WARNING,
		     "[Utils::Obs::VolumeMeter::Meter::~Meter] Failed to get strong reference to input. Has it been destroyed?");
		return;
	}

	signal_handler_t *sh = obs_source_get_signal_handler(input);
	signal_handler_disconnect(sh, "volume", Meter::InputVolumeCallback, this);

	obs_source_remove_audio_capture_callback(input, Meter::InputAudioCaptureCallback, this);

	blog_debug("[Utils::Obs::VolumeMeter::Meter::~Meter] Meter destroyed for input: %s", obs_source_get_name(input));
}

bool Utils::Obs::VolumeMeter::Meter::InputValid()
{
	return !obs_weak_source_expired(_input);
}

json Utils::Obs::VolumeMeter::Meter::GetMeterData()
{
	json ret;

	OBSSourceAutoRelease input = obs_weak_source_get_source(_input);
	if (!input) {
		blog(LOG_WARNING,
		     "[Utils::Obs::VolumeMeter::Meter::GetMeterData] Failed to get strong reference to input. Has it been destroyed?");
		return ret;
	}

	std::vector<std::vector<float>> levels;
	const float volume = _muted ? 0.0f : _volume.load();

	std::unique_lock<std::mutex> l(_mutex);

	if (_lastUpdate != 0 && (os_gettime_ns() - _lastUpdate) * 0.000000001 > 0.3)
		ResetAudioLevels();

	for (int channel = 0; channel < _channels; channel++) {
		std::vector<float> level;
		level.push_back(_magnitude[channel] * volume);
		level.push_back(_peak[channel] * volume);
		level.push_back(_peak[channel]);

		levels.push_back(level);
	}
	l.unlock();

	ret["inputName"] = obs_source_get_name(input);
	ret["inputUuid"] = obs_source_get_uuid(input);
	ret["inputLevelsMul"] = levels;

	return ret;
}

// MUST HOLD LOCK
void Utils::Obs::VolumeMeter::Meter::ResetAudioLevels()
{
	_lastUpdate = 0;
	for (int channelNumber = 0; channelNumber < MAX_AUDIO_CHANNELS; channelNumber++) {
		_magnitude[channelNumber] = 0;
		_peak[channelNumber] = 0;
	}
}

// MUST HOLD LOCK
void Utils::Obs::VolumeMeter::Meter::ProcessAudioChannels(const struct audio_data *data)
{
	int channels = 0;
	for (int i = 0; i < MAX_AV_PLANES; i++) {
		if (data->data[i])
			channels++;
	}

	bool channelsChanged = _channels != channels;
	_channels = std::clamp(channels, 0, MAX_AUDIO_CHANNELS);

	if (channelsChanged)
		ResetAudioLevels();
}

// MUST HOLD LOCK
void Utils::Obs::VolumeMeter::Meter::ProcessPeak(const struct audio_data *data)
{
	size_t sampleCount = data->frames;
	int channelNumber = 0;

	for (int planeNumber = 0; channelNumber < _channels; planeNumber++) {
		float *samples = (float *)data->data[planeNumber];
		if (!samples)
			continue;

		if (((uintptr_t)samples & 0xf) > 0) {
			_peak[channelNumber] = 1.0f;
			channelNumber++;
			continue;
		}

		__m128 previousSamples = _mm_loadu_ps(_previousSamples[channelNumber]);

		float peak;
		switch (PeakMeterType) {
		default:
		case SAMPLE_PEAK_METER:
			peak = GetSamplePeak(previousSamples, samples, sampleCount);
			break;
		case TRUE_PEAK_METER:
			peak = GetTruePeak(previousSamples, samples, sampleCount);
			break;
		}

		switch (sampleCount) {
		case 0:
			break;
		case 1:
			_previousSamples[channelNumber][0] = _previousSamples[channelNumber][1];
			_previousSamples[channelNumber][1] = _previousSamples[channelNumber][2];
			_previousSamples[channelNumber][2] = _previousSamples[channelNumber][3];
			_previousSamples[channelNumber][3] = samples[sampleCount - 1];
			break;
		case 2:
			_previousSamples[channelNumber][0] = _previousSamples[channelNumber][2];
			_previousSamples[channelNumber][1] = _previousSamples[channelNumber][3];
			_previousSamples[channelNumber][2] = samples[sampleCount - 2];
			_previousSamples[channelNumber][3] = samples[sampleCount - 1];
			break;
		case 3:
			_previousSamples[channelNumber][0] = _previousSamples[channelNumber][3];
			_previousSamples[channelNumber][1] = samples[sampleCount - 3];
			_previousSamples[channelNumber][2] = samples[sampleCount - 2];
			_previousSamples[channelNumber][3] = samples[sampleCount - 1];
			break;
		default:
			_previousSamples[channelNumber][0] = samples[sampleCount - 4];
			_previousSamples[channelNumber][1] = samples[sampleCount - 3];
			_previousSamples[channelNumber][2] = samples[sampleCount - 2];
			_previousSamples[channelNumber][3] = samples[sampleCount - 1];
		}

		_peak[channelNumber] = peak;

		channelNumber++;
	}

	for (; channelNumber < MAX_AUDIO_CHANNELS; channelNumber++)
		_peak[channelNumber] = 0.0;
}

// MUST HOLD LOCK
void Utils::Obs::VolumeMeter::Meter::ProcessMagnitude(const struct audio_data *data)
{
	size_t sampleCount = data->frames;

	int channelNumber = 0;
	for (int planeNumber = 0; channelNumber < _channels; planeNumber++) {
		float *samples = (float *)data->data[planeNumber];
		if (!samples)
			continue;

		float sum = 0.0;
		for (size_t i = 0; i < sampleCount; i++) {
			float sample = samples[i];
			sum += sample * sample;
		}

		_magnitude[channelNumber] = std::sqrt(sum / sampleCount);

		channelNumber++;
	}
}

void Utils::Obs::VolumeMeter::Meter::InputAudioCaptureCallback(void *priv_data, obs_source_t *, const struct audio_data *data,
							       bool muted)
{
	auto c = static_cast<Meter *>(priv_data);

	std::unique_lock<std::mutex> l(c->_mutex);

	c->_muted = muted;
	c->ProcessAudioChannels(data);
	c->ProcessPeak(data);
	c->ProcessMagnitude(data);

	c->_lastUpdate = os_gettime_ns();
}

void Utils::Obs::VolumeMeter::Meter::InputVolumeCallback(void *priv_data, calldata_t *cd)
{
	auto c = static_cast<Meter *>(priv_data);

	c->_volume = (float)calldata_float(cd, "volume");
}

Utils::Obs::VolumeMeter::Handler::Handler(UpdateCallback cb, uint64_t updatePeriod)
	: _updateCallback(cb),
	  _updatePeriod(updatePeriod),
	  _running(false)
{
	signal_handler_t *sh = obs_get_signal_handler();
	if (!sh)
		return;

	auto enumProc = [](void *priv_data, obs_source_t *input) {
		auto c = static_cast<Handler *>(priv_data);

		if (!obs_source_active(input))
			return true;

		uint32_t flags = obs_source_get_output_flags(input);
		if ((flags & OBS_SOURCE_AUDIO) == 0)
			return true;

		c->_meters.emplace_back(std::move(new Meter(input)));

		return true;
	};
	obs_enum_sources(enumProc, this);

	signal_handler_connect(sh, "source_activate", Handler::InputActivateCallback, this);
	signal_handler_connect(sh, "source_deactivate", Handler::InputDeactivateCallback, this);

	_running = true;
	_updateThread = std::thread(&Handler::UpdateThread, this);

	blog_debug("[Utils::Obs::VolumeMeter::Handler::Handler] Handler created.");
}

Utils::Obs::VolumeMeter::Handler::~Handler()
{
	signal_handler_t *sh = obs_get_signal_handler();
	if (!sh)
		return;

	signal_handler_disconnect(sh, "source_activate", Handler::InputActivateCallback, this);
	signal_handler_disconnect(sh, "source_deactivate", Handler::InputDeactivateCallback, this);

	if (_running) {
		_running = false;
		_cond.notify_all();
	}

	if (_updateThread.joinable())
		_updateThread.join();

	blog_debug("[Utils::Obs::VolumeMeter::Handler::~Handler] Handler destroyed.");
}

void Utils::Obs::VolumeMeter::Handler::UpdateThread()
{
	blog_debug("[Utils::Obs::VolumeMeter::Handler::UpdateThread] Thread started.");
	while (_running) {
		{
			std::unique_lock<std::mutex> l(_mutex);
			if (_cond.wait_for(l, std::chrono::milliseconds(_updatePeriod), [this] { return !_running; }))
				break;
		}

		std::vector<json> inputs;
		std::unique_lock<std::mutex> l(_meterMutex);
		for (auto &meter : _meters) {
			if (meter->InputValid())
				inputs.push_back(meter->GetMeterData());
		}
		l.unlock();

		if (_updateCallback)
			_updateCallback(inputs);
	}
	blog_debug("[Utils::Obs::VolumeMeter::Handler::UpdateThread] Thread stopped.");
}

void Utils::Obs::VolumeMeter::Handler::InputActivateCallback(void *priv_data, calldata_t *cd)
{
	auto c = static_cast<Handler *>(priv_data);

	obs_source_t *input = GetCalldataPointer<obs_source_t>(cd, "source");
	if (!input)
		return;

	if (obs_source_get_type(input) != OBS_SOURCE_TYPE_INPUT)
		return;

	uint32_t flags = obs_source_get_output_flags(input);
	if ((flags & OBS_SOURCE_AUDIO) == 0)
		return;

	std::unique_lock<std::mutex> l(c->_meterMutex);
	c->_meters.emplace_back(std::move(new Meter(input)));
}

void Utils::Obs::VolumeMeter::Handler::InputDeactivateCallback(void *priv_data, calldata_t *cd)
{
	auto c = static_cast<Handler *>(priv_data);

	obs_source_t *input = GetCalldataPointer<obs_source_t>(cd, "source");
	if (!input)
		return;

	if (obs_source_get_type(input) != OBS_SOURCE_TYPE_INPUT)
		return;

	// Don't ask me why, but using std::remove_if segfaults trying this.
	std::unique_lock<std::mutex> l(c->_meterMutex);
	std::vector<MeterPtr>::iterator iter;
	for (iter = c->_meters.begin(); iter != c->_meters.end();) {
		if (obs_weak_source_references_source(iter->get()->GetWeakInput(), input))
			iter = c->_meters.erase(iter);
		else
			++iter;
	}
}
