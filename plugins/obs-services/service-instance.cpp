#include "service-instance.hpp"

#include <util/util.hpp>

service_instance::service_instance(obs_data_t *settings, obs_service_t *self)
	: _factory(reinterpret_cast<service_factory *>(
		  obs_service_get_type_data(self)))
{
	update(settings);
}

// XXX: Add server updater for each protocols
void service_instance::update(obs_data_t *settings)
{
	protocol = obs_data_get_string(settings, "protocol");

	if (protocol.compare("RTMP") == 0)
		server = obs_data_get_string(settings, "server_rtmp");
	if (protocol.compare("RTMPS") == 0)
		server = obs_data_get_string(settings, "server_rtmps");
	if (protocol.compare("HLS") == 0)
		server = obs_data_get_string(settings, "server_hls");
	if (protocol.compare("FTL") == 0)
		server = obs_data_get_string(settings, "server_ftl");

	key = obs_data_get_string(settings, "key");
}

const char *service_instance::get_protocol()
{
	return protocol.c_str();
}

const char *service_instance::get_url()
{
	return server.c_str();
}

const char *service_instance::get_key()
{
	return key.c_str();
}
