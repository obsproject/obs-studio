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
