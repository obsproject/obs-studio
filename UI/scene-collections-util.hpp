/******************************************************************************
    Copyright (C) 2023 by Sebastian Beckmann

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

#include <functional>
#include <string>

void EnumSceneCollections(std::function<bool(const char *, const char *)> &&cb);

typedef enum {
	kSceneCollectionOrderLastUsed,
	kSceneCollectionOrderName
} SceneCollectionOrder;
void EnumSceneCollectionsOrdered(
	SceneCollectionOrder order,
	std::function<bool(const char *, const char *, time_t)> &&cb);
bool SceneCollectionExists(const std::string &name);

bool CreateSceneCollection(const std::string &name);
bool RenameSceneCollection(std::string old_path, const std::string &new_name);
bool DuplicateSceneCollection(const std::string &path,
			      const std::string &new_name);
bool DeleteSceneCollection(std::string path);
bool ExportSceneCollection(const std::string &path,
			   const std::string &new_file);
