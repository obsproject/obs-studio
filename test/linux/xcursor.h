#pragma once

#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Display *dpy;
    unsigned long last_serial;
    short unsigned int last_width;
    short unsigned int last_height;
    texture_t tex;
} xcursor_t;

/**
 * Initializes the xcursor object
 */
xcursor_t *xcursor_init(Display *dpy);

/**
 * Destroys the xcursor object
 */
void xcursor_destroy(xcursor_t *data);

/**
 * Draw the cursor
 * 
 * This needs to be executed within a valid render context
 */
void xcursor_render(xcursor_t *data);

#ifdef __cplusplus
}
#endif