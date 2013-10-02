#ifndef __glxew_h__
#define __glxew_h__
#define __GLXEW_H__

#ifdef __glxext_h_
#error glxext.h included before glxew.h
#endif

#if defined(GLX_H) || defined(__GLX_glx_h__) || defined(__glx_h__)
#error glx.h included before glxew.h
#endif

#define __glxext_h_

#define GLX_H
#define __GLX_glx_h__
#define __glx_h__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#include <GL/glew.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------- GLX_VERSION_1_0 --------------------------- */

#ifndef GLX_VERSION_1_0
#define GLX_VERSION_1_0 1

#define GLX_USE_GL 1
#define GLX_BUFFER_SIZE 2
#define GLX_LEVEL 3
#define GLX_RGBA 4
#define GLX_DOUBLEBUFFER 5
#define GLX_STEREO 6
#define GLX_AUX_BUFFERS 7
#define GLX_RED_SIZE 8
#define GLX_GREEN_SIZE 9
#define GLX_BLUE_SIZE 10
#define GLX_ALPHA_SIZE 11
#define GLX_DEPTH_SIZE 12
#define GLX_STENCIL_SIZE 13
#define GLX_ACCUM_RED_SIZE 14
#define GLX_ACCUM_GREEN_SIZE 15
#define GLX_ACCUM_BLUE_SIZE 16
#define GLX_ACCUM_ALPHA_SIZE 17
#define GLX_BAD_SCREEN 1
#define GLX_BAD_ATTRIBUTE 2
#define GLX_NO_EXTENSION 3
#define GLX_BAD_VISUAL 4
#define GLX_BAD_CONTEXT 5
#define GLX_BAD_VALUE 6
#define GLX_BAD_ENUM 7

typedef XID GLXDrawable;
typedef XID GLXPixmap;
#ifdef __sun
typedef struct __glXContextRec *GLXContext;
#else
typedef struct __GLXcontextRec *GLXContext;
#endif

typedef unsigned int GLXVideoDeviceNV; 

extern Bool glXQueryExtension (Display *dpy, int *errorBase, int *eventBase);
extern Bool glXQueryVersion (Display *dpy, int *major, int *minor);
extern int glXGetConfig (Display *dpy, XVisualInfo *vis, int attrib, int *value);
extern XVisualInfo* glXChooseVisual (Display *dpy, int screen, int *attribList);
extern GLXPixmap glXCreateGLXPixmap (Display *dpy, XVisualInfo *vis, Pixmap pixmap);
extern void glXDestroyGLXPixmap (Display *dpy, GLXPixmap pix);
extern GLXContext glXCreateContext (Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
extern void glXDestroyContext (Display *dpy, GLXContext ctx);
extern Bool glXIsDirect (Display *dpy, GLXContext ctx);
extern void glXCopyContext (Display *dpy, GLXContext src, GLXContext dst, GLulong mask);
extern Bool glXMakeCurrent (Display *dpy, GLXDrawable drawable, GLXContext ctx);
extern GLXContext glXGetCurrentContext (void);
extern GLXDrawable glXGetCurrentDrawable (void);
extern void glXWaitGL (void);
extern void glXWaitX (void);
extern void glXSwapBuffers (Display *dpy, GLXDrawable drawable);
extern void glXUseXFont (Font font, int first, int count, int listBase);

#define GLXEW_VERSION_1_0 GLXEW_GET_VAR(__GLXEW_VERSION_1_0)

#endif /* GLX_VERSION_1_0 */

/* ---------------------------- GLX_VERSION_1_1 --------------------------- */

#ifndef GLX_VERSION_1_1
#define GLX_VERSION_1_1

#define GLX_VENDOR 0x1
#define GLX_VERSION 0x2
#define GLX_EXTENSIONS 0x3

extern const char* glXQueryExtensionsString (Display *dpy, int screen);
extern const char* glXGetClientString (Display *dpy, int name);
extern const char* glXQueryServerString (Display *dpy, int screen, int name);

#define GLXEW_VERSION_1_1 GLXEW_GET_VAR(__GLXEW_VERSION_1_1)

#endif /* GLX_VERSION_1_1 */

