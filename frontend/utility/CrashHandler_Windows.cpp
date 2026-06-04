/******************************************************************************
 Copyright (C) 2025 by Patrick Heyer <PatTheMav@users.noreply.github.com>
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "CrashHandler.hpp"
#include <OBSApp.hpp>

#include <util/util.hpp>

#include <vector>

namespace OBS {

using CrashFileEntry = std::pair<std::filesystem::path, std::filesystem::file_time_type>;

PlatformType CrashHandler::getPlatformType() const
{
	return PlatformType::Windows;
}

std::filesystem::path CrashHandler::findLastCrashLog() const
{
	std::filesystem::path lastCrashLogFile;

	std::filesystem::path crashLogDirectory = getCrashLogDirectory();

	if (!std::filesystem::exists(crashLogDirectory)) {
		blog(LOG_ERROR, "Crash log directory '%s' does not exist", crashLogDirectory.u8string().c_str());

		return lastCrashLogFile;
	}

	std::vector<CrashFileEntry> crashLogFiles;

	for (const auto &entry : std::filesystem::directory_iterator(crashLogDirectory)) {
		if (entry.is_directory()) {
			continue;
		}

		std::string entryFileName = entry.path().filename().u8string();

		if (entryFileName.rfind("Crash ", 0) != 0) {
			continue;
		}

		CrashFileEntry crashLogFile =
			CrashFileEntry(entry.path(), std::filesystem::last_write_time(entry.path()));

		crashLogFiles.push_back(crashLogFile);
	}

	std::sort(crashLogFiles.begin(), crashLogFiles.end(),
		  [](CrashFileEntry &lhs, CrashFileEntry &rhs) { return lhs.second > rhs.second; });

	if (crashLogFiles.size() > 0) {
		lastCrashLogFile = crashLogFiles.front().first;
	}

	return lastCrashLogFile;
}

std::filesystem::path CrashHandler::getCrashLogDirectory() const
{
	BPtr crashLogDirectory = GetAppConfigPathPtr("obs-studio/crashes");

	std::string crashLogDirectoryString = crashLogDirectory.Get();

	std::filesystem::path crashLogDirectoryPath = std::filesystem::u8path(crashLogDirectoryString);

	return crashLogDirectoryPath;
}
} // namespace OBS
