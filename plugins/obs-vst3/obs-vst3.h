/******************************************************************************
    Copyright (C) 2025-2026 pkv <pkv@obsproject.com>
    This file is part of obs-vst3.
    It uses the Steinberg VST3 SDK, which is licensed under MIT license.
    See https://github.com/steinbergmedia/vst3sdk for details.
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include <obs-module.h>
#include <media-io/audio-resampler.h>
#include <media-io/audio-io.h>
#include <util/darray.h>
#include <util/deque.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

constexpr int kMaxPreprocessorChannels = 8;
constexpr int kMaxSidechainChannels = 2;
constexpr int64_t kBufferSizeMilliseconds = 10;

class VST3HostApp;
class VST3Plugin;

VST3HostApp *getHostApp() noexcept;

struct Vst3AudioData {
	obs_source_t *context = nullptr;

	std::shared_ptr<VST3Plugin> plugin = nullptr;
	std::string vst3Id;
	std::string vst3Path;
	std::string vst3Name;

	int sampleRate = 48000;
	size_t frames = 480;
	size_t channels = 2;
	speaker_layout layout = SPEAKERS_STEREO;
	int64_t runningSampleCount = 0;
	uint64_t systemTime = 0;
	uint64_t lastTimestamp = 0;
	uint64_t latency = 0;

	struct deque infoBuffer;
	struct deque inputBuffers[kMaxPreprocessorChannels];
	struct deque outputBuffers[kMaxPreprocessorChannels];
	struct deque sidechainInputBuffers[kMaxPreprocessorChannels];

	float *copyBuffers[kMaxPreprocessorChannels];
	float *sidechainCopyBuffers[kMaxPreprocessorChannels];

	struct obs_audio_data outputAudio;
	DARRAY(float) outputData;

	std::atomic<bool> bypass = true;
	std::atomic<bool> sidechainEnabled = false;
	std::atomic<bool> noview = true;
	std::atomic_flag initInProgress = ATOMIC_FLAG_INIT;

	std::atomic<bool> hasSidechain = false;
	obs_weak_source_t *weakSidechain = nullptr;
	std::string sidechainName;
	uint64_t sidechainCheckTime = 0;
	std::shared_ptr<audio_resampler> sidechainResampler;
	size_t sidechainChannels = 0;
	uint64_t sidechainLastTimestamp = 0;
	std::mutex sidechainUpdateMutex;
	std::mutex sidechainMutex;

	bool lastInitFailed = false;
};
