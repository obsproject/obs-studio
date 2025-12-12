#include <alsa/asoundlib.h>
#include <stdio.h>
#include <string.h>

// Mock Implementation

const char *snd_strerror(int errnum)
{
	return "Mock Error";
}

int snd_pcm_open(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode)
{
	*pcm = (snd_pcm_t *)0x1234;
	return 0;
}
int snd_pcm_close(snd_pcm_t *pcm)
{
	return 0;
}
int snd_pcm_drop(snd_pcm_t *pcm)
{
	return 0;
}
int snd_pcm_start(snd_pcm_t *pcm)
{
	return 0;
}
int snd_pcm_wait(snd_pcm_t *pcm, int timeout)
{
	return 0;
}
snd_pcm_state_t snd_pcm_state(snd_pcm_t *pcm)
{
	return SND_PCM_STATE_PREPARED;
}

snd_pcm_sframes_t snd_pcm_readi(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size)
{
	// Return explicit 0 or silence to avoid noise
	memset(buffer, 0, size * 4); // assume max 4 bytes/frame
	return size;
}
snd_pcm_sframes_t snd_pcm_recover(snd_pcm_t *pcm, int err, int silent)
{
	return 0;
}

// HW Params
size_t snd_pcm_hw_params_sizeof(void)
{
	return 1024;
}
int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	return 0;
}
int snd_pcm_hw_params_set_access(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access)
{
	return 0;
}
int snd_pcm_hw_params_set_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val)
{
	return 0;
}
int snd_pcm_hw_params_test_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val)
{
	return 0;
}

int snd_pcm_hw_params_set_rate_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir)
{
	if (*val == 0)
		*val = 44100;
	return 0;
}
int snd_pcm_hw_params_set_channels_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val)
{
	if (*val == 0)
		*val = 2;
	return 0;
}
int snd_pcm_hw_params_get_channels(const snd_pcm_hw_params_t *params, unsigned int *val)
{
	*val = 2;
	return 0;
}
int snd_pcm_hw_params_get_period_size(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir)
{
	*frames = 1024;
	return 0;
}
int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	return 0;
}
int snd_pcm_format_physical_width(snd_pcm_format_t format)
{
	return 16;
}

// Hints
int snd_device_name_hint(int card, const char *iface, void ***hints)
{
	*hints = malloc(sizeof(void *) * 2);
	(*hints)[0] = (void *)1; // Mock device
	(*hints)[1] = NULL;
	return 0;
}
char *snd_device_name_get_hint(const void *hint, const char *id)
{
	if (strcmp(id, "IOID") == 0)
		return strdup("Input");
	if (strcmp(id, "NAME") == 0)
		return strdup("default");
	if (strcmp(id, "DESC") == 0)
		return strdup("Mock ALSA Device");
	return NULL;
}
int snd_device_name_free_hint(void **hints)
{
	free(hints);
	return 0;
}
