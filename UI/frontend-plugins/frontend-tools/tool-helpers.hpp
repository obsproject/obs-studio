#pragma once

#include <obs.hpp>
#include <string>
#include <QString>

static inline obs_weak_source_t *GetWeakSourceByName(const char *name)
{
	OBSWeakSourceAutoRelease weak;
	OBSSourceAutoRelease source = obs_get_source_by_name(name);
	if (source)
		weak = obs_source_get_weak_source(source);

	return weak;
}

static inline obs_weak_source_t *GetWeakSourceByQString(const QString &name)
{
	return GetWeakSourceByName(name.toUtf8().constData());
}

static inline std::string GetWeakSourceName(obs_weak_source_t *weak_source)
{
	std::string name;

	OBSSourceAutoRelease source = obs_weak_source_get_source(weak_source);
	if (source)
		name = obs_source_get_name(source);

	return name;
}
