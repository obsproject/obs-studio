#include <obs-internal.h>

void obs_enum_audio_monitoring_devices(obs_enum_audio_device_cb cb, void *data)
{
	UNUSED_PARAMETER(cb);
	UNUSED_PARAMETER(data);
}

struct audio_monitor *audio_monitor_create(obs_source_t *source)
{
	UNUSED_PARAMETER(source);
	return NULL;
}

void audio_monitor_reset(struct audio_monitor *monitor)
{
	UNUSED_PARAMETER(monitor);
}

void audio_monitor_destroy(struct audio_monitor *monitor)
{
	UNUSED_PARAMETER(monitor);
}
