#include "GoLiveAPI_CensoredJson.hpp"

#include <nlohmann/json.hpp>

#include <unordered_map>

void censorRecurse(obs_data_t *);
void censorRecurseArray(obs_data_array_t *);

void censorRecurse(obs_data_t *data)
{
	// if we found what we came to censor, censor it
	const char *a = obs_data_get_string(data, "authentication");
	if (a && *a) {
		obs_data_set_string(data, "authentication", "CENSORED");
	}

	// recurse to child objects and arrays
	obs_data_item_t *item = obs_data_first(data);
	for (; item != NULL; obs_data_item_next(&item)) {
		enum obs_data_type typ = obs_data_item_gettype(item);

		if (typ == OBS_DATA_OBJECT) {
			obs_data_t *child_data = obs_data_item_get_obj(item);
			censorRecurse(child_data);
			obs_data_release(child_data);
		} else if (typ == OBS_DATA_ARRAY) {
			OBSDataArrayAutoRelease child_array = obs_data_item_get_array(item);
			censorRecurseArray(child_array);
		}
	}
}

void censorRecurseArray(obs_data_array_t *array)
{
	const size_t sz = obs_data_array_count(array);
	for (size_t i = 0; i < sz; i++) {
		obs_data_t *item = obs_data_array_item(array, i);
		censorRecurse(item);
		obs_data_release(item);
	}
}

QString censoredJson(obs_data_t *data, bool pretty)
{
	if (!data) {
		return "";
	}

	// Ugly clone via JSON write/read
	const char *j = obs_data_get_json(data);
	obs_data_t *clone = obs_data_create_from_json(j);

	// Censor our copy
	censorRecurse(clone);

	// Turn our copy into JSON
	QString s = pretty ? obs_data_get_json_pretty(clone) : obs_data_get_json(clone);

	// Eliminate our copy
	obs_data_release(clone);

	return s;
}

using json = nlohmann::json;

void censorRecurse(json &data)
{
	if (!data.is_structured())
		return;

	auto it = data.find("authentication");
	if (it != data.end() && it->is_string()) {
		*it = "CENSORED";
	}

	for (auto &child : data) {
		censorRecurse(child);
	}
}

QString censoredJson(json data, bool pretty)
{
	censorRecurse(data);

	return QString::fromStdString(data.dump(pretty ? 4 : -1));
}
