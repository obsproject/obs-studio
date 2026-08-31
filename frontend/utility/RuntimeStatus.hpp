/******************************************************************************
    Copyright (C) 2026 by nicolaeser <nico@laeser.software>

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

#pragma once

#include <obs-frontend-api.h>

#include <filesystem>
#include <string>

namespace OBS {

class RuntimeStatus {
public:
	RuntimeStatus();
	~RuntimeStatus();

	RuntimeStatus(const RuntimeStatus &) = delete;
	RuntimeStatus &operator=(const RuntimeStatus &) = delete;
	RuntimeStatus(RuntimeStatus &&) = delete;
	RuntimeStatus &operator=(RuntimeStatus &&) = delete;

	void handleFrontendEvent(enum obs_frontend_event event);
	void shutdown() noexcept;

private:
	void initialize();
	bool tryInitializeDirectory(const std::filesystem::path &directory);
	void removeStaleFiles();
	void writeMarker(const char *name);
	void removeMarker(const char *name);
	void setMarker(const char *name, bool isActive);
	std::filesystem::path markerPath(const char *name) const;

	int processId_ = 0;
	std::string pidText_;
	std::filesystem::path statusDirectory_;
	bool isActive_ = false;
};

} // namespace OBS
