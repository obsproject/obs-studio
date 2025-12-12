#ifndef TO_BE_MOCKED_ALSA_PCM_H
#define TO_BE_MOCKED_ALSA_PCM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

    typedef struct _snd_pcm snd_pcm_t;
    struct _snd_pcm_hw_params {
        uint8_t dummy[512];  // Reserve space for mock
    };
    typedef struct _snd_pcm_hw_params snd_pcm_hw_params_t;
    typedef unsigned long snd_pcm_uframes_t;
    typedef long snd_pcm_sframes_t;

    typedef enum _snd_pcm_stream {
        SND_PCM_STREAM_PLAYBACK = 0,
        SND_PCM_STREAM_CAPTURE,
    } snd_pcm_stream_t;

    typedef enum _snd_pcm_format {
        SND_PCM_FORMAT_UNKNOWN = -1,
        SND_PCM_FORMAT_S8 = 0,
        SND_PCM_FORMAT_U8,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_FORMAT_S32_LE,
        SND_PCM_FORMAT_FLOAT_LE,
    } snd_pcm_format_t;

    typedef enum _snd_pcm_access {
        SND_PCM_ACCESS_MMAP_INTERLEAVED = 0,
        SND_PCM_ACCESS_RW_INTERLEAVED,
    } snd_pcm_access_t;

    typedef enum _snd_pcm_state {
        SND_PCM_STATE_OPEN = 0,
        SND_PCM_STATE_SETUP,
        SND_PCM_STATE_PREPARED,
        SND_PCM_STATE_RUNNING,
    } snd_pcm_state_t;

    const char *snd_strerror(int errnum);

    int snd_pcm_open(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode);
    int snd_pcm_close(snd_pcm_t *pcm);
    int snd_pcm_drop(snd_pcm_t *pcm);
    int snd_pcm_start(snd_pcm_t *pcm);
    int snd_pcm_wait(snd_pcm_t *pcm, int timeout);
    snd_pcm_state_t snd_pcm_state(snd_pcm_t *pcm);

    snd_pcm_sframes_t snd_pcm_readi(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size);
    snd_pcm_sframes_t snd_pcm_recover(snd_pcm_t *pcm, int err, int silent);

    // HW Params
    size_t snd_pcm_hw_params_sizeof(void);
    int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
    int snd_pcm_hw_params_set_access(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access);
    int snd_pcm_hw_params_set_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
    int snd_pcm_hw_params_test_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
    int snd_pcm_hw_params_set_rate_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
    int snd_pcm_hw_params_set_channels_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val);
    int snd_pcm_hw_params_get_channels(const snd_pcm_hw_params_t *params, unsigned int *val);
    int snd_pcm_hw_params_get_period_size(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir);
    int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
    int snd_pcm_format_physical_width(snd_pcm_format_t format);

    // Hints (Device List)
    int snd_device_name_hint(int card, const char *iface, void ***hints);
    char *snd_device_name_get_hint(const void *hint, const char *id);
    int snd_device_name_free_hint(void **hints);

// Macros/Alloca usually
#define snd_pcm_hw_params_alloca(ptr) do { *ptr = (snd_pcm_hw_params_t *)malloc(sizeof(snd_pcm_hw_params_t)); } while (0)
// wait, we need malloc
#include <stdlib.h>

#ifdef __cplusplus
}
#endif

#endif
