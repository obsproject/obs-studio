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

#pragma once

#include "Rect.hpp"

#include <string>
#include <filesystem>

namespace OBS {

enum class SceneCoordinateMode { Invalid, Absolute, Relative };

class SceneCollection {
private:
	std::string name_;
	std::filesystem::path filePath_;

	SceneCoordinateMode coordinateMode_ = SceneCoordinateMode::Relative;
	Rect migrationResolution_;

public:
	SceneCollection() = default;
	SceneCollection(const std::string &name, const std::filesystem::path &filePath)
		: name_(name),
		  filePath_(filePath) {};

	std::string getName() const;
	std::string getFileName() const;
	std::string getFileStem() const;
	std::filesystem::path getFilePath() const;
	std::string getFilePathString() const;

	SceneCoordinateMode getCoordinateMode() const;
	void setCoordinateMode(SceneCoordinateMode newMode);

	Rect getMigrationResolution() const;
	void setMigrationResolution(Rect newResolution);
	template<typename widthType, typename heightType>
	void setMigrationResolution(widthType width, heightType height)
	{
		this->migrationResolution_ = Rect(width, height);
	}

	inline int getVersion() const { return (coordinateMode_ == SceneCoordinateMode::Relative) ? 2 : 1; };

	bool empty() const { return name_.empty() || filePath_.empty(); }
};
} // namespace OBS
