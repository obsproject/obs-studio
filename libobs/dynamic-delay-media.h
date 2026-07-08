// dynamic-delay-media.h
#ifndef DYNAMIC_DELAY_MEDIA_H
#define DYNAMIC_DELAY_MEDIA_H

#include "obs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct dyn_delay_media;

typedef void (*dyn_delay_media_packet_cb)(void *param, struct encoder_packet *packet);

/* Creates the waiting-media pipeline: a private looping ffmpeg_source rendered
 * into an auxiliary view/video mix and encoded by a second video/audio encoder
 * pair mirroring the settings of the output's main encoders. Encoded packets
 * are delivered through the callback (invoked from encoder threads). */
struct dyn_delay_media *dyn_delay_media_create(const char *media_path, obs_encoder_t *main_venc,
					       obs_encoder_t *main_aenc, dyn_delay_media_packet_cb cb, void *param);

/* Stops the encoders; no more packets are delivered after this returns. Must
 * not be called while holding a mutex the packet callback also takes. */
void dyn_delay_media_stop(struct dyn_delay_media *media);

/* Stops and frees everything. Same locking restriction as stop. */
void dyn_delay_media_destroy(struct dyn_delay_media *media);

#ifdef __cplusplus
}
#endif

#endif // DYNAMIC_DELAY_MEDIA_H
