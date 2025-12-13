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

#include "SceneCollection.hpp"

static constexpr std::string_view invalidCoordinateMode = "Invalid coordinate mode provided";

namespace OBS {
std::string SceneCollection::getName() const
{
	return name_;
}

std::string SceneCollection::getFileName() const
{
	return filePath_.filename().u8string();
}

std::string SceneCollection::getFileStem() const
{
	return filePath_.stem().u8string();
}

std::filesystem::path SceneCollection::getFilePath() const
{
	return filePath_;
}

std::string SceneCollection::getFilePathString() const
{
	return filePath_.u8string();
}

SceneCoordinateMode SceneCollection::getCoordinateMode() const
{
	return coordinateMode_;
}

void SceneCollection::setCoordinateMode(SceneCoordinateMode newMode)
{
	if (newMode != SceneCoordinateMode::Invalid) {
		coordinateMode_ = newMode;
	} else {
		throw std::invalid_argument(invalidCoordinateMode.data());
	}
}

Rect SceneCollection::getMigrationResolution() const
{
	return migrationResolution_;
}

void SceneCollection::setMigrationResolution(Rect newResolution)
{
	migrationResolution_ = newResolution;
}
} // namespace OBS
