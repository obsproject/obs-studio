#include <string.h>
#include <obs.h>

EXPORT bool enum_outputs(size_t idx, const char **name);
EXPORT uint32_t module_version(uint32_t in_version);

static const char *outputs[] = {"ffmpeg_output"};

uint32_t module_version(uint32_t in_version)
{
	return LIBOBS_API_VER;
}

bool enum_outputs(size_t idx, const char **name)
{
	if (idx >= sizeof(outputs)/sizeof(const char*))
		return false;

	*name = outputs[idx];
	return true;
}
