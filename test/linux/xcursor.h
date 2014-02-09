#pragma once

#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Display *dpy;
    unsigned long last_serial;
    texture_t tex;
} xcursor_t;

// initialize xcursor data
xcursor_t *xcursor_init(Display *dpy);

// destroy xcursor data
void xcursor_destroy(xcursor_t *data);

// draw the cursor
void xcursor_render(xcursor_t *data);

#ifdef __cplusplus
}
#endif