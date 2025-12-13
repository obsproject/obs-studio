/*
obs-websocket
Copyright (C) 2016-2021 Stephane Lepin <stephane.lepin@gmail.com>
Copyright (C) 2020-2021 Kyle Manning <tt2468@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <inttypes.h>
#include <QString>

#include <obs-module.h>

#include <util/util.hpp>

#include "Obs.h"
#include "plugin-macros.generated.h"

std::string Utils::Obs::StringHelper::GetObsVersion()
{
	uint32_t version = obs_get_version();

	uint8_t major, minor, patch;
	major = (version >> 24) & 0xFF;
	minor = (version >> 16) & 0xFF;
	patch = version & 0xFF;

	QString combined = QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
	return combined.toStdString();
}

std::string Utils::Obs::StringHelper::GetModuleConfigPath(std::string fileName)
{
	BPtr<char> configPath = obs_module_config_path(fileName.c_str());
	return std::string(configPath.Get());
}

std::string Utils::Obs::StringHelper::GetCurrentSceneCollection()
{
	BPtr<char> sceneCollectionName = obs_frontend_get_current_scene_collection();
	return std::string(sceneCollectionName.Get());
}

std::string Utils::Obs::StringHelper::GetCurrentProfile()
{
	BPtr<char> profileName = obs_frontend_get_current_profile();
	return std::string(profileName.Get());
}

std::string Utils::Obs::StringHelper::GetCurrentProfilePath()
{
	BPtr<char> profilePath = obs_frontend_get_current_profile_path();
	return std::string(profilePath.Get());
}

std::string Utils::Obs::StringHelper::GetCurrentRecordOutputPath()
{
	BPtr<char> recordOutputPath = obs_frontend_get_current_record_output_path();
	return std::string(recordOutputPath.Get());
}

std::string Utils::Obs::StringHelper::GetLastRecordFileName()
{
	OBSOutputAutoRelease output = obs_frontend_get_recording_output();
	if (!output)
		return "";

	OBSDataAutoRelease outputSettings = obs_output_get_settings(output);

	obs_data_item_t *item = obs_data_item_byname(outputSettings, "url");
	if (!item) {
		item = obs_data_item_byname(outputSettings, "path");
		if (!item)
			return "";
	}

	std::string ret = obs_data_item_get_string(item);
	obs_data_item_release(&item);
	return ret;
}

std::string Utils::Obs::StringHelper::GetLastReplayBufferFileName()
{
	BPtr<char> replayBufferPath = obs_frontend_get_last_replay();
	return std::string(replayBufferPath.Get());
}

std::string Utils::Obs::StringHelper::GetLastScreenshotFileName()
{
	BPtr<char> screenshotPath = obs_frontend_get_last_screenshot();
	return std::string(screenshotPath.Get());
}

std::string Utils::Obs::StringHelper::DurationToTimecode(uint64_t ms)
{
	uint64_t secs = ms / 1000ULL;
	uint64_t minutes = secs / 60ULL;

	uint64_t hoursPart = minutes / 60ULL;
	uint64_t minutesPart = minutes % 60ULL;
	uint64_t secsPart = secs % 60ULL;
	uint64_t msPart = ms % 1000ULL;

	QString formatted =
		QString::asprintf("%02" PRIu64 ":%02" PRIu64 ":%02" PRIu64 ".%03" PRIu64, hoursPart, minutesPart, secsPart, msPart);
	return formatted.toStdString();
}
