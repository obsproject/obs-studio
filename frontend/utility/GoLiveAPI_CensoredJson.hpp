#pragma once

#include <obs.hpp>

#include <nlohmann/json_fwd.hpp>

#include <QString>

/**
 * Returns the input serialized to JSON, but any non-empty "authorization"
 * properties have their values replaced by "CENSORED".
 */
QString censoredJson(obs_data_t *data, bool pretty = false);
QString censoredJson(nlohmann::json data, bool pretty = false);
