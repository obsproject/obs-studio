#include <string.h>
#include <util/c99defs.h>

EXPORT const char *enum_outputs(size_t idx);

static const char *outputs[] = {"obs_ffmpeg"};

const char *enum_outputs(size_t idx)
{
	if (idx >= sizeof(outputs)/sizeof(const char*))
		return NULL;

	return outputs[idx];
}
