#pragma once

#include "service-factory.hpp"

extern "C" {
#include <obs-module.h>
#include <jansson.h>
}

class service_instance {
	service_factory *_factory;

	std::string protocol;
	std::string server;
	std::string key;

public:
	service_instance(obs_data_t *settings, obs_service_t *self);
	virtual ~service_instance(){};

	void update(obs_data_t *settings);

	const char *get_protocol();
	const char *get_url();
	const char *get_key();
};
