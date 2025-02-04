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
		  filePath_(filePath){};

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
