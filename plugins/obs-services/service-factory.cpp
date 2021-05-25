#include "service-factory.hpp"

#include "plugin.hpp"
#include "service-instance.hpp"

extern "C" {
#include <obs-module.h>
}

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
