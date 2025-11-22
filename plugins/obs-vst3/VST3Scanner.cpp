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
#include "VST3Scanner.h"
#include <iterator>
#include <fstream>

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

bool VST3Scanner::tryReadModuleInfo(const std::string &bundlePath)
{
	namespace fs = std::filesystem;

	fs::path p(bundlePath);
	fs::path bundleRoot;

#ifdef _WIN32
	if (!fs::is_regular_file(p))
		return false;

	bundleRoot = p.parent_path().parent_path().parent_path();
#else
	if (!fs::is_directory(p))
		return false;

	bundleRoot = p;
#endif
	fs::path jsonPath = bundleRoot / "Contents" / "Resources" / "moduleinfo.json";

	if (!fs::exists(jsonPath) || !fs::is_regular_file(jsonPath))
		return false;

	std::ifstream f(jsonPath.string(), std::ios::binary);
	if (!f.is_open())
		return false;

	std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

	auto parsed = Steinberg::ModuleInfoLib::parseJson(json, nullptr);
	if (!parsed)
		return false;

	return loadFromModuleInfo(*parsed, bundleRoot.string());
}

bool VST3Scanner::loadFromModuleInfo(const Steinberg::ModuleInfo &info, const std::string &bundleRoot)
{
	const std::string pluginName = std::filesystem::path(bundleRoot).stem().string();

	for (const auto &c : info.classes) {
		// Only accept audio classes
		if (c.category != kVstAudioEffectClass)
			continue;

		VST3ClassInfo entry;
		entry.id = c.cid;
		entry.name = c.name;
		entry.path = bundleRoot;
		entry.pluginName = pluginName;

		pluginList.push_back(std::move(entry));
		++classCount[bundleRoot];
	}

	return true;
}

bool VST3Scanner::scanForVST3Plugins()
{
	pluginList.clear();
	classCount.clear();
	auto paths = getDefaultSearchPaths();

	for (const auto &folder : paths) {
		if (!std::filesystem::exists(folder))
			continue;
		for (const auto &entry : std::filesystem::recursive_directory_iterator(folder)) {
#ifdef _WIN32
			if (entry.is_regular_file() && entry.path().extension() == ".vst3")
#else
			if (entry.is_directory() && entry.path().extension() == ".vst3")
#endif
			{
				const std::string bundlePath = entry.path().string();
				const std::string bundleName = entry.path().stem().string();

				// --- Scan for classes inside this VST3
				bool ok = tryReadModuleInfo(bundlePath);
				if (!ok) {
					std::string error;
					auto module = VST3::Hosting::Module::create(bundlePath, error);
					if (module) {
						auto factory = module->getFactory();
						VST3::Hosting::PluginFactory hostFactory(factory);
						for (const auto &classInfo : hostFactory.classInfos()) {
							if (classInfo.category() == kVstAudioEffectClass) {
								VST3ClassInfo klass;
								klass.id = classInfo.ID().toString();
								klass.name = classInfo.name();
								klass.pluginName = bundleName;
								klass.path = bundlePath;
								pluginList.push_back(std::move(klass));
								++classCount[bundlePath];
							}
						}
					}
				} else {
					//printf("[VST3] detected through moduleinfo json !");
				}
			}
		}
	}
	sort();
	return !pluginList.empty();
}

bool VST3Scanner::bundle_has_multiple_classes(const std::string &bundlePath) const
{
	auto it = classCount.find(bundlePath);
	return it != classCount.end() && it->second > 1;
}
std::string VST3Scanner::get_name_by_id(const std::string &class_id) const
{
	for (const auto &c : pluginList)
		if (c.id == class_id)
			if (c.id == class_id)
				return c.path;
	return {};
}

std::string VST3Scanner::get_path_by_id(const std::string &class_id) const
{
	for (const auto &c : pluginList)
		if (c.id == class_id)
			return c.path;
	return {};
}
