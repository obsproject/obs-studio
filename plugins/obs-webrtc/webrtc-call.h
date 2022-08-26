#pragma once

#include <libavcodec/avcodec.h>

extern struct gs_texture *webrtcTexture;

void setup_call();
void start_call();
static void *video_thread(void *data);