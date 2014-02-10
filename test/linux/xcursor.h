#pragma once

#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Display *dpy;
    float pos_x;
    float pos_y;
    unsigned long last_serial;
    unsigned short int last_width;
    unsigned short int last_height;
    texture_t tex;
} xcursor_t;

/**
 * Initializes the xcursor object
 * 
 * This needs to be executed within a valid render context
 */
xcursor_t *xcursor_init(Display *dpy);

/**
 * Destroys the xcursor object
 */
void xcursor_destroy(xcursor_t *data);

/**
 * Update the cursor texture
 * 
 * This needs to be executed within a valid render context
 */
void xcursor_tick(xcursor_t *data);

/**
 * Draw the cursor
 * 
 * This needs to be executed within a valid render context
 */
void xcursor_render(xcursor_t *data);

#ifdef __cplusplus
}
#endif