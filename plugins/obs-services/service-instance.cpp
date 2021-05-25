#include "service-instance.hpp"

#include <util/util.hpp>

service_instance::service_instance(obs_data_t *settings, obs_service_t *self)
	: _factory(reinterpret_cast<service_factory *>(
		  obs_service_get_type_data(self)))
{
	update(settings);
}

void service_instance::update(obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
}
