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
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/moduleinfo/moduleinfo.h"
#include "public.sdk/source/vst/moduleinfo/moduleinfoparser.h"
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
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
	bool discardable; // the classes need to be reloaded from the module at each host startup and can not be cached
};

struct ModuleCache {
	bool discardable = false;
	std::vector<VST3ClassInfo> classes;
};

class VST3Scanner {
public:
	std::vector<std::string> getDefaultSearchPaths();
	std::string getNameById(const std::string &class_id) const;
	std::string getPathById(const std::string &class_id) const;
	bool ModWithMultipleClasses(const std::string &bundlePath) const;
	bool hasVST3();
	bool addModuleClasses(const std::string &bundlePath);
	bool scanForVST3Plugins();
	std::vector<VST3ClassInfo> pluginList;
	void sort();
	std::unordered_map<std::string, size_t> classCount;
	void updateModulesList(std::unordered_map<std::string, ModuleCache> &modules,
			       const std::unordered_set<std::string> &cachedPaths);

private:
	std::unordered_set<std::string> getVST3Paths();
	bool tryReadModuleInfo(const std::string &bundlePath);
	bool loadFromModuleInfo(const Steinberg::ModuleInfo &info, const std::string &bundlePath);
};
