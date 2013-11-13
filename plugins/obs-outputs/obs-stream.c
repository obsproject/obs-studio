#include "obs-stream.h"

void *rtmp_stream_getname(const char *locale)
{
	/* TODO: locale stuff */
	return "RTMP Stream";
}

void *rtmp_stream_create(const char *settings, obs_output_t output)
{
	
}

void *rtmp_stream_destroy(void *data)
{
}

void *rtmp_stream_update(void *data, const char *settings)
{
}

void rtmp_stream_start(void *data)
{
}

void rtmp_stream_stop(void *data)
{
}
