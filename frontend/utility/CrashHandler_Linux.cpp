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

namespace OBS {

PlatformType CrashHandler::getPlatformType() const
{
	return PlatformType::Linux;
}

std::filesystem::path CrashHandler::findLastCrashLog() const
{
	std::filesystem::path lastCrashLogFile;

	return lastCrashLogFile;
}

std::filesystem::path CrashHandler::getCrashLogDirectory() const
{
	std::filesystem::path crashLogDirectoryPath;

	return crashLogDirectoryPath;
}
} // namespace OBS
