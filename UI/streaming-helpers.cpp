#include "streaming-helpers.hpp"

#include "../plugins/rtmp-services/rtmp-format-ver.h"

#include <util/platform.h>
#include <util/util.hpp>
#include <obs.h>

using namespace json11;

static Json open_json_file(const char *path)
{
	BPtr<char> file_data = os_quick_read_utf8_file(path);
	if (!file_data)
		return Json();

	std::string err;
	Json json = Json::parse(file_data, err);
	if (json["format_version"].int_value() != RTMP_SERVICES_FORMAT_VERSION)
		return Json();
	return json;
}

static inline bool name_matches(const Json &service, const char *name)
{
	if (service["name"].string_value() == name)
		return true;

	auto &alt_names = service["alt_names"].array_items();
	for (const Json &alt_name : alt_names) {
		if (alt_name.string_value() == name) {
			return true;
		}
	}

	return false;
}

Json get_services_json()
{
	obs_module_t *mod = obs_get_module("rtmp-services");
	Json root;

	BPtr<char> file = obs_module_get_config_path(mod, "services.json");
	if (file)
		root = open_json_file(file);

	if (root.is_null()) {
		file = obs_find_module_file(mod, "services.json");
		if (file)
			root = open_json_file(file);
	}

	return root;
}

Json get_service_from_json(Json &root, const char *name)
{
	auto &services = root["services"].array_items();
	for (const Json &service : services) {
		if (name_matches(service, name)) {
			return service;
		}
	}

	return Json();
}
