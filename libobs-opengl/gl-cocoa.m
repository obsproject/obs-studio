/******************************************************************************
    Copyright (C) 2013 by Ruwen Hahn <palana@stunned.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "gl-subsystem.h"
#include <OpenGL/OpenGL.h>

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

struct gl_windowinfo {
    NSView *view;
    NSOpenGLContext *context;
    gs_texture_t *texture;
    GLuint fbo;
};

struct gl_platform {
    NSOpenGLContext *context;
};

typedef struct {
    gs_swapchain_t *swapchain;
    NSOpenGLContext *platformContext;
    NSOpenGLContext *swapchainContext;
    GLuint frameBufferObjectId;
    enum gs_color_format textureFormat;
    NSSize textureSize;
} OBSFrameBufferUpdateData;

static NSOpenGLContext *gl_context_create(NSOpenGLContext *share)
{
    NSOpenGLPixelFormatAttribute attributes[] = {NSOpenGLPFADoubleBuffer, NSOpenGLPFAOpenGLProfile,
                                                 NSOpenGLProfileVersion3_2Core, 0};

    NSOpenGLPixelFormat *pf;
    pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
    if (!pf) {
        blog(LOG_ERROR, "Failed to create pixel format");
        return NULL;
    }

    NSOpenGLContext *context;
    context = [[NSOpenGLContext alloc] initWithFormat:pf shareContext:share];
    [pf release];
    if (!context) {
        blog(LOG_ERROR, "Failed to create context");
        return NULL;
    }

    [context clearDrawable];

    return context;
}

struct gl_platform *gl_platform_create(gs_device_t *device __unused, uint32_t adapter __unused)
{
    NSOpenGLContext *context = gl_context_create(nil);
    if (!context) {
        blog(LOG_ERROR, "gl_context_create failed");
        return NULL;
    }

    [context makeCurrentContext];
    GLint interval = 0;
    [context setValues:&interval forParameter:NSOpenGLContextParameterSwapInterval];
    const bool success = gladLoadGL() != 0;

    if (!success) {
        blog(LOG_ERROR, "gladLoadGL failed");
        [context release];
        return NULL;
    }

    struct gl_platform *plat = bzalloc(sizeof(struct gl_platform));
    plat->context = context;
    return plat;
}

void gl_platform_destroy(struct gl_platform *platform)
{
    if (!platform)
        return;

    [platform->context release];
    platform->context = nil;

    bfree(platform);
}

bool gl_platform_init_swapchain(struct gs_swap_chain *swap)
{
    NSOpenGLContext *parent = swap->device->plat->context;
    NSOpenGLContext *context = gl_context_create(parent);
    bool success = context != nil;
    if (success) {
        CGLContextObj parent_obj = [parent CGLContextObj];
        CGLLockContext(parent_obj);

        [parent makeCurrentContext];
        struct gs_init_data *init_data = &swap->info;
        swap->wi->texture = device_texture_create(swap->device, init_data->cx, init_data->cy, init_data->format, 1,
                                                  NULL, GS_RENDER_TARGET);
        glFlush();
        [NSOpenGLContext clearCurrentContext];

        CGLContextObj context_obj = [context CGLContextObj];
        CGLLockContext(context_obj);

        [context makeCurrentContext];

#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        [context setView:swap->wi->view];
#pragma clang diagnostic pop
        GLint interval = 0;
        [context setValues:&interval forParameter:NSOpenGLContextParameterSwapInterval];
        gl_gen_framebuffers(1, &swap->wi->fbo);
        gl_bind_framebuffer(GL_FRAMEBUFFER, swap->wi->fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, swap->wi->texture->texture, 0);
        gl_success("glFrameBufferTexture2D");
        glFlush();
        [NSOpenGLContext clearCurrentContext];

        CGLUnlockContext(context_obj);

        CGLUnlockContext(parent_obj);

        swap->wi->context = context;
    }

    return success;
}

void gl_platform_cleanup_swapchain(struct gs_swap_chain *swap)
{
    NSOpenGLContext *parent = swap->device->plat->context;
    CGLContextObj parent_obj = [parent CGLContextObj];
    CGLLockContext(parent_obj);

    NSOpenGLContext *context = swap->wi->context;
    CGLContextObj context_obj = [context CGLContextObj];
    CGLLockContext(context_obj);

    [context makeCurrentContext];
    gl_delete_framebuffers(1, &swap->wi->fbo);
    glFlush();
    [NSOpenGLContext clearCurrentContext];

    CGLUnlockContext(context_obj);

    [parent makeCurrentContext];
    gs_texture_destroy(swap->wi->texture);
    glFlush();
    [NSOpenGLContext clearCurrentContext];
    swap->wi->context = nil;

    CGLUnlockContext(parent_obj);
}

struct gl_windowinfo *gl_windowinfo_create(const struct gs_init_data *info)
{
    if (!info)
        return NULL;

    if (!info->window.view)
        return NULL;

    struct gl_windowinfo *wi = bzalloc(sizeof(struct gl_windowinfo));

    wi->view = info->window.view;
    wi->view.window.colorSpace = NSColorSpace.sRGBColorSpace;
#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    wi->view.wantsBestResolutionOpenGLSurface = YES;
#pragma clang diagnostic pop

    return wi;
}

void gl_windowinfo_destroy(struct gl_windowinfo *wi)
{
    if (!wi)
        return;

    wi->view = nil;
    bfree(wi);
void updateSwapchainFramebuffer(gs_device_t *device, OBSFrameBufferUpdateData updateData)
{
    if (!(device && updateData.swapchain)) {
        return;
    }

    NSOpenGLContext *platformContext = updateData.platformContext;
    NSOpenGLContext *swapContext = updateData.swapchainContext;

    CGLContextObj platformContextObj = platformContext.CGLContextObj;
    CGLLockContext(platformContextObj);

    CGLContextObj swapContextObj = swapContext.CGLContextObj;
    CGLLockContext(swapContextObj);

    [swapContext makeCurrentContext];
    [swapContext update];

    gs_texture_t *framebufferTexture = device_texture_create(device, updateData.textureSize.width,
                                                             updateData.textureSize.height, updateData.textureFormat, 1,
                                                             NULL, GS_RENDER_TARGET);

    if (!framebufferTexture) {
        blog(LOG_ERROR, "gl_update (deferred): Unable to generate backing texture for frame buffer.");

        goto updateSwapchainCleanup;
    }

    BOOL hasBoundFramebuffer = gl_bind_framebuffer(GL_FRAMEBUFFER, updateData.frameBufferObjectId);

    if (!hasBoundFramebuffer) {
        goto updateSwapchainCleanup;
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture->texture, 0);
    BOOL hasBoundFramebufferTexture = gl_success("glFrameBufferTexture2D");

    if (!hasBoundFramebufferTexture) {
        GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
            switch (framebufferStatus) {
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    blog(LOG_ERROR, "gl_update (deferred): Framebuffer object attachment is not complete.");
                    goto updateSwapchainCleanup;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    blog(LOG_ERROR, "gl_update (deferred): Framebuffer object attachment is missing.");
                    goto updateSwapchainCleanup;
                default:
                    blog(LOG_ERROR, "gl_update (deferred): Framebuffer object is incomplete.");
                    goto updateSwapchainCleanup;
            }
        }

        goto updateSwapchainCleanup;
    }

    gl_bind_framebuffer(GL_FRAMEBUFFER, 0);

    glFlush();

    gs_swapchain_t *currentSwapchain = updateData.swapchain;

    if (currentSwapchain && currentSwapchain->wi && currentSwapchain->wi->texture) {
        gs_texture_t *priorFramebufferTexture = currentSwapchain->wi->texture;
        currentSwapchain->wi->texture = framebufferTexture;
        gs_texture_destroy(priorFramebufferTexture);
    }

updateSwapchainCleanup:
    [NSOpenGLContext clearCurrentContext];

    CGLUnlockContext(swapContextObj);
    CGLUnlockContext(platformContextObj);
}

void gl_update(gs_device_t *device)
{
    if (!(device && device->cur_swap)) {
        return;
    }

    gs_swapchain_t *currentSwapchain = device->cur_swap;

    NSOpenGLContext *platformContext = device->plat->context;
    NSOpenGLContext *swapContext = currentSwapchain->wi->context;

    OBSFrameBufferUpdateData updateData = {.swapchain = currentSwapchain,
                                           .platformContext = platformContext,
                                           .swapchainContext = swapContext,
                                           .frameBufferObjectId = currentSwapchain->wi->fbo,
                                           .textureFormat = currentSwapchain->info.format,
                                           .textureSize =
                                               NSMakeSize(currentSwapchain->info.cx, currentSwapchain->info.cy)};

    if (platformContext && swapContext) {
        [platformContext retain];
        [swapContext retain];

        dispatch_async(dispatch_get_main_queue(), ^() {
            updateSwapchainFramebuffer(device, updateData);

            [swapContext release];
            [platformContext release];
        });
    }
}

void gl_clear_context(gs_device_t *device)
{
    UNUSED_PARAMETER(device);
    [NSOpenGLContext clearCurrentContext];
}

void device_enter_context(gs_device_t *device)
{
    CGLLockContext([device->plat->context CGLContextObj]);

    [device->plat->context makeCurrentContext];
}

void device_leave_context(gs_device_t *device)
{
    glFlush();
    [NSOpenGLContext clearCurrentContext];
    device->cur_vertex_buffer = NULL;
    device->cur_index_buffer = NULL;
    device->cur_render_target = NULL;
    device->cur_zstencil_buffer = NULL;
    device->cur_swap = NULL;
    device->cur_fbo = NULL;

    CGLUnlockContext([device->plat->context CGLContextObj]);
}

void *device_get_device_obj(gs_device_t *device)
{
    return device->plat->context;
}

void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swap)
{
    if (device->cur_swap == swap)
        return;

    device->cur_swap = swap;
    if (swap) {
        device_set_render_target(device, swap->wi->texture, NULL);
    }
}

bool device_is_present_ready(gs_device_t *device __unused)
{
    return true;
}

void device_present(gs_device_t *device)
{
    glFlush();
    [NSOpenGLContext clearCurrentContext];

    CGLLockContext([device->cur_swap->wi->context CGLContextObj]);

    [device->cur_swap->wi->context makeCurrentContext];
    gl_bind_framebuffer(GL_READ_FRAMEBUFFER, device->cur_swap->wi->fbo);
    gl_bind_framebuffer(GL_DRAW_FRAMEBUFFER, 0);
    const uint32_t width = device->cur_swap->info.cx;
    const uint32_t height = device->cur_swap->info.cy;
    glBlitFramebuffer(0, 0, width, height, 0, height, width, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    [device->cur_swap->wi->context flushBuffer];
    glFlush();
    [NSOpenGLContext clearCurrentContext];

    CGLUnlockContext([device->cur_swap->wi->context CGLContextObj]);

    [device->plat->context makeCurrentContext];
}

bool device_is_monitor_hdr(gs_device_t *device __unused, void *monitor __unused)
{
    return false;
}

void gl_getclientsize(const struct gs_swap_chain *swap, uint32_t *width, uint32_t *height)
{
    if (width)
        *width = swap->info.cx;
    if (height)
        *height = swap->info.cy;
}

gs_texture_t *device_texture_create_from_iosurface(gs_device_t *device, void *iosurf)
{
    IOSurfaceRef ref = (IOSurfaceRef) iosurf;
    struct gs_texture_2d *tex = bzalloc(sizeof(struct gs_texture_2d));

    OSType pf = IOSurfaceGetPixelFormat(ref);

    FourCharCode l10r_code = 0;
    l10r_code = ('l' << 24) | ('1' << 16) | ('0' << 8) | 'r';

    FourCharCode bgra_code = 0;
    bgra_code = ('B' << 24) | ('G' << 16) | ('R' << 8) | 'A';

    const bool l10r = pf == l10r_code;
    if (pf == 0)
        blog(LOG_ERROR, "Invalid IOSurface Buffer");
    else if ((pf != bgra_code) && !l10r)
        blog(LOG_ERROR, "Unexpected pixel format: %d (%c%c%c%c)", pf, pf >> 24, pf >> 16, pf >> 8, pf);

    const enum gs_color_format color_format = l10r ? GS_R10G10B10A2 : GS_BGRA;

    tex->base.device = device;
    tex->base.type = GS_TEXTURE_2D;
    tex->base.format = color_format;
    tex->base.levels = 1;
    tex->base.gl_format = l10r ? GL_BGRA : convert_gs_format(color_format);
    tex->base.gl_internal_format = convert_gs_internal_format(color_format);
    tex->base.gl_type = l10r ? GL_UNSIGNED_INT_2_10_10_10_REV : GL_UNSIGNED_INT_8_8_8_8_REV;
    tex->base.gl_target = GL_TEXTURE_RECTANGLE_ARB;
    tex->base.is_dynamic = false;
    tex->base.is_render_target = false;
    tex->base.gen_mipmaps = false;
    tex->width = (uint32_t) IOSurfaceGetWidth(ref);
    tex->height = (uint32_t) IOSurfaceGetHeight(ref);

    if (!gl_gen_textures(1, &tex->base.texture))
        goto fail;

    if (!gl_bind_texture(tex->base.gl_target, tex->base.texture))
        goto fail;

    CGLError err = CGLTexImageIOSurface2D([[NSOpenGLContext currentContext] CGLContextObj], tex->base.gl_target,
                                          tex->base.gl_internal_format, tex->width, tex->height, tex->base.gl_format,
                                          tex->base.gl_type, ref, 0);

    if (err != kCGLNoError) {
        blog(LOG_ERROR,
             "CGLTexImageIOSurface2D: %u, %s"
             " (device_texture_create_from_iosurface)",
             err, CGLErrorString(err));

        gl_success("CGLTexImageIOSurface2D");
        goto fail;
    }

    if (!gl_tex_param_i(tex->base.gl_target, GL_TEXTURE_MAX_LEVEL, 0))
        goto fail;

    if (!gl_bind_texture(tex->base.gl_target, 0))
        goto fail;

    return (gs_texture_t *) tex;

fail:
    gs_texture_destroy((gs_texture_t *) tex);
    blog(LOG_ERROR, "device_texture_create_from_iosurface (GL) failed");
    return NULL;
}

gs_texture_t *device_texture_open_shared(gs_device_t *device, uint32_t handle)
{
    gs_texture_t *texture = NULL;
    IOSurfaceRef ref = IOSurfaceLookupFromMachPort((mach_port_t) handle);
    texture = device_texture_create_from_iosurface(device, ref);
    CFRelease(ref);
    return texture;
}

bool device_shared_texture_available(void)
{
    return true;
}

bool gs_texture_rebind_iosurface(gs_texture_t *texture, void *iosurf)
{
    if (!texture)
        return false;

    if (!iosurf)
        return false;

    FourCharCode l10r_code = 0;
    l10r_code = ('l' << 24) | ('1' << 16) | ('0' << 8) | 'r';

    FourCharCode bgra_code = 0;
    bgra_code = ('B' << 24) | ('G' << 16) | ('R' << 8) | 'A';

    struct gs_texture_2d *tex = (struct gs_texture_2d *) texture;
    IOSurfaceRef ref = (IOSurfaceRef) iosurf;

    OSType pf = IOSurfaceGetPixelFormat(ref);
    if (pf == 0) {
        blog(LOG_ERROR, "Invalid IOSurface buffer");
    } else if ((pf != bgra_code) && (pf != l10r_code)) {
        blog(LOG_ERROR, "Unexpected pixel format: %d (%c%c%c%c)", pf, pf >> 24, pf >> 16, pf >> 8, pf);
    }

    tex->width = (uint32_t) IOSurfaceGetWidth(ref);
    tex->height = (uint32_t) IOSurfaceGetHeight(ref);

    if (!gl_bind_texture(tex->base.gl_target, tex->base.texture))
        return false;

    CGLError err = CGLTexImageIOSurface2D([[NSOpenGLContext currentContext] CGLContextObj], tex->base.gl_target,
                                          tex->base.gl_internal_format, tex->width, tex->height, tex->base.gl_format,
                                          tex->base.gl_type, ref, 0);

    if (err != kCGLNoError) {
        blog(LOG_ERROR,
             "CGLTexImageIOSurface2D: %u, %s"
             " (gs_texture_rebind_iosurface)",
             err, CGLErrorString(err));

        gl_success("CGLTexImageIOSurface2D");
        return false;
    }

    if (!gl_bind_texture(tex->base.gl_target, 0))
        return false;

    return true;
}
