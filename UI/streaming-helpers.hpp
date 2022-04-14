#pragma once

#include <json11.hpp>

extern json11::Json get_services_json();
extern json11::Json get_service_from_json(json11::Json &root, const char *name);
