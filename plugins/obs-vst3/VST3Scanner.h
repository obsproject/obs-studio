/* Copyright (c) 2025 pkv <pkv@obsproject.com>
 * This file is part of obs-vst3.
 *
 * This file uses the Steinberg VST3 SDK, which is licensed under MIT license.
 * See https://github.com/steinbergmedia/vst3sdk for details.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */
#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include "sdk/public.sdk/source/vst/hosting/module.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef kVstAudioEffectClass
#define kVstAudioEffectClass "Audio Module Class"
#endif

struct VST3ClassInfo {
	std::string name;
	std::string id;
	std::string path;
	std::string pluginName;
};

class VST3Scanner {
public:
	std::vector<std::string> getDefaultSearchPaths();
	std::string get_name_by_id(const std::string &class_id) const;
	std::string get_path_by_id(const std::string &class_id) const;
	bool bundle_has_multiple_classes(const std::string &bundlePath) const;
	bool hasVST3();
	bool scanForVST3Plugins();
	std::vector<VST3ClassInfo> pluginList;
	void sort();
	std::unordered_map<std::string, size_t> classCount;
};
