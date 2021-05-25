#include "service-factory.hpp"

#include "plugin.hpp"
#include "service-instance.hpp"

extern "C" {
#include <obs-module.h>
}

// XXX: Add the server list for each protocols
void service_factory::create_server_lists(obs_properties_t *props)
{
	obs_property_t *rtmp, *rtmps, *hls, *ftl;
	rtmp = obs_properties_add_list(props, "server_rtmp",
				       obs_module_text("Server"),
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_STRING);
	rtmps = obs_properties_add_list(props, "server_rtmps",
					obs_module_text("Server"),
					OBS_COMBO_TYPE_LIST,
					OBS_COMBO_FORMAT_STRING);
	hls = obs_properties_add_list(props, "server_hls",
				      obs_module_text("Server"),
				      OBS_COMBO_TYPE_LIST,
				      OBS_COMBO_FORMAT_STRING);
	ftl = obs_properties_add_list(props, "server_ftl",
				      obs_module_text("Server"),
				      OBS_COMBO_TYPE_LIST,
				      OBS_COMBO_FORMAT_STRING);

	for (size_t idx = 0; idx < servers.size(); idx++) {
		if (strcmp(servers[idx].protocol, "RTMP") == 0)
			obs_property_list_add_string(
				rtmp, obs_module_text(servers[idx].name),
				servers[idx].url);

		if (strcmp(servers[idx].protocol, "RTMPS") == 0)
			obs_property_list_add_string(
				rtmps, obs_module_text(servers[idx].name),
				servers[idx].url);

		if (strcmp(servers[idx].protocol, "HLS") == 0)
			obs_property_list_add_string(
				hls, obs_module_text(servers[idx].name),
				servers[idx].url);

		if (strcmp(servers[idx].protocol, "FTL") == 0)
			obs_property_list_add_string(
				ftl, obs_module_text(servers[idx].name),
				servers[idx].url);
	}
}

/* ----------------------------------------------------------------- */

static inline int get_int_val(json_t *service, const char *key)
{
	json_t *integer_val = json_object_get(service, key);
	if (!integer_val || !json_is_integer(integer_val))
		return -1;

	return (int)json_integer_value(integer_val);
}

static inline const char *get_string_val(json_t *service, const char *key)
{
	json_t *str_val = json_object_get(service, key);
	if (!str_val || !json_is_string(str_val))
		return NULL;

	return json_string_value(str_val);
}

const char *service_factory::_get_name(void *type_data) noexcept
try {
	if (type_data)
		return reinterpret_cast<service_factory *>(type_data)
			->get_name();
	return nullptr;
} catch (const std::exception &ex) {
	blog(LOG_ERROR, "Unexpected exception in function %s: %s", __func__,
	     ex.what());
	return nullptr;
} catch (...) {
	blog(LOG_ERROR, "Unexpected exception in function %s", __func__);
	return nullptr;
}

void *service_factory::_create(obs_data_t *settings,
			       obs_service_t *service) noexcept
try {
	service_factory *fac = reinterpret_cast<service_factory *>(
		obs_service_get_type_data(service));
	return fac->create(settings, service);
} catch (const std::exception &ex) {
	blog(LOG_ERROR, "Unexpected exception in function %s: %s", __func__,
	     ex.what());
	return nullptr;
} catch (...) {
	blog(LOG_ERROR, "Unexpected exception in function %s", __func__);
	return nullptr;
}

void service_factory::_destroy(void *data) noexcept
try {
	if (data)
		delete reinterpret_cast<service_instance *>(data);
} catch (const std::exception &ex) {
	blog(LOG_ERROR, "Unexpected exception in function %s: %s", __func__,
	     ex.what());
} catch (...) {
	blog(LOG_ERROR, "Unexpected exception in function %s", __func__);
}

void service_factory::_update(void *data, obs_data_t *settings) noexcept
try {
	service_instance *priv = reinterpret_cast<service_instance *>(data);
	if (priv) {
		priv->update(settings);
	}

} catch (const std::exception &ex) {
	blog(LOG_ERROR, "Unexpected exception in function %s: %s", __func__,
	     ex.what());
} catch (...) {
	blog(LOG_ERROR, "Unexpected exception in function %s", __func__);
}

void service_factory::_get_defaults2(obs_data_t *settings,
				     void *type_data) noexcept
try {
	if (type_data)
		reinterpret_cast<service_factory *>(type_data)->get_defaults2(
			settings);
} catch (const std::exception &ex) {
	blog(LOG_ERROR, "Unexpected exception in function %s: %s", __func__,
	     ex.what());
} catch (...) {
	blog(LOG_ERROR, "Unexpected exception in function %s", __func__);
}

obs_properties_t *service_factory::_get_properties2(void *data,
						    void *type_data) noexcept
try {
	if (type_data)
		return reinterpret_cast<service_factory *>(type_data)
			->get_properties2(data);
	return nullptr;
} catch (const std::exception &ex) {
	blog(LOG_ERROR, "Unexpected exception in function %s: %s", __func__,
	     ex.what());
	return nullptr;
} catch (...) {
	blog(LOG_ERROR, "Unexpected exception in function %s", __func__);
	return nullptr;
}

const char *service_factory::_get_protocol(void *data) noexcept
try {
	service_instance *priv = reinterpret_cast<service_instance *>(data);
	if (priv)
		return priv->get_protocol();
	return nullptr;
} catch (const std::exception &ex) {
	blog(LOG_ERROR, "Unexpected exception in function %s: %s", __func__,
	     ex.what());
	return nullptr;
} catch (...) {
	blog(LOG_ERROR, "Unexpected exception in function %s", __func__);
	return nullptr;
}

const char *service_factory::_get_url(void *data) noexcept
try {
	service_instance *priv = reinterpret_cast<service_instance *>(data);
	if (priv)
		return priv->get_url();
	return nullptr;
} catch (const std::exception &ex) {
	blog(LOG_ERROR, "Unexpected exception in function %s: %s", __func__,
	     ex.what());
	return nullptr;
} catch (...) {
	blog(LOG_ERROR, "Unexpected exception in function %s", __func__);
	return nullptr;
}

const char *service_factory::_get_key(void *data) noexcept
try {
	service_instance *priv = reinterpret_cast<service_instance *>(data);
	if (priv)
		return priv->get_key();
	return nullptr;
} catch (const std::exception &ex) {
	blog(LOG_ERROR, "Unexpected exception in function %s: %s", __func__,
	     ex.what());
	return nullptr;
} catch (...) {
	blog(LOG_ERROR, "Unexpected exception in function %s", __func__);
	return nullptr;
}

/* ----------------------------------------------------------------- */

service_factory::service_factory(json_t *service)
{
	/** JSON data extraction **/

	_id = get_string_val(service, "id");
	_name = get_string_val(service, "name");

	json_t *object;
	size_t idx;
	json_t *element;

	/* Available protocols extraction */

	object = json_object_get(service, "available_protocols");
	//If not provided set only RTMP by default
	if (!object)
		protocols.push_back("RTMP");
	else {
		json_incref(object);

		json_array_foreach (object, idx, element) {
			const char *prtcl = json_string_value(element);
			if (prtcl != NULL)
				protocols.push_back(prtcl);
		}

		json_decref(object);
	}

	/* Servers extraction */

	object = json_object_get(service, "servers");
	if (object) {
		json_incref(object);

		json_array_foreach (object, idx, element) {
			const char *prtcl = get_string_val(element, "protocol");
			const char *url = get_string_val(element, "url");
			const char *name = get_string_val(element, "name");

			if ((url != NULL) && (name != NULL)) {
				//If not provided set RTMP by default
				if (prtcl != NULL)
					servers.push_back({strdup(prtcl),
							   strdup(url),
							   strdup(name)});
				else
					servers.push_back({"RTMP", strdup(url),
							   strdup(name)});
			}
		}

		json_decref(object);
	}

	/** Service implementation **/

	_info.type_data = this;
	_info.id = _id.c_str();

	_info.get_name = _get_name;

	_info.create = _create;
	_info.destroy = _destroy;

	_info.update = _update;

	_info.get_defaults2 = _get_defaults2;

	_info.get_properties2 = _get_properties2;

	_info.get_protocol = _get_protocol;
	_info.get_url = _get_url;
	_info.get_key = _get_key;

	obs_register_service(&_info);
}

service_factory::~service_factory()
{
	protocols.clear();
	servers.clear();
}

const char *service_factory::get_name()
{
	return _name.c_str();
}

void *service_factory::create(obs_data_t *settings, obs_service_t *service)
{
	return reinterpret_cast<void *>(
		new service_instance(settings, service));
}

void service_factory::get_defaults2(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "protocol", protocols[0].c_str());
}

// XXX: Set the visibility of server lists for each protocols
static bool modified_protocol(obs_properties_t *props, obs_property_t *,
			      obs_data_t *settings) noexcept
try {
	const char *protocol = obs_data_get_string(settings, "protocol");
	bool rtmp = strcmp(protocol, "RTMP") == 0;
	bool rtmps = strcmp(protocol, "RTMPS") == 0;
	bool hls = strcmp(protocol, "HLS") == 0;
	bool ftl = strcmp(protocol, "FTL") == 0;

	//Server lists
	obs_property_set_visible(obs_properties_get(props, "server_rtmp"),
				 rtmp);
	obs_property_set_visible(obs_properties_get(props, "server_rtmps"),
				 rtmps);
	obs_property_set_visible(obs_properties_get(props, "server_hls"), hls);
	obs_property_set_visible(obs_properties_get(props, "server_ftl"), ftl);

	return true;
} catch (const std::exception &ex) {
	blog(LOG_ERROR, "Unexpected exception in function %s: %s", __func__,
	     ex.what());
	return false;
} catch (...) {
	blog(LOG_ERROR, "Unexpected exception in function %s", __func__);
	return false;
}

obs_properties_t *service_factory::get_properties2(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	p = obs_properties_add_list(props, "protocol",
				    obs_module_text("Protocol"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(p, modified_protocol);

	for (size_t idx = 0; idx < protocols.size(); idx++)
		obs_property_list_add_string(p, protocols[idx].c_str(),
					     protocols[idx].c_str());

	create_server_lists(props);

	obs_properties_add_text(props, "key", obs_module_text("StreamKey"),
				OBS_TEXT_PASSWORD);

	return props;
}
