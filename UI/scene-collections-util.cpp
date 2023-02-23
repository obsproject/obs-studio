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

#include "scene-collections-util.hpp"

#include <sys/stat.h>
#include <string>

#include <util/platform.h>
#include <util/dstr.hpp>
#include <obs-app.hpp>

#include "window-basic-main.hpp"

#ifdef _MSC_VER
#define strcasecmp stricmp
#endif

void EnumSceneCollections(std::function<bool(const char *, const char *)> &&cb)
{
	char path[512];
	os_glob_t *glob;

	int ret = GetConfigPath(path, sizeof(path),
				"obs-studio/basic/scenes/*.json");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get config path for scene "
				  "collections");
		return;
	}

	if (os_glob(path, 0, &glob) != 0) {
		blog(LOG_WARNING, "Failed to glob scene collections");
		return;
	}

	for (size_t i = 0; i < glob->gl_pathc; i++) {
		const char *filePath = glob->gl_pathv[i].path;

		if (glob->gl_pathv[i].directory)
			continue;

		OBSDataAutoRelease data =
			obs_data_create_from_json_file_safe(filePath, "bak");
		std::string name = obs_data_get_string(data, "name");

		/* if no name found, use the file name as the name
		 * (this only happens when switching to the new version) */
		if (name.empty()) {
			name = strrchr(filePath, '/') + 1;
			name.resize(name.size() - 5);
		}

		if (!cb(name.c_str(), filePath))
			break;
	}

	os_globfree(glob);
}

void EnumSceneCollectionsOrdered(
	SceneCollectionOrder order,
	std::function<bool(const char *, const char *, time_t)> &&cb)
{
	struct collections_info {
		char *name;
		char *file;
		time_t last_used;
	};

	std::vector<collections_info> collections;

	EnumSceneCollections([&](const char *name, const char *file) {
		struct stat stats;
		if (os_stat(file, &stats) != 0)
			stats.st_atime = 0;

		struct collections_info info {
			bstrdup(name), bstrdup(file), stats.st_atime
		};
		collections.push_back(info);
		return true;
	});

	BPtr current_collection = obs_frontend_get_current_scene_collection();
	std::sort(
		collections.begin(), collections.end(),
		[&](const struct collections_info &left,
		    const struct collections_info &right) {
			switch (order) {
			case kSceneCollectionOrderName:
				return strcasecmp(left.name, right.name) < 0;
			case kSceneCollectionOrderLastUsed:
				/* Unfortunately the atime doesn't care about reads so we have
				 * to manually put the currently active one first as it might
				 * not have been written to. */
				if (strcmp(left.name, current_collection) == 0)
					return true;
				if (strcmp(right.name, current_collection) == 0)
					return false;

				return left.last_used >= right.last_used;
			}
			return false;
		});

	for (auto collection : collections) {
		if (!cb(collection.name, collection.file, collection.last_used))
			break;
	}

	for (auto collection : collections) {
		bfree(collection.name);
		bfree(collection.file);
	}
}

bool SceneCollectionExists(const std::string &name)
{
	bool found = false;
	auto func = [&](const char *sc_name, const char *) {
		if (sc_name == name) {
			found = true;
			return false;
		}
		return true;
	};

	EnumSceneCollections(func);
	return found;
}

static obs_data_t *GenerateEmptySceneCollection(const std::string &name)
{

	OBSDataAutoRelease defaultScene = obs_data_create();
	const char *scene_name = Str("Basic.Scene");
	obs_data_set_string(defaultScene, "id", "scene");
	obs_data_set_string(defaultScene, "name", scene_name);
	obs_data_set_int(defaultScene, "prev_ver", LIBOBS_API_VER);
	OBSDataArrayAutoRelease sources = obs_data_array_create();
	obs_data_array_push_back(sources, defaultScene);

	OBSDataArrayAutoRelease quick_transitions = obs_data_array_create();
	auto add_quick_transition = [&](const char *name, bool fade_to_black,
					int id) {
		OBSDataAutoRelease transition = obs_data_create();
		obs_data_set_int(transition, "duration", 300);
		obs_data_set_bool(transition, "fade_to_black", fade_to_black);
		obs_data_set_string(transition, "name", name);
		obs_data_set_int(transition, "id", id);
		obs_data_array_push_back(quick_transitions, transition);
	};
	add_quick_transition(obs_source_get_display_name("cut_transition"),
			     false, 1);
	add_quick_transition(obs_source_get_display_name("fade_transition"),
			     false, 2);
	add_quick_transition(obs_source_get_display_name("fade_transition"),
			     true, 3);

	OBSData collection = obs_data_create();
	obs_data_set_array(collection, "sources", sources);
	obs_data_set_string(collection, "current_scene", scene_name);
	obs_data_set_int(collection, "transition_duration", 300);
	obs_data_set_string(collection, "name", name.c_str());
	obs_data_set_array(collection, "quick_transitions", quick_transitions);

	return collection;
}

bool CreateSceneCollection(const std::string &name)
{
	if (SceneCollectionExists(name))
		return false;

	std::string file;
	if (!GetUnusedSceneCollectionFile(name, file))
		return false;

	OBSDataAutoRelease saveData = GenerateEmptySceneCollection(name);

	DStr relative_path;
	dstr_printf(relative_path, "obs-studio/basic/scenes/%s.json",
		    file.c_str());

	char savePath[1024];
	GetConfigPath(savePath, sizeof(savePath), relative_path);
	if (!obs_data_save_json_safe(saveData, savePath, "tmp", "bak")) {
		blog(LOG_ERROR, "Could not save scene data to %s", savePath);
		return false;
	}

	blog(LOG_INFO, "Created new scene collection '%s' (%s.json)",
	     name.c_str(), file.c_str());

	return true;
}

static inline bool CurrentlyActive(const std::string &full_path)
{
	bool active = true;
	BPtr current_collection = obs_frontend_get_current_scene_collection();
	EnumSceneCollections([&](const char *name, const char *file) {
		if (strcmp(full_path.c_str(), file) == 0) {
			active = strcmp(current_collection, name) == 0;
			return false;
		}
		return true;
	});
	return active;
}

bool RenameSceneCollection(std::string old_path, const std::string &new_name)
{
	if (SceneCollectionExists(new_name))
		return false;

	std::string new_file_name;
	if (!GetUnusedSceneCollectionFile(new_name, new_file_name))
		return false;

	if (CurrentlyActive(old_path)) {
		// Save current project in case changes got made since last save
		OBSBasic *main =
			static_cast<OBSBasic *>(App()->GetMainWindow());
		main->SaveProjectNow();

		// Change names in current config
		config_set_string(App()->GlobalConfig(), "Basic",
				  "SceneCollection", new_name.c_str());
		config_set_string(App()->GlobalConfig(), "Basic",
				  "SceneCollectionFile", new_file_name.c_str());
	}

	OBSDataAutoRelease collectionData =
		obs_data_create_from_json_file(old_path.c_str());
	if (!collectionData)
		return false;

	BPtr old_name = bstrdup(obs_data_get_string(collectionData, "name"));
	obs_data_set_string(collectionData, "name", new_name.c_str());

	DStr new_path_relative;
	dstr_printf(new_path_relative, "obs-studio/basic/scenes/%s.json",
		    new_file_name.c_str());
	char new_path_absolute[1024];
	GetConfigPath(new_path_absolute, sizeof(new_path_absolute),
		      new_path_relative);

	if (!obs_data_save_json_safe(collectionData, new_path_absolute, "tmp",
				     "bak")) {
		blog(LOG_ERROR, "Could not duplicate scene data to %s",
		     new_path_absolute);
		return false;
	}

	os_unlink(old_path.c_str());
	old_path += ".bak";
	os_unlink(old_path.c_str());

	blog(LOG_INFO, "Renamed scene collection '%s' to '%s' (%s.json)",
	     old_name.Get(), new_name.c_str(), new_file_name.c_str());

	return true;
}

bool DuplicateSceneCollection(const std::string &path,
			      const std::string &new_name)
{
	if (SceneCollectionExists(new_name))
		return false;

	std::string duplicate_file_name;
	if (!GetUnusedSceneCollectionFile(new_name, duplicate_file_name))
		return false;

	if (CurrentlyActive(path)) {
		// Save current project in case changes got made since last save
		OBSBasic *main =
			static_cast<OBSBasic *>(App()->GetMainWindow());
		main->SaveProjectNow();
	}

	OBSDataAutoRelease collectionData =
		obs_data_create_from_json_file(path.c_str());
	if (!collectionData)
		return false;

	BPtr original_name =
		bstrdup(obs_data_get_string(collectionData, "name"));
	obs_data_set_string(collectionData, "name", new_name.c_str());

	/* Source UUIDs should be changed when a scene collection is duplicated so
	 * that there are no duplicate UUIDs. Unfortunately, this means that the
	 * scene item belonging to the sources have to change as well, bringing us
	 * this beatuty: */
	std::map<std::string, std::string> uuid_map;
	OBSDataArrayAutoRelease sources =
		obs_data_get_array(collectionData, "sources");
	auto regenerateUuidsAndMakeMap = [](obs_data_t *source, void *opaque) {
		auto uuid_map =
			static_cast<std::map<std::string, std::string> *>(
				opaque);
		const char *old_uuid = obs_data_get_string(source, "uuid");
		char *new_uuid = os_generate_uuid();
		uuid_map->insert(std::pair(old_uuid, new_uuid));

		obs_data_set_string(source, "uuid", new_uuid);
		bfree(new_uuid);
	};
	obs_data_array_enum(sources, regenerateUuidsAndMakeMap, &uuid_map);
	auto rewireSceneItems = [](obs_data_t *source, void *opaque) {
		if (strcmp(obs_data_get_string(source, "id"), "scene") != 0)
			return;

		OBSDataAutoRelease settings =
			obs_data_get_obj(source, "settings");
		OBSDataArrayAutoRelease items =
			obs_data_get_array(settings, "items");

		auto rewireItem = [](obs_data_t *item, void *opaque) {
			auto uuid_map = static_cast<
				std::map<std::string, std::string> *>(opaque);
			const char *original_uuid =
				obs_data_get_string(item, "source_uuid");
			std::string new_uuid = uuid_map->at(original_uuid);
			obs_data_set_string(item, "source_uuid",
					    new_uuid.c_str());
		};
		obs_data_array_enum(items, rewireItem, opaque);
	};
	obs_data_array_enum(sources, rewireSceneItems, &uuid_map);

	DStr duplicate_path_relative;
	dstr_printf(duplicate_path_relative, "obs-studio/basic/scenes/%s.json",
		    duplicate_file_name.c_str());
	char duplicate_path_absolute[1024];
	GetConfigPath(duplicate_path_absolute, sizeof(duplicate_path_absolute),
		      duplicate_path_relative);

	if (!obs_data_save_json_safe(collectionData, duplicate_path_absolute,
				     "tmp", "bak")) {
		blog(LOG_ERROR, "Could not duplicate scene data to %s",
		     duplicate_path_absolute);
		return false;
	}

	blog(LOG_INFO, "Duplicated scene collection '%s' as '%s' (%s.json)",
	     original_name.Get(), new_name.c_str(),
	     duplicate_file_name.c_str());

	return true;
}

bool DeleteSceneCollection(std::string path)
{
	if (CurrentlyActive(path))
		return false;

	OBSDataAutoRelease collectionData =
		obs_data_create_from_json_file(path.c_str());
	if (!collectionData)
		return false;

	const char *name = obs_data_get_string(collectionData, "name");

	/* os_rename() overwrites if necessary, only the .bak file will remain. */
	os_rename(path.c_str(), (path + ".bak").c_str());
	blog(LOG_INFO, "Deleted scene collection '%s'", name);
	return true;
}

bool ExportSceneCollection(const std::string &path, const std::string &new_file)
{
	if (CurrentlyActive(path)) {
		// Save current project in case changes got made since last save
		OBSBasic *main =
			static_cast<OBSBasic *>(App()->GetMainWindow());
		main->SaveProjectNow();
	}

	OBSDataAutoRelease collection =
		obs_data_create_from_json_file(path.c_str());
	if (!collection)
		return false;

	OBSDataArrayAutoRelease sources =
		obs_data_get_array(collection, "sources");
	if (!sources) {
		blog(LOG_WARNING, "No sources in exported scene collection");
		return false;
	}
	obs_data_erase(collection, "sources");

	// We're just using std::sort on a vector to make life easier.
	std::vector<OBSData> sourceItems;
	obs_data_array_enum(
		sources,
		[](obs_data_t *data, void *pVec) -> void {
			auto &sourceItems =
				*static_cast<std::vector<OBSData> *>(pVec);
			sourceItems.push_back(data);
		},
		&sourceItems);

	std::sort(sourceItems.begin(), sourceItems.end(),
		  [](const OBSData &a, const OBSData &b) {
			  return astrcmpi(obs_data_get_string(a, "name"),
					  obs_data_get_string(b, "name")) < 0;
		  });

	OBSDataArrayAutoRelease newSources = obs_data_array_create();
	for (auto &item : sourceItems)
		obs_data_array_push_back(newSources, item);

	obs_data_set_array(collection, "sources", newSources);

	const char *name = obs_data_get_string(collection, "name");

	if (!obs_data_save_json_pretty_safe(collection, new_file.c_str(), "tmp",
					    "bak")) {
		blog(LOG_ERROR, "Could not export scene collection to %s",
		     new_file.c_str());
		return false;
	}

	blog(LOG_INFO, "Exported scene collection '%s' to '%s'", name,
	     new_file.c_str());
	return true;
}
