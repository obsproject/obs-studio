#include <string.h>
#include "obs-outputs.h"

static const char *outputs[] = {"rtmp_stream"};

const char *enum_outputs(size_t idx)
{
	if (idx >= sizeof(outputs)/sizeof(const char*))
		return NULL;

	return outputs[idx];
}
