#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <obs.h>
#include "xcursor.h"

#define XSHM_DATA(voidptr) struct xshm_data *data = voidptr;

struct xshm_data {
    Display *dpy;
    Window root_window;
    uint32_t width, height;
    int shm_attached;
    XShmSegmentInfo shm_info;
    XImage *image;
    texture_t texture;
    xcursor_t *cursor;
};

static const char* xshm_input_getname(const char* locale)
{
    return "X11 Shared Memory Screen Input";
}

static void xshm_input_destroy(void *vptr)
{
    XSHM_DATA(vptr);
    
    if (data) {
        gs_entercontext(obs_graphics());

        texture_destroy(data->texture);
        xcursor_destroy(data->cursor);

        gs_leavecontext();
        
        // detach xshm
        if (data->shm_attached)
            XShmDetach(data->dpy, &data->shm_info);
        
        // detach shared memory
        if (data->shm_info.shmaddr != (char *) -1) {
            shmdt(data->shm_info.shmaddr);
            data->shm_info.shmaddr = (char *) -1;
        }
        
        // remove shared memory
        if (data->shm_info.shmid != -1)
            shmctl(data->shm_info.shmid, IPC_RMID, NULL);
        
        // destroy image
        if (data->image)
            XDestroyImage(data->image);
        
        // close display
        if (data->dpy)
            XCloseDisplay(data->dpy);
            
        bfree(data);
    }
}

static void *xshm_input_create(obs_data_t settings, obs_source_t source)
{
    // create data structure
    struct xshm_data *data = bmalloc(sizeof(struct xshm_data));
    memset(data, 0, sizeof(struct xshm_data));
    
    // try to open display and all the good stuff
    data->dpy = XOpenDisplay(NULL);
    if (!data->dpy)
        goto fail;
    
    Screen *screen = XDefaultScreenOfDisplay(data->dpy);
    data->width = WidthOfScreen(screen);
    data->height = HeightOfScreen(screen);
    data->root_window = XRootWindowOfScreen(screen);
    Visual *visual = DefaultVisualOfScreen(screen);
    int depth = DefaultDepthOfScreen(screen);
    
    // query for shm extension
    if (!XShmQueryExtension(data->dpy))
        goto fail;
    
    // create xshm image
    data->image = XShmCreateImage(data->dpy, visual, depth,
                                  ZPixmap, NULL, &data->shm_info,
                                  data->width, data->height);
    if (!data->image)
        goto fail;
    
    // create shared memory
    data->shm_info.shmid = shmget(IPC_PRIVATE, data->image->bytes_per_line *
                                  data->image->height, IPC_CREAT | 0700);
    if (data->shm_info.shmid < 0)
        goto fail;
    
    // attach shared memory
    data->shm_info.shmaddr = data->image->data 
                           = (char *) shmat(data->shm_info.shmid, 0, 0);
    if (data->shm_info.shmaddr == (char *) -1)
        goto fail;
    // set shared memory as read only
    data->shm_info.readOnly = False;
    
    // attach shm
    if (!XShmAttach(data->dpy, &data->shm_info))
        goto fail;
    data->shm_attached = 1;

    // get image
    if (!XShmGetImage(data->dpy, data->root_window, data->image,
                      0, 0, AllPlanes))
        goto fail;

    // create obs texture    
    gs_entercontext(obs_graphics());
    data->texture = gs_create_texture(data->width, data->height, GS_BGRA, 1,
                                      (const void**) &data->image->data,
                                      GS_DYNAMIC);
    data->cursor = xcursor_init(data->dpy);
    gs_leavecontext();
    
    if (!data->texture)
        goto fail;
    
    return data;
    
fail:
    // clean up and return null
    xshm_input_destroy(data);
    return NULL;
}

static void xshm_input_video_tick(void *vptr, float seconds)
{
    XSHM_DATA(vptr);
    
    gs_entercontext(obs_graphics());
    
    // update screen texture
    XShmGetImage(data->dpy, data->root_window, data->image, 0, 0, AllPlanes);
    texture_setimage(data->texture, (void *) data->image->data,
                     data->width * 4, False);
    
    // update mouse cursor
    xcursor_tick(data->cursor);
    
    gs_leavecontext();
}

static void xshm_input_video_render(void *vptr, effect_t effect)
{
    XSHM_DATA(vptr);
    
    eparam_t diffuse = effect_getparambyname(effect, "diffuse");
    effect_settexture(effect, diffuse, data->texture);
    
    gs_draw_sprite(data->texture, 0, 0, 0);
    
    // render the cursor
    xcursor_render(data->cursor);
}

static uint32_t xshm_input_getwidth(void *vptr)
{
    XSHM_DATA(vptr);
    
    return texture_getwidth(data->texture);
}

static uint32_t xshm_input_getheight(void *vptr)
{
    XSHM_DATA(vptr);
    
    return texture_getheight(data->texture);
}

struct obs_source_info xshm_input = {
    .id           = "xshm_input",
    .type         = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_VIDEO,
    .getname      = xshm_input_getname,
    .create       = xshm_input_create,
    .destroy      = xshm_input_destroy,
    .video_tick   = xshm_input_video_tick,
    .video_render = xshm_input_video_render,
    .getwidth     = xshm_input_getwidth,
    .getheight    = xshm_input_getheight
};
