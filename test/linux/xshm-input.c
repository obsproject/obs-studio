#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include "xcursor.h"
#include "xshm-input.h"

struct xshm_data {
    Display *dpy;
    Screen *screen;
    Window root_window;
    Visual *visual;
    int depth;
    uint32_t width, height;
    int shm_attached;
    XShmSegmentInfo shm_info;
    XImage *image;
    texture_t texture;
    xcursor_t *cursor;
};

const char* xshm_input_getname(const char* locale)
{
    return "X11 Shared Memory Screen Input";
}

struct xshm_data *xshm_input_create(const char *settings, obs_source_t source)
{
    // create data structure
    struct xshm_data *data = bmalloc(sizeof(struct xshm_data));
    memset(data, 0, sizeof(struct xshm_data));
    
    // try to open display and all the good stuff
    data->dpy = XOpenDisplay(NULL);
    if (!data->dpy)
        goto fail;
    
    data->screen = XDefaultScreenOfDisplay(data->dpy);
    data->width = WidthOfScreen(data->screen);
    data->height = HeightOfScreen(data->screen);
    data->root_window = XRootWindowOfScreen(data->screen);
    data->visual = DefaultVisualOfScreen(data->screen);
    data->depth = DefaultDepthOfScreen(data->screen);
    
    // query for shm extension
    if (!XShmQueryExtension(data->dpy))
        goto fail;
    
    // create xshm image
    data->image = XShmCreateImage(data->dpy, data->visual, data->depth,
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

void xshm_input_destroy(struct xshm_data *data)
{
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

uint32_t xshm_input_get_output_flags(struct xshm_data *data)
{
    return SOURCE_VIDEO | SOURCE_DEFAULT_EFFECT;
}

void xshm_input_video_render(struct xshm_data *data, obs_source_t filter_target)
{
    // update texture
    XShmGetImage(data->dpy, data->root_window, data->image, 0, 0, AllPlanes);
    texture_setimage(data->texture, (void *) data->image->data,
                     data->width * 4, False);
    
    effect_t effect  = gs_geteffect();
    eparam_t diffuse = effect_getparambyname(effect, "diffuse");

    effect_settexture(effect, diffuse, data->texture);
    gs_draw_sprite(data->texture, 0, 0, 0);
    
    // render the cursor
    xcursor_render(data->cursor);
}

uint32_t xshm_input_getwidth(struct xshm_data *data)
{
    return texture_getwidth(data->texture);
}

uint32_t xshm_input_getheight(struct xshm_data *data)
{
    return texture_getheight(data->texture);
}
