/* Copyright (c) 2025 pkv <pkv@obsproject.com>
 * This file is part of obs-vst3.
 *
 * Portions are derived from EasyVst (https://github.com/iffyloop/EasyVst),
 * licensed under Public Domain (Unlicense) or MIT No Attribution.
 *
 * This file uses the Steinberg VST3 SDK, which is licensed under MIT license.
 * See https://github.com/steinbergmedia/vst3sdk for details.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */
#pragma once
#include "pluginterfaces/vst/vsttypes.h"
#include <atomic>

using namespace Steinberg;
using namespace Vst;

struct ParamEdit {
	ParamID id;
	ParamValue value;
};

class VST3ParamEditQueue {
	static constexpr size_t CAP = 1024;
	ParamEdit buf[CAP];
	std::atomic<size_t> head{0}, tail{0};

public:
	bool push(const ParamEdit &e)
	{
		size_t h = head.load(std::memory_order_relaxed);
		size_t n = (h + 1) % CAP;
		if (n == tail.load(std::memory_order_acquire))
			return false;

		buf[h] = e;
		head.store(n, std::memory_order_release);
		return true;
	}
	bool pop(ParamEdit &out)
	{
		size_t t = tail.load(std::memory_order_relaxed);
		if (t == head.load(std::memory_order_acquire))
			return false;

		out = buf[t];
		tail.store((t + 1) % CAP, std::memory_order_release);
		return true;
	}
};
