/******************************************************************************
    Copyright (C) 2025 pkv <pkv@obsproject.com>
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
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/moduleinfo/moduleinfo.h"
#include "public.sdk/source/vst/moduleinfo/moduleinfoparser.h"
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <algorithm>
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

private:
	bool tryReadModuleInfo(const std::string &bundlePath);
	bool loadFromModuleInfo(const Steinberg::ModuleInfo &info, const std::string &bundlePath);
};
