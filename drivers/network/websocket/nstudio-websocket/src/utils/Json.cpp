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

#include <fstream>

#include "Json.h"
#include "plugin-macros.generated.h"

bool Utils::Json::JsonArrayIsValidObsArray(const json &j)
{
	for (auto it : j) {
		if (!it.is_object())
			return false;
	}

	return true;
}

void obs_data_set_json_object_item(obs_data_t *d, json j);

void obs_data_set_json_object(obs_data_t *d, const char *key, json j)
{
	obs_data_t *subObj = obs_data_create();
	obs_data_set_json_object_item(subObj, j);
	obs_data_set_obj(d, key, subObj);
	obs_data_release(subObj);
}

void obs_data_set_json_array(obs_data_t *d, const char *key, json j)
{
	obs_data_array_t *array = obs_data_array_create();

	for (auto &[key, value] : j.items()) {
		if (!value.is_object())
			continue;

		obs_data_t *item = obs_data_create();
		obs_data_set_json_object_item(item, value);
		obs_data_array_push_back(array, item);
		obs_data_release(item);
	}

	obs_data_set_array(d, key, array);
	obs_data_array_release(array);
}

void obs_data_set_json_object_item(obs_data_t *d, json j)
{
	for (auto &[key, value] : j.items()) {
		if (value.is_object()) {
			obs_data_set_json_object(d, key.c_str(), value);
		} else if (value.is_array()) {
			obs_data_set_json_array(d, key.c_str(), value);
		} else if (value.is_string()) {
			obs_data_set_string(d, key.c_str(), value.get<std::string>().c_str());
		} else if (value.is_number_integer()) {
			obs_data_set_int(d, key.c_str(), value.get<int64_t>());
		} else if (value.is_number_float()) {
			obs_data_set_double(d, key.c_str(), value.get<double>());
		} else if (value.is_boolean()) {
			obs_data_set_bool(d, key.c_str(), value.get<bool>());
		}
	}
}

obs_data_t *Utils::Json::JsonToObsData(json j)
{
	obs_data_t *data = obs_data_create();

	if (!j.is_object()) {
		obs_data_release(data);
		return nullptr;
	}

	obs_data_set_json_object_item(data, j);

	return data;
}

void set_json_string(json *j, const char *name, obs_data_item_t *item)
{
	const char *val = obs_data_item_get_string(item);
	j->emplace(name, val);
}
void set_json_number(json *j, const char *name, obs_data_item_t *item)
{
	enum obs_data_number_type type = obs_data_item_numtype(item);

	if (type == OBS_DATA_NUM_INT) {
		long long val = obs_data_item_get_int(item);
		j->emplace(name, val);
	} else {
		double val = obs_data_item_get_double(item);
		j->emplace(name, val);
	}
}
void set_json_bool(json *j, const char *name, obs_data_item_t *item)
{
	bool val = obs_data_item_get_bool(item);
	j->emplace(name, val);
}
void set_json_object(json *j, const char *name, obs_data_item_t *item, bool includeDefault)
{
	obs_data_t *obj = obs_data_item_get_obj(item);
	j->emplace(name, Utils::Json::ObsDataToJson(obj, includeDefault));
	obs_data_release(obj);
}
void set_json_array(json *j, const char *name, obs_data_item_t *item, bool includeDefault)
{
	json jArray = json::array();
	obs_data_array_t *array = obs_data_item_get_array(item);
	size_t count = obs_data_array_count(array);

	for (size_t idx = 0; idx < count; idx++) {
		obs_data_t *subItem = obs_data_array_item(array, idx);
		json jItem = Utils::Json::ObsDataToJson(subItem, includeDefault);
		obs_data_release(subItem);
		jArray.push_back(jItem);
	}

	obs_data_array_release(array);
	j->emplace(name, jArray);
}

json Utils::Json::ObsDataToJson(obs_data_t *d, bool includeDefault)
{
	json j = json::object();
	obs_data_item_t *item = nullptr;

	if (!d)
		return j;

	for (item = obs_data_first(d); item; obs_data_item_next(&item)) {
		enum obs_data_type type = obs_data_item_gettype(item);
		const char *name = obs_data_item_get_name(item);

		if (!obs_data_item_has_user_value(item) && !includeDefault)
			continue;

		switch (type) {
		case OBS_DATA_STRING:
			set_json_string(&j, name, item);
			break;
		case OBS_DATA_NUMBER:
			set_json_number(&j, name, item);
			break;
		case OBS_DATA_BOOLEAN:
			set_json_bool(&j, name, item);
			break;
		case OBS_DATA_OBJECT:
			set_json_object(&j, name, item, includeDefault);
			break;
		case OBS_DATA_ARRAY:
			set_json_array(&j, name, item, includeDefault);
			break;
		default:;
		}
	}

	return j;
}

bool Utils::Json::GetJsonFileContent(std::string fileName, json &content)
{
	std::ifstream f(std::filesystem::u8path(fileName));
	if (!f.is_open())
		return false;

	try {
		content = json::parse(f);
	} catch (json::parse_error &e) {
		blog(LOG_WARNING, "[Utils::Json::GetJsonFileContent] Failed to decode content of JSON file `%s`. Error: %s",
		     fileName.c_str(), e.what());
		return false;
	}

	return true;
}

bool Utils::Json::SetJsonFileContent(std::string fileName, const json &content, bool makeDirs)
{
	auto jsonFilePath = std::filesystem::u8path(fileName);

	if (makeDirs) {
		auto p = jsonFilePath.parent_path();
		std::error_code ec;
		if (!ec && !std::filesystem::exists(p, ec))
			std::filesystem::create_directories(p, ec);
		if (ec) {
			blog(LOG_ERROR, "[Utils::Json::SetJsonFileContent] Failed to create path directories: %s",
			     ec.message().c_str());
			return false;
		}
	}

	std::ofstream f(jsonFilePath);
	if (!f.is_open()) {
		blog(LOG_ERROR, "[Utils::Json::SetJsonFileContent] Failed to open file `%s` for writing", fileName.c_str());
		return false;
	}

	// Set indent to 2 spaces, then dump content
	f << std::setw(2) << content;

	return true;
}
