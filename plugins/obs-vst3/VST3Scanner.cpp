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
#include "VST3Scanner.h"
#include "util/bmem.h"
#include <util/platform.h>

std::vector<std::string> VST3Scanner::getDefaultSearchPaths()
{
	std::vector<std::string> paths;

#ifdef _WIN32
	char *programFiles = std::getenv("ProgramFiles");
	if (programFiles)
		paths.emplace_back(std::string(programFiles) + "\\Common Files\\VST3");

	char *localAppData = std::getenv("LOCALAPPDATA");
	if (localAppData)
		paths.emplace_back(std::string(localAppData) + "\\Programs\\Common\\VST3");
#elif defined(__APPLE__)
	paths.emplace_back("/Library/Audio/Plug-Ins/VST3");
	if (const char *home = std::getenv("HOME"))
		paths.emplace_back(std::string(home) + "/Library/Audio/Plug-Ins/VST3");
#elif defined(__linux__)
	paths.emplace_back("/usr/lib/vst3");
	paths.emplace_back("/usr/local/lib/vst3");
	if (const char *home = std::getenv("HOME"))
		paths.emplace_back(std::string(home) + "/.vst3");
#endif
	return paths;
}

// this retrieves the location of all VST3s (files on windows and dirs on other OSes)
std::unordered_set<std::string> VST3Scanner::getVST3Paths()
{
	std::unordered_set<std::string> fsPaths;

	for (const auto &folder : getDefaultSearchPaths()) {
		if (!std::filesystem::exists(folder))
			continue;

		for (const auto &entry : std::filesystem::recursive_directory_iterator(folder)) {
			if (!entry.exists())
				continue;

#if defined(_WIN32)
			if (entry.is_regular_file() && entry.path().extension() == ".vst3")
#else
			if (entry.is_directory() && entry.path().extension() == ".vst3")
#endif
				fsPaths.insert(entry.path().string());
		}
	}

	return fsPaths;
}

static std::string lowerAscii(std::string s) noexcept
{
	for (auto &c : s)
		if (c >= 'A' && c <= 'Z')
			c = static_cast<char>(c - 'A' + 'a');
	return s;
}

// Alphabetical sorting of vst3 classes; note that a VST3 plugin can have multiple classes (ex: LSP). The sorting is
// done across VST3s. In case of multiple classes, the VST3 name is appended.
void VST3Scanner::sort()
{
	std::sort(pluginList.begin(), pluginList.end(), [](const VST3ClassInfo &a, const VST3ClassInfo &b) {
		const auto an = lowerAscii(a.name);
		const auto bn = lowerAscii(b.name);
		if (an != bn)
			return an < bn;

		const auto ap = lowerAscii(a.pluginName);
		const auto bp = lowerAscii(b.pluginName);
		if (ap != bp)
			return ap < bp;

		if (a.path != b.path)
			return a.path < b.path;

		return a.id < b.id;
	});
}

bool VST3Scanner::hasVST3()
{
	auto paths = getDefaultSearchPaths();

	for (const auto &folder : paths) {
		if (!std::filesystem::exists(folder))
			continue;

		try {
			for (const auto &entry : std::filesystem::directory_iterator(folder)) {
				if (!entry.exists())
					continue;

				const auto &p = entry.path();

#if defined(_WIN32)
				if (entry.is_regular_file() && p.extension() == ".vst3")
					return true;
#elif defined(__APPLE__) || defined(__linux__)
				if (entry.is_directory() && p.extension() == ".vst3")
					return true;
#endif
			}
		} catch (const std::exception &e) {
			(void)e;
		}
	}

	return false;
}

// try to load the moduleinfo.json when provided by VST3
bool VST3Scanner::tryReadModuleInfo(const std::string &bundlePath)
{
	namespace fs = std::filesystem;

	fs::path p(bundlePath);
	fs::path bundleRoot;

#ifdef _WIN32
	if (!fs::is_regular_file(p))
		return false;

	// path is <bundleRoot>/Contents/Resources/moduleinfo.json
	bundleRoot = p.parent_path().parent_path().parent_path();
#else
	if (!fs::is_directory(p))
		return false;

	bundleRoot = p;
#endif

	fs::path jsonPath = bundleRoot / "Contents" / "Resources" / "moduleinfo.json";

	if (!fs::exists(jsonPath) || !fs::is_regular_file(jsonPath))
		return false;

	char *jsonBuf = os_quick_read_utf8_file(jsonPath.string().c_str());
	if (!jsonBuf)
		return false;

	std::string json(jsonBuf);
	bfree(jsonBuf);

	auto parsed = Steinberg::ModuleInfoLib::parseJson(json, nullptr);
	if (!parsed)
		return false;

	bool discardable = parsed.value().factoryInfo.flags & Steinberg::PFactoryInfo::kClassesDiscardable;

	if (discardable)
		return addModuleClasses(bundlePath);

	return loadFromModuleInfo(*parsed, bundleRoot.string());
}

// load classes from moduleinfo.json
bool VST3Scanner::loadFromModuleInfo(const Steinberg::ModuleInfo &info, const std::string &bundleRoot)
{
	const std::string pluginName = std::filesystem::path(bundleRoot).stem().string();
	size_t added = 0;
	bool discardable = info.factoryInfo.flags & Steinberg::PFactoryInfo::kClassesDiscardable;
	for (const auto &c : info.classes) {
		// We only accept audio effects, not MIDI nor instruments ...
		if (c.category != kVstAudioEffectClass)
			continue;

		VST3ClassInfo entry;
		entry.id = c.cid;
		entry.name = c.name;
		entry.path = bundleRoot;
		entry.pluginName = pluginName;
		entry.discardable = discardable;

		pluginList.push_back(std::move(entry));
		++classCount[bundleRoot];
		++added;
	}

	return added > 0;
}

// this updates the json in case some VST3s were installed or removed since last obs startup
void VST3Scanner::updateModulesList(std::unordered_map<std::string, ModuleCache> &modules,
				    const std::unordered_set<std::string> &cachedPaths)
{
	std::unordered_set<std::string> fsPaths = getVST3Paths();

	for (auto it = modules.begin(); it != modules.end();) {
		if (!fsPaths.count(it->first))
			it = modules.erase(it);
		else
			++it;
	}

	for (const auto &path : fsPaths) {
		if (!cachedPaths.count(path)) {
			if (!tryReadModuleInfo(path))
				addModuleClasses(path);
		}
	}
}

// load classes directly from binary; very close to previous function, a pity that factorization is not possible.
bool VST3Scanner::addModuleClasses(const std::string &bundlePath)
{
	std::string error;
	const std::string pluginName = std::filesystem::path(bundlePath).stem().string();
	size_t added = 0;
	VST3::Hosting::Module::Ptr module = VST3::Hosting::Module::create(bundlePath, error);

	if (!module) {
		blog(LOG_ERROR, "[VST3 Scanner] Module failed to load with error %s", error.c_str());
		return false;
	}

	VST3::Hosting::PluginFactory factory = module->getFactory();
	bool discardable = factory.info().classesDiscardable();
	for (const auto &classInfo : factory.classInfos()) {
		if (classInfo.category() == kVstAudioEffectClass) {
			VST3ClassInfo entry;
			entry.id = classInfo.ID().toString();
			entry.name = classInfo.name();
			entry.pluginName = pluginName;
			entry.path = bundlePath;
			entry.discardable = discardable;

			pluginList.push_back(std::move(entry));
			++classCount[bundlePath];
			++added;
		}
	}

	return added > 0;
}

// scan done by fully loading the module, very costly in time so this is done on a separate thread
bool VST3Scanner::scanForVST3Plugins()
{
	pluginList.clear();
	classCount.clear();
	auto paths = getVST3Paths();

	for (const auto &bundlePath : paths) {
		if (!tryReadModuleInfo(bundlePath))
			addModuleClasses(bundlePath);
	}
	sort();
	return !pluginList.empty();
}

bool VST3Scanner::ModWithMultipleClasses(const std::string &bundlePath) const
{
	auto it = classCount.find(bundlePath);
	return it != classCount.end() && it->second > 1;
}

std::string VST3Scanner::getNameById(const std::string &class_id) const
{
	for (const auto &c : pluginList)
		if (c.id == class_id)
			if (c.id == class_id)
				return c.path;
	return {};
}

std::string VST3Scanner::getPathById(const std::string &class_id) const
{
	for (const auto &c : pluginList)
		if (c.id == class_id)
			return c.path;
	return {};
}
