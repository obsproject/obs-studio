
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glad/glad.h>

#ifndef __glad_glxext_h_

#ifdef __glxext_h_
#error GLX header already included, remove this include, glad already provides it
#endif

#define __glad_glxext_h_
#define __glxext_h_

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (* GLADloadproc)(const char *name);

#define GLAD_GLAPI_EXPORT

#ifndef GLAPI
# if defined(GLAD_GLAPI_EXPORT)
#  if defined(WIN32) || defined(__CYGWIN__)
#   if defined(GLAD_GLAPI_EXPORT_BUILD)
#    if defined(__GNUC__)
#     define GLAPI __attribute__ ((dllexport)) extern
#    else
#     define GLAPI __declspec(dllexport) extern
#    endif
#   else
#    if defined(__GNUC__)
#     define GLAPI __attribute__ ((dllimport)) extern
#    else
#     define GLAPI __declspec(dllimport) extern
#    endif
#   endif
#  elif defined(__GNUC__) && defined(GLAD_GLAPI_EXPORT_BUILD)
#   define GLAPI __attribute__ ((visibility ("default"))) extern
#  else
#   define GLAPI extern
#  endif
# else
#  define GLAPI extern
# endif
#endif

GLAPI int gladLoadGLX(Display *dpy, int screen);

GLAPI void gladLoadGLXLoader(GLADloadproc, Display *dpy, int screen);

#ifndef GLEXT_64_TYPES_DEFINED
/* This code block is duplicated in glext.h, so must be protected */
#define GLEXT_64_TYPES_DEFINED
/* Define int32_t, int64_t, and uint64_t types for UST/MSC */
/* (as used in the GLX_OML_sync_control extension). */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#include <inttypes.h>
#elif defined(__sun__) || defined(__digital__)
#include <inttypes.h>
#if defined(__STDC__)
#if defined(__arch64__) || defined(_LP64)
typedef long int int64_t;
typedef unsigned long int uint64_t;
#else
typedef long long int int64_t;
typedef unsigned long long int uint64_t;
#endif /* __arch64__ */
#endif /* __STDC__ */
#elif defined( __VMS ) || defined(__sgi)
#include <inttypes.h>
#elif defined(__SCO__) || defined(__USLC__)
#include <stdint.h>
#elif defined(__UNIXOS2__) || defined(__SOL64__)
typedef long int int32_t;
typedef long long int int64_t;
typedef unsigned long long int uint64_t;
#elif defined(_WIN32) && defined(__GNUC__)
#include <stdint.h>
#elif defined(_WIN32)
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
/* Fallback if nothing above works */
#include <inttypes.h>
#endif
#endif
typedef XID GLXFBConfigID;
typedef struct __GLXFBConfigRec *GLXFBConfig;
typedef XID GLXContextID;
typedef struct __GLXcontextRec *GLXContext;
typedef XID GLXPixmap;
typedef XID GLXDrawable;
typedef XID GLXWindow;
typedef XID GLXPbuffer;
typedef void (APIENTRY *__GLXextFuncPtr)(void);
typedef XID GLXVideoCaptureDeviceNV;
typedef unsigned int GLXVideoDeviceNV;
typedef XID GLXVideoSourceSGIX;
typedef XID GLXFBConfigIDSGIX;
typedef struct __GLXFBConfigRec *GLXFBConfigSGIX;
typedef XID GLXPbufferSGIX;
typedef struct {
    int event_type;      /* GLX_DAMAGED or GLX_SAVED */
    int draw_type;       /* GLX_WINDOW or GLX_PBUFFER */
    unsigned long serial;       /* # of last request processed by server */
    Bool send_event;     /* true if this came for SendEvent request */
    Display *display;    /* display the event was read from */
    GLXDrawable drawable;       /* XID of Drawable */
    unsigned int buffer_mask;   /* mask indicating which buffers are affected */
    unsigned int aux_buffer;    /* which aux buffer was affected */
    int x, y;
    int width, height;
    int count;    /* if nonzero, at least this many more */
} GLXPbufferClobberEvent;
typedef struct {
    int type;
    unsigned long serial;       /* # of last request processed by server */
    Bool send_event;     /* true if this came from a SendEvent request */
    Display *display;    /* Display the event was read from */
    GLXDrawable drawable;       /* drawable on which event was requested in event mask */
    int event_type;
    int64_t ust;
    int64_t msc;
    int64_t sbc;
} GLXBufferSwapComplete;
typedef union __GLXEvent {
    GLXPbufferClobberEvent glxpbufferclobber;
    GLXBufferSwapComplete glxbufferswapcomplete;
    long pad[24];
} GLXEvent;
typedef struct {
    int type;
    unsigned long serial;   /* # of last request processed by server */
    Bool send_event; /* true if this came for SendEvent request */
    Display *display;       /* display the event was read from */
    GLXDrawable drawable;   /* i.d. of Drawable */
    int event_type;  /* GLX_DAMAGED_SGIX or GLX_SAVED_SGIX */
    int draw_type;   /* GLX_WINDOW_SGIX or GLX_PBUFFER_SGIX */
    unsigned int mask;      /* mask indicating which buffers are affected*/
    int x, y;
    int width, height;
    int count;       /* if nonzero, at least this many more */
} GLXBufferClobberEventSGIX;
typedef struct {
    char    pipeName[80]; /* Should be [GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX] */
    int     networkId;
} GLXHyperpipeNetworkSGIX;
typedef struct {
    char    pipeName[80]; /* Should be [GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX] */
    int     channel;
    unsigned int participationType;
    int     timeSlice;
} GLXHyperpipeConfigSGIX;
typedef struct {
    char pipeName[80]; /* Should be [GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX] */
    int srcXOrigin, srcYOrigin, srcWidth, srcHeight;
    int destXOrigin, destYOrigin, destWidth, destHeight;
} GLXPipeRect;
typedef struct {
    char pipeName[80]; /* Should be [GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX] */
    int XOrigin, YOrigin, maxHeight, maxWidth;
} GLXPipeRectLimits;
#define GLX_EXTENSION_NAME "GLX"
#define GLX_PbufferClobber 0
#define GLX_BufferSwapComplete 1
#define __GLX_NUMBER_EVENTS 17
#define GLX_BAD_SCREEN 1
#define GLX_BAD_ATTRIBUTE 2
#define GLX_NO_EXTENSION 3
#define GLX_BAD_VISUAL 4
#define GLX_BAD_CONTEXT 5
#define GLX_BAD_VALUE 6
#define GLX_BAD_ENUM 7
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
#define GLX_VENDOR 0x1
#define GLX_VERSION 0x2
#define GLX_EXTENSIONS 0x3
#define GLX_WINDOW_BIT 0x00000001
#define GLX_PIXMAP_BIT 0x00000002
#define GLX_PBUFFER_BIT 0x00000004
#define GLX_RGBA_BIT 0x00000001
#define GLX_COLOR_INDEX_BIT 0x00000002
#define GLX_PBUFFER_CLOBBER_MASK 0x08000000
#define GLX_FRONT_LEFT_BUFFER_BIT 0x00000001
#define GLX_FRONT_RIGHT_BUFFER_BIT 0x00000002
#define GLX_BACK_LEFT_BUFFER_BIT 0x00000004
#define GLX_BACK_RIGHT_BUFFER_BIT 0x00000008
#define GLX_AUX_BUFFERS_BIT 0x00000010
#define GLX_DEPTH_BUFFER_BIT 0x00000020
#define GLX_STENCIL_BUFFER_BIT 0x00000040
#define GLX_ACCUM_BUFFER_BIT 0x00000080
#define GLX_CONFIG_CAVEAT 0x20
#define GLX_X_VISUAL_TYPE 0x22
#define GLX_TRANSPARENT_TYPE 0x23
#define GLX_TRANSPARENT_INDEX_VALUE 0x24
#define GLX_TRANSPARENT_RED_VALUE 0x25
#define GLX_TRANSPARENT_GREEN_VALUE 0x26
#define GLX_TRANSPARENT_BLUE_VALUE 0x27
#define GLX_TRANSPARENT_ALPHA_VALUE 0x28
#define GLX_DONT_CARE 0xFFFFFFFF
#define GLX_NONE 0x8000
#define GLX_SLOW_CONFIG 0x8001
#define GLX_TRUE_COLOR 0x8002
#define GLX_DIRECT_COLOR 0x8003
#define GLX_PSEUDO_COLOR 0x8004
#define GLX_STATIC_COLOR 0x8005
#define GLX_GRAY_SCALE 0x8006
#define GLX_STATIC_GRAY 0x8007
#define GLX_TRANSPARENT_RGB 0x8008
#define GLX_TRANSPARENT_INDEX 0x8009
#define GLX_VISUAL_ID 0x800B
#define GLX_SCREEN 0x800C
#define GLX_NON_CONFORMANT_CONFIG 0x800D
#define GLX_DRAWABLE_TYPE 0x8010
#define GLX_RENDER_TYPE 0x8011
#define GLX_X_RENDERABLE 0x8012
#define GLX_FBCONFIG_ID 0x8013
#define GLX_RGBA_TYPE 0x8014
#define GLX_COLOR_INDEX_TYPE 0x8015
#define GLX_MAX_PBUFFER_WIDTH 0x8016
#define GLX_MAX_PBUFFER_HEIGHT 0x8017
#define GLX_MAX_PBUFFER_PIXELS 0x8018
#define GLX_PRESERVED_CONTENTS 0x801B
#define GLX_LARGEST_PBUFFER 0x801C
#define GLX_WIDTH 0x801D
#define GLX_HEIGHT 0x801E
#define GLX_EVENT_MASK 0x801F
#define GLX_DAMAGED 0x8020
#define GLX_SAVED 0x8021
#define GLX_WINDOW 0x8022
#define GLX_PBUFFER 0x8023
#define GLX_PBUFFER_HEIGHT 0x8040
#define GLX_PBUFFER_WIDTH 0x8041
#define GLX_SAMPLE_BUFFERS 100000
#define GLX_SAMPLES 100001
#ifndef GLX_VERSION_1_0
#define GLX_VERSION_1_0 1
GLAPI int GLAD_GLX_VERSION_1_0;
typedef XVisualInfo* (APIENTRYP PFNGLXCHOOSEVISUALPROC)(Display*, int, int*);
GLAPI PFNGLXCHOOSEVISUALPROC glad_glXChooseVisual;
#define glXChooseVisual glad_glXChooseVisual
typedef GLXContext (APIENTRYP PFNGLXCREATECONTEXTPROC)(Display*, XVisualInfo*, GLXContext, Bool);
GLAPI PFNGLXCREATECONTEXTPROC glad_glXCreateContext;
#define glXCreateContext glad_glXCreateContext
typedef void (APIENTRYP PFNGLXDESTROYCONTEXTPROC)(Display*, GLXContext);
GLAPI PFNGLXDESTROYCONTEXTPROC glad_glXDestroyContext;
#define glXDestroyContext glad_glXDestroyContext
typedef Bool (APIENTRYP PFNGLXMAKECURRENTPROC)(Display*, GLXDrawable, GLXContext);
GLAPI PFNGLXMAKECURRENTPROC glad_glXMakeCurrent;
#define glXMakeCurrent glad_glXMakeCurrent
typedef void (APIENTRYP PFNGLXCOPYCONTEXTPROC)(Display*, GLXContext, GLXContext, unsigned long);
GLAPI PFNGLXCOPYCONTEXTPROC glad_glXCopyContext;
#define glXCopyContext glad_glXCopyContext
typedef void (APIENTRYP PFNGLXSWAPBUFFERSPROC)(Display*, GLXDrawable);
GLAPI PFNGLXSWAPBUFFERSPROC glad_glXSwapBuffers;
#define glXSwapBuffers glad_glXSwapBuffers
typedef GLXPixmap (APIENTRYP PFNGLXCREATEGLXPIXMAPPROC)(Display*, XVisualInfo*, Pixmap);
GLAPI PFNGLXCREATEGLXPIXMAPPROC glad_glXCreateGLXPixmap;
#define glXCreateGLXPixmap glad_glXCreateGLXPixmap
typedef void (APIENTRYP PFNGLXDESTROYGLXPIXMAPPROC)(Display*, GLXPixmap);
GLAPI PFNGLXDESTROYGLXPIXMAPPROC glad_glXDestroyGLXPixmap;
#define glXDestroyGLXPixmap glad_glXDestroyGLXPixmap
typedef Bool (APIENTRYP PFNGLXQUERYEXTENSIONPROC)(Display*, int*, int*);
GLAPI PFNGLXQUERYEXTENSIONPROC glad_glXQueryExtension;
#define glXQueryExtension glad_glXQueryExtension
typedef Bool (APIENTRYP PFNGLXQUERYVERSIONPROC)(Display*, int*, int*);
GLAPI PFNGLXQUERYVERSIONPROC glad_glXQueryVersion;
#define glXQueryVersion glad_glXQueryVersion
typedef Bool (APIENTRYP PFNGLXISDIRECTPROC)(Display*, GLXContext);
GLAPI PFNGLXISDIRECTPROC glad_glXIsDirect;
#define glXIsDirect glad_glXIsDirect
typedef int (APIENTRYP PFNGLXGETCONFIGPROC)(Display*, XVisualInfo*, int, int*);
GLAPI PFNGLXGETCONFIGPROC glad_glXGetConfig;
#define glXGetConfig glad_glXGetConfig
typedef GLXContext (APIENTRYP PFNGLXGETCURRENTCONTEXTPROC)();
GLAPI PFNGLXGETCURRENTCONTEXTPROC glad_glXGetCurrentContext;
#define glXGetCurrentContext glad_glXGetCurrentContext
typedef GLXDrawable (APIENTRYP PFNGLXGETCURRENTDRAWABLEPROC)();
GLAPI PFNGLXGETCURRENTDRAWABLEPROC glad_glXGetCurrentDrawable;
#define glXGetCurrentDrawable glad_glXGetCurrentDrawable
typedef void (APIENTRYP PFNGLXWAITGLPROC)();
GLAPI PFNGLXWAITGLPROC glad_glXWaitGL;
#define glXWaitGL glad_glXWaitGL
typedef void (APIENTRYP PFNGLXWAITXPROC)();
GLAPI PFNGLXWAITXPROC glad_glXWaitX;
#define glXWaitX glad_glXWaitX
typedef void (APIENTRYP PFNGLXUSEXFONTPROC)(Font, int, int, int);
GLAPI PFNGLXUSEXFONTPROC glad_glXUseXFont;
#define glXUseXFont glad_glXUseXFont
#endif
#ifndef GLX_VERSION_1_1
#define GLX_VERSION_1_1 1
GLAPI int GLAD_GLX_VERSION_1_1;
typedef const char* (APIENTRYP PFNGLXQUERYEXTENSIONSSTRINGPROC)(Display*, int);
GLAPI PFNGLXQUERYEXTENSIONSSTRINGPROC glad_glXQueryExtensionsString;
#define glXQueryExtensionsString glad_glXQueryExtensionsString
typedef const char* (APIENTRYP PFNGLXQUERYSERVERSTRINGPROC)(Display*, int, int);
GLAPI PFNGLXQUERYSERVERSTRINGPROC glad_glXQueryServerString;
#define glXQueryServerString glad_glXQueryServerString
typedef const char* (APIENTRYP PFNGLXGETCLIENTSTRINGPROC)(Display*, int);
GLAPI PFNGLXGETCLIENTSTRINGPROC glad_glXGetClientString;
#define glXGetClientString glad_glXGetClientString
#endif
#ifndef GLX_VERSION_1_2
#define GLX_VERSION_1_2 1
GLAPI int GLAD_GLX_VERSION_1_2;
typedef Display* (APIENTRYP PFNGLXGETCURRENTDISPLAYPROC)();
GLAPI PFNGLXGETCURRENTDISPLAYPROC glad_glXGetCurrentDisplay;
#define glXGetCurrentDisplay glad_glXGetCurrentDisplay
#endif
#ifndef GLX_VERSION_1_3
#define GLX_VERSION_1_3 1
GLAPI int GLAD_GLX_VERSION_1_3;
typedef GLXFBConfig* (APIENTRYP PFNGLXGETFBCONFIGSPROC)(Display*, int, int*);
GLAPI PFNGLXGETFBCONFIGSPROC glad_glXGetFBConfigs;
#define glXGetFBConfigs glad_glXGetFBConfigs
typedef GLXFBConfig* (APIENTRYP PFNGLXCHOOSEFBCONFIGPROC)(Display*, int, const int*, int*);
GLAPI PFNGLXCHOOSEFBCONFIGPROC glad_glXChooseFBConfig;
#define glXChooseFBConfig glad_glXChooseFBConfig
typedef int (APIENTRYP PFNGLXGETFBCONFIGATTRIBPROC)(Display*, GLXFBConfig, int, int*);
GLAPI PFNGLXGETFBCONFIGATTRIBPROC glad_glXGetFBConfigAttrib;
#define glXGetFBConfigAttrib glad_glXGetFBConfigAttrib
typedef XVisualInfo* (APIENTRYP PFNGLXGETVISUALFROMFBCONFIGPROC)(Display*, GLXFBConfig);
GLAPI PFNGLXGETVISUALFROMFBCONFIGPROC glad_glXGetVisualFromFBConfig;
#define glXGetVisualFromFBConfig glad_glXGetVisualFromFBConfig
typedef GLXWindow (APIENTRYP PFNGLXCREATEWINDOWPROC)(Display*, GLXFBConfig, Window, const int*);
GLAPI PFNGLXCREATEWINDOWPROC glad_glXCreateWindow;
#define glXCreateWindow glad_glXCreateWindow
typedef void (APIENTRYP PFNGLXDESTROYWINDOWPROC)(Display*, GLXWindow);
GLAPI PFNGLXDESTROYWINDOWPROC glad_glXDestroyWindow;
#define glXDestroyWindow glad_glXDestroyWindow
typedef GLXPixmap (APIENTRYP PFNGLXCREATEPIXMAPPROC)(Display*, GLXFBConfig, Pixmap, const int*);
GLAPI PFNGLXCREATEPIXMAPPROC glad_glXCreatePixmap;
#define glXCreatePixmap glad_glXCreatePixmap
typedef void (APIENTRYP PFNGLXDESTROYPIXMAPPROC)(Display*, GLXPixmap);
GLAPI PFNGLXDESTROYPIXMAPPROC glad_glXDestroyPixmap;
#define glXDestroyPixmap glad_glXDestroyPixmap
typedef GLXPbuffer (APIENTRYP PFNGLXCREATEPBUFFERPROC)(Display*, GLXFBConfig, const int*);
GLAPI PFNGLXCREATEPBUFFERPROC glad_glXCreatePbuffer;
#define glXCreatePbuffer glad_glXCreatePbuffer
typedef void (APIENTRYP PFNGLXDESTROYPBUFFERPROC)(Display*, GLXPbuffer);
GLAPI PFNGLXDESTROYPBUFFERPROC glad_glXDestroyPbuffer;
#define glXDestroyPbuffer glad_glXDestroyPbuffer
typedef void (APIENTRYP PFNGLXQUERYDRAWABLEPROC)(Display*, GLXDrawable, int, unsigned int*);
GLAPI PFNGLXQUERYDRAWABLEPROC glad_glXQueryDrawable;
#define glXQueryDrawable glad_glXQueryDrawable
typedef GLXContext (APIENTRYP PFNGLXCREATENEWCONTEXTPROC)(Display*, GLXFBConfig, int, GLXContext, Bool);
GLAPI PFNGLXCREATENEWCONTEXTPROC glad_glXCreateNewContext;
#define glXCreateNewContext glad_glXCreateNewContext
typedef Bool (APIENTRYP PFNGLXMAKECONTEXTCURRENTPROC)(Display*, GLXDrawable, GLXDrawable, GLXContext);
GLAPI PFNGLXMAKECONTEXTCURRENTPROC glad_glXMakeContextCurrent;
#define glXMakeContextCurrent glad_glXMakeContextCurrent
typedef GLXDrawable (APIENTRYP PFNGLXGETCURRENTREADDRAWABLEPROC)();
GLAPI PFNGLXGETCURRENTREADDRAWABLEPROC glad_glXGetCurrentReadDrawable;
#define glXGetCurrentReadDrawable glad_glXGetCurrentReadDrawable
typedef int (APIENTRYP PFNGLXQUERYCONTEXTPROC)(Display*, GLXContext, int, int*);
GLAPI PFNGLXQUERYCONTEXTPROC glad_glXQueryContext;
#define glXQueryContext glad_glXQueryContext
typedef void (APIENTRYP PFNGLXSELECTEVENTPROC)(Display*, GLXDrawable, unsigned long);
GLAPI PFNGLXSELECTEVENTPROC glad_glXSelectEvent;
#define glXSelectEvent glad_glXSelectEvent
typedef void (APIENTRYP PFNGLXGETSELECTEDEVENTPROC)(Display*, GLXDrawable, unsigned long*);
GLAPI PFNGLXGETSELECTEDEVENTPROC glad_glXGetSelectedEvent;
#define glXGetSelectedEvent glad_glXGetSelectedEvent
#endif
#ifndef GLX_VERSION_1_4
#define GLX_VERSION_1_4 1
GLAPI int GLAD_GLX_VERSION_1_4;
typedef __GLXextFuncPtr (APIENTRYP PFNGLXGETPROCADDRESSPROC)(const GLubyte*);
GLAPI PFNGLXGETPROCADDRESSPROC glad_glXGetProcAddress;
#define glXGetProcAddress glad_glXGetProcAddress
#endif
#define GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20B2
#define GLX_SHARE_CONTEXT_EXT 0x800A
#define GLX_VISUAL_ID_EXT 0x800B
#define GLX_SCREEN_EXT 0x800C
#define GLX_COVERAGE_SAMPLES_NV 100001
#define GLX_COLOR_SAMPLES_NV 0x20B3
#define GLX_MULTISAMPLE_SUB_RECT_WIDTH_SGIS 0x8026
#define GLX_MULTISAMPLE_SUB_RECT_HEIGHT_SGIS 0x8027
#define GLX_PBUFFER_BIT_SGIX 0x00000004
#define GLX_BUFFER_CLOBBER_MASK_SGIX 0x08000000
#define GLX_FRONT_LEFT_BUFFER_BIT_SGIX 0x00000001
#define GLX_FRONT_RIGHT_BUFFER_BIT_SGIX 0x00000002
#define GLX_BACK_LEFT_BUFFER_BIT_SGIX 0x00000004
#define GLX_BACK_RIGHT_BUFFER_BIT_SGIX 0x00000008
#define GLX_AUX_BUFFERS_BIT_SGIX 0x00000010
#define GLX_DEPTH_BUFFER_BIT_SGIX 0x00000020
#define GLX_STENCIL_BUFFER_BIT_SGIX 0x00000040
#define GLX_ACCUM_BUFFER_BIT_SGIX 0x00000080
#define GLX_SAMPLE_BUFFERS_BIT_SGIX 0x00000100
#define GLX_MAX_PBUFFER_WIDTH_SGIX 0x8016
#define GLX_MAX_PBUFFER_HEIGHT_SGIX 0x8017
#define GLX_MAX_PBUFFER_PIXELS_SGIX 0x8018
#define GLX_OPTIMAL_PBUFFER_WIDTH_SGIX 0x8019
#define GLX_OPTIMAL_PBUFFER_HEIGHT_SGIX 0x801A
#define GLX_PRESERVED_CONTENTS_SGIX 0x801B
#define GLX_LARGEST_PBUFFER_SGIX 0x801C
#define GLX_WIDTH_SGIX 0x801D
#define GLX_HEIGHT_SGIX 0x801E
#define GLX_EVENT_MASK_SGIX 0x801F
#define GLX_DAMAGED_SGIX 0x8020
#define GLX_SAVED_SGIX 0x8021
#define GLX_WINDOW_SGIX 0x8022
#define GLX_PBUFFER_SGIX 0x8023
#define GLX_RGBA_FLOAT_TYPE_ARB 0x20B9
#define GLX_RGBA_FLOAT_BIT_ARB 0x00000004
#define GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX 80
#define GLX_BAD_HYPERPIPE_CONFIG_SGIX 91
#define GLX_BAD_HYPERPIPE_SGIX 92
#define GLX_HYPERPIPE_DISPLAY_PIPE_SGIX 0x00000001
#define GLX_HYPERPIPE_RENDER_PIPE_SGIX 0x00000002
#define GLX_PIPE_RECT_SGIX 0x00000001
#define GLX_PIPE_RECT_LIMITS_SGIX 0x00000002
#define GLX_HYPERPIPE_STEREO_SGIX 0x00000003
#define GLX_HYPERPIPE_PIXEL_AVERAGE_SGIX 0x00000004
#define GLX_HYPERPIPE_ID_SGIX 0x8030
#define GLX_CONTEXT_RESET_ISOLATION_BIT_ARB 0x00000008
#define GLX_BUFFER_SWAP_COMPLETE_INTEL_MASK 0x04000000
#define GLX_EXCHANGE_COMPLETE_INTEL 0x8180
#define GLX_COPY_COMPLETE_INTEL 0x8181
#define GLX_FLIP_COMPLETE_INTEL 0x8182
#define GLX_SYNC_FRAME_SGIX 0x00000000
#define GLX_SYNC_SWAP_SGIX 0x00000001
#define GLX_CONTEXT_ES2_PROFILE_BIT_EXT 0x00000004
#define GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT 0x20B2
#define GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT 0x20B1
#define GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT 0x00000008
#define GLX_BACK_BUFFER_AGE_EXT 0x20F4
#define GLX_SAMPLE_BUFFERS_3DFX 0x8050
#define GLX_SAMPLES_3DFX 0x8051
#define GLX_X_VISUAL_TYPE_EXT 0x22
#define GLX_TRANSPARENT_TYPE_EXT 0x23
#define GLX_TRANSPARENT_INDEX_VALUE_EXT 0x24
#define GLX_TRANSPARENT_RED_VALUE_EXT 0x25
#define GLX_TRANSPARENT_GREEN_VALUE_EXT 0x26
#define GLX_TRANSPARENT_BLUE_VALUE_EXT 0x27
#define GLX_TRANSPARENT_ALPHA_VALUE_EXT 0x28
#define GLX_NONE_EXT 0x8000
#define GLX_TRUE_COLOR_EXT 0x8002
#define GLX_DIRECT_COLOR_EXT 0x8003
#define GLX_PSEUDO_COLOR_EXT 0x8004
#define GLX_STATIC_COLOR_EXT 0x8005
#define GLX_GRAY_SCALE_EXT 0x8006
#define GLX_STATIC_GRAY_EXT 0x8007
#define GLX_TRANSPARENT_RGB_EXT 0x8008
#define GLX_TRANSPARENT_INDEX_EXT 0x8009
#define GLX_SAMPLE_BUFFERS_SGIS 100000
#define GLX_SAMPLES_SGIS 100001
#define GLX_3DFX_WINDOW_MODE_MESA 0x1
#define GLX_3DFX_FULLSCREEN_MODE_MESA 0x2
#define GLX_TEXTURE_1D_BIT_EXT 0x00000001
#define GLX_TEXTURE_2D_BIT_EXT 0x00000002
#define GLX_TEXTURE_RECTANGLE_BIT_EXT 0x00000004
#define GLX_BIND_TO_TEXTURE_RGB_EXT 0x20D0
#define GLX_BIND_TO_TEXTURE_RGBA_EXT 0x20D1
#define GLX_BIND_TO_MIPMAP_TEXTURE_EXT 0x20D2
#define GLX_BIND_TO_TEXTURE_TARGETS_EXT 0x20D3
#define GLX_Y_INVERTED_EXT 0x20D4
#define GLX_TEXTURE_FORMAT_EXT 0x20D5
#define GLX_TEXTURE_TARGET_EXT 0x20D6
#define GLX_MIPMAP_TEXTURE_EXT 0x20D7
#define GLX_TEXTURE_FORMAT_NONE_EXT 0x20D8
#define GLX_TEXTURE_FORMAT_RGB_EXT 0x20D9
#define GLX_TEXTURE_FORMAT_RGBA_EXT 0x20DA
#define GLX_TEXTURE_1D_EXT 0x20DB
#define GLX_TEXTURE_2D_EXT 0x20DC
#define GLX_TEXTURE_RECTANGLE_EXT 0x20DD
#define GLX_FRONT_LEFT_EXT 0x20DE
#define GLX_FRONT_RIGHT_EXT 0x20DF
#define GLX_BACK_LEFT_EXT 0x20E0
#define GLX_BACK_RIGHT_EXT 0x20E1
#define GLX_FRONT_EXT 0x20DE
#define GLX_BACK_EXT 0x20E0
#define GLX_AUX0_EXT 0x20E2
#define GLX_AUX1_EXT 0x20E3
#define GLX_AUX2_EXT 0x20E4
#define GLX_AUX3_EXT 0x20E5
#define GLX_AUX4_EXT 0x20E6
#define GLX_AUX5_EXT 0x20E7
#define GLX_AUX6_EXT 0x20E8
#define GLX_AUX7_EXT 0x20E9
#define GLX_AUX8_EXT 0x20EA
#define GLX_AUX9_EXT 0x20EB
#define GLX_DEVICE_ID_NV 0x20CD
#define GLX_UNIQUE_ID_NV 0x20CE
#define GLX_NUM_VIDEO_CAPTURE_SLOTS_NV 0x20CF
#define GLX_SAMPLE_BUFFERS_ARB 100000
#define GLX_SAMPLES_ARB 100001
#define GLX_SWAP_INTERVAL_EXT 0x20F1
#define GLX_MAX_SWAP_INTERVAL_EXT 0x20F2
#define GLX_RENDERER_VENDOR_ID_MESA 0x8183
#define GLX_RENDERER_DEVICE_ID_MESA 0x8184
#define GLX_RENDERER_VERSION_MESA 0x8185
#define GLX_RENDERER_ACCELERATED_MESA 0x8186
#define GLX_RENDERER_VIDEO_MEMORY_MESA 0x8187
#define GLX_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_MESA 0x8188
#define GLX_RENDERER_PREFERRED_PROFILE_MESA 0x8189
#define GLX_RENDERER_OPENGL_CORE_PROFILE_VERSION_MESA 0x818A
#define GLX_RENDERER_OPENGL_COMPATIBILITY_PROFILE_VERSION_MESA 0x818B
#define GLX_RENDERER_OPENGL_ES_PROFILE_VERSION_MESA 0x818C
#define GLX_RENDERER_OPENGL_ES2_PROFILE_VERSION_MESA 0x818D
#define GLX_RENDERER_ID_MESA 0x818E
#define GLX_CONTEXT_DEBUG_BIT_ARB 0x00000001
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
#define GLX_CONTEXT_FLAGS_ARB 0x2094
#define GLX_CONTEXT_ES_PROFILE_BIT_EXT 0x00000004
#define GLX_WINDOW_BIT_SGIX 0x00000001
#define GLX_PIXMAP_BIT_SGIX 0x00000002
#define GLX_RGBA_BIT_SGIX 0x00000001
#define GLX_COLOR_INDEX_BIT_SGIX 0x00000002
#define GLX_DRAWABLE_TYPE_SGIX 0x8010
#define GLX_RENDER_TYPE_SGIX 0x8011
#define GLX_X_RENDERABLE_SGIX 0x8012
#define GLX_FBCONFIG_ID_SGIX 0x8013
#define GLX_RGBA_TYPE_SGIX 0x8014
#define GLX_COLOR_INDEX_TYPE_SGIX 0x8015
#define GLX_VISUAL_SELECT_GROUP_SGIX 0x8028
#define GLX_VIDEO_OUT_COLOR_NV 0x20C3
#define GLX_VIDEO_OUT_ALPHA_NV 0x20C4
#define GLX_VIDEO_OUT_DEPTH_NV 0x20C5
#define GLX_VIDEO_OUT_COLOR_AND_ALPHA_NV 0x20C6
#define GLX_VIDEO_OUT_COLOR_AND_DEPTH_NV 0x20C7
#define GLX_VIDEO_OUT_FRAME_NV 0x20C8
#define GLX_VIDEO_OUT_FIELD_1_NV 0x20C9
#define GLX_VIDEO_OUT_FIELD_2_NV 0x20CA
#define GLX_VIDEO_OUT_STACKED_FIELDS_1_2_NV 0x20CB
#define GLX_VIDEO_OUT_STACKED_FIELDS_2_1_NV 0x20CC
#define GLX_BLENDED_RGBA_SGIS 0x8025
#define GLX_DIGITAL_MEDIA_PBUFFER_SGIX 0x8024
#define GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB 0x00000004
#define GLX_LOSE_CONTEXT_ON_RESET_ARB 0x8252
#define GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define GLX_NO_RESET_NOTIFICATION_ARB 0x8261
#define GLX_LATE_SWAPS_TEAR_EXT 0x20F3
#define GLX_VISUAL_CAVEAT_EXT 0x20
#define GLX_SLOW_VISUAL_EXT 0x8001
#define GLX_NON_CONFORMANT_VISUAL_EXT 0x800D
#define GLX_FLOAT_COMPONENTS_NV 0x20B0
#define GLX_SWAP_METHOD_OML 0x8060
#define GLX_SWAP_EXCHANGE_OML 0x8061
#define GLX_SWAP_COPY_OML 0x8062
#define GLX_SWAP_UNDEFINED_OML 0x8063
#define GLX_NUM_VIDEO_SLOTS_NV 0x20F0
#define GLX_GPU_VENDOR_AMD 0x1F00
#define GLX_GPU_RENDERER_STRING_AMD 0x1F01
#define GLX_GPU_OPENGL_VERSION_STRING_AMD 0x1F02
#define GLX_GPU_FASTEST_TARGET_GPUS_AMD 0x21A2
#define GLX_GPU_RAM_AMD 0x21A3
#define GLX_GPU_CLOCK_AMD 0x21A4
#define GLX_GPU_NUM_PIPES_AMD 0x21A5
#define GLX_GPU_NUM_SIMD_AMD 0x21A6
#define GLX_GPU_NUM_RB_AMD 0x21A7
#define GLX_GPU_NUM_SPI_AMD 0x21A8
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define GLX_CONTEXT_PROFILE_MASK_ARB 0x9126
#define GLX_CONTEXT_ALLOW_BUFFER_BYTE_ORDER_MISMATCH_ARB 0x2095
#ifndef GLX_ARB_framebuffer_sRGB
#define GLX_ARB_framebuffer_sRGB 1
GLAPI int GLAD_GLX_ARB_framebuffer_sRGB;
#endif
#ifndef GLX_EXT_import_context
#define GLX_EXT_import_context 1
GLAPI int GLAD_GLX_EXT_import_context;
typedef Display* (APIENTRYP PFNGLXGETCURRENTDISPLAYEXTPROC)();
GLAPI PFNGLXGETCURRENTDISPLAYEXTPROC glad_glXGetCurrentDisplayEXT;
#define glXGetCurrentDisplayEXT glad_glXGetCurrentDisplayEXT
typedef int (APIENTRYP PFNGLXQUERYCONTEXTINFOEXTPROC)(Display*, GLXContext, int, int*);
GLAPI PFNGLXQUERYCONTEXTINFOEXTPROC glad_glXQueryContextInfoEXT;
#define glXQueryContextInfoEXT glad_glXQueryContextInfoEXT
typedef GLXContextID (APIENTRYP PFNGLXGETCONTEXTIDEXTPROC)(const GLXContext);
GLAPI PFNGLXGETCONTEXTIDEXTPROC glad_glXGetContextIDEXT;
#define glXGetContextIDEXT glad_glXGetContextIDEXT
typedef GLXContext (APIENTRYP PFNGLXIMPORTCONTEXTEXTPROC)(Display*, GLXContextID);
GLAPI PFNGLXIMPORTCONTEXTEXTPROC glad_glXImportContextEXT;
#define glXImportContextEXT glad_glXImportContextEXT
typedef void (APIENTRYP PFNGLXFREECONTEXTEXTPROC)(Display*, GLXContext);
GLAPI PFNGLXFREECONTEXTEXTPROC glad_glXFreeContextEXT;
#define glXFreeContextEXT glad_glXFreeContextEXT
#endif
#ifndef GLX_NV_multisample_coverage
#define GLX_NV_multisample_coverage 1
GLAPI int GLAD_GLX_NV_multisample_coverage;
#endif
#ifndef GLX_SGIS_shared_multisample
#define GLX_SGIS_shared_multisample 1
GLAPI int GLAD_GLX_SGIS_shared_multisample;
#endif
#ifndef GLX_SGIX_pbuffer
#define GLX_SGIX_pbuffer 1
GLAPI int GLAD_GLX_SGIX_pbuffer;
typedef GLXPbufferSGIX (APIENTRYP PFNGLXCREATEGLXPBUFFERSGIXPROC)(Display*, GLXFBConfigSGIX, unsigned int, unsigned int, int*);
GLAPI PFNGLXCREATEGLXPBUFFERSGIXPROC glad_glXCreateGLXPbufferSGIX;
#define glXCreateGLXPbufferSGIX glad_glXCreateGLXPbufferSGIX
typedef void (APIENTRYP PFNGLXDESTROYGLXPBUFFERSGIXPROC)(Display*, GLXPbufferSGIX);
GLAPI PFNGLXDESTROYGLXPBUFFERSGIXPROC glad_glXDestroyGLXPbufferSGIX;
#define glXDestroyGLXPbufferSGIX glad_glXDestroyGLXPbufferSGIX
typedef int (APIENTRYP PFNGLXQUERYGLXPBUFFERSGIXPROC)(Display*, GLXPbufferSGIX, int, unsigned int*);
GLAPI PFNGLXQUERYGLXPBUFFERSGIXPROC glad_glXQueryGLXPbufferSGIX;
#define glXQueryGLXPbufferSGIX glad_glXQueryGLXPbufferSGIX
typedef void (APIENTRYP PFNGLXSELECTEVENTSGIXPROC)(Display*, GLXDrawable, unsigned long);
GLAPI PFNGLXSELECTEVENTSGIXPROC glad_glXSelectEventSGIX;
#define glXSelectEventSGIX glad_glXSelectEventSGIX
typedef void (APIENTRYP PFNGLXGETSELECTEDEVENTSGIXPROC)(Display*, GLXDrawable, unsigned long*);
GLAPI PFNGLXGETSELECTEDEVENTSGIXPROC glad_glXGetSelectedEventSGIX;
#define glXGetSelectedEventSGIX glad_glXGetSelectedEventSGIX
#endif
#ifndef GLX_NV_swap_group
#define GLX_NV_swap_group 1
GLAPI int GLAD_GLX_NV_swap_group;
typedef Bool (APIENTRYP PFNGLXJOINSWAPGROUPNVPROC)(Display*, GLXDrawable, GLuint);
GLAPI PFNGLXJOINSWAPGROUPNVPROC glad_glXJoinSwapGroupNV;
#define glXJoinSwapGroupNV glad_glXJoinSwapGroupNV
typedef Bool (APIENTRYP PFNGLXBINDSWAPBARRIERNVPROC)(Display*, GLuint, GLuint);
GLAPI PFNGLXBINDSWAPBARRIERNVPROC glad_glXBindSwapBarrierNV;
#define glXBindSwapBarrierNV glad_glXBindSwapBarrierNV
typedef Bool (APIENTRYP PFNGLXQUERYSWAPGROUPNVPROC)(Display*, GLXDrawable, GLuint*, GLuint*);
GLAPI PFNGLXQUERYSWAPGROUPNVPROC glad_glXQuerySwapGroupNV;
#define glXQuerySwapGroupNV glad_glXQuerySwapGroupNV
typedef Bool (APIENTRYP PFNGLXQUERYMAXSWAPGROUPSNVPROC)(Display*, int, GLuint*, GLuint*);
GLAPI PFNGLXQUERYMAXSWAPGROUPSNVPROC glad_glXQueryMaxSwapGroupsNV;
#define glXQueryMaxSwapGroupsNV glad_glXQueryMaxSwapGroupsNV
typedef Bool (APIENTRYP PFNGLXQUERYFRAMECOUNTNVPROC)(Display*, int, GLuint*);
GLAPI PFNGLXQUERYFRAMECOUNTNVPROC glad_glXQueryFrameCountNV;
#define glXQueryFrameCountNV glad_glXQueryFrameCountNV
typedef Bool (APIENTRYP PFNGLXRESETFRAMECOUNTNVPROC)(Display*, int);
GLAPI PFNGLXRESETFRAMECOUNTNVPROC glad_glXResetFrameCountNV;
#define glXResetFrameCountNV glad_glXResetFrameCountNV
#endif
#ifndef GLX_ARB_fbconfig_float
#define GLX_ARB_fbconfig_float 1
GLAPI int GLAD_GLX_ARB_fbconfig_float;
#endif
#ifndef GLX_SGIX_hyperpipe
#define GLX_SGIX_hyperpipe 1
GLAPI int GLAD_GLX_SGIX_hyperpipe;
typedef GLXHyperpipeNetworkSGIX* (APIENTRYP PFNGLXQUERYHYPERPIPENETWORKSGIXPROC)(Display*, int*);
GLAPI PFNGLXQUERYHYPERPIPENETWORKSGIXPROC glad_glXQueryHyperpipeNetworkSGIX;
#define glXQueryHyperpipeNetworkSGIX glad_glXQueryHyperpipeNetworkSGIX
typedef int (APIENTRYP PFNGLXHYPERPIPECONFIGSGIXPROC)(Display*, int, int, GLXHyperpipeConfigSGIX*, int*);
GLAPI PFNGLXHYPERPIPECONFIGSGIXPROC glad_glXHyperpipeConfigSGIX;
#define glXHyperpipeConfigSGIX glad_glXHyperpipeConfigSGIX
typedef GLXHyperpipeConfigSGIX* (APIENTRYP PFNGLXQUERYHYPERPIPECONFIGSGIXPROC)(Display*, int, int*);
GLAPI PFNGLXQUERYHYPERPIPECONFIGSGIXPROC glad_glXQueryHyperpipeConfigSGIX;
#define glXQueryHyperpipeConfigSGIX glad_glXQueryHyperpipeConfigSGIX
typedef int (APIENTRYP PFNGLXDESTROYHYPERPIPECONFIGSGIXPROC)(Display*, int);
GLAPI PFNGLXDESTROYHYPERPIPECONFIGSGIXPROC glad_glXDestroyHyperpipeConfigSGIX;
#define glXDestroyHyperpipeConfigSGIX glad_glXDestroyHyperpipeConfigSGIX
typedef int (APIENTRYP PFNGLXBINDHYPERPIPESGIXPROC)(Display*, int);
GLAPI PFNGLXBINDHYPERPIPESGIXPROC glad_glXBindHyperpipeSGIX;
#define glXBindHyperpipeSGIX glad_glXBindHyperpipeSGIX
typedef int (APIENTRYP PFNGLXQUERYHYPERPIPEBESTATTRIBSGIXPROC)(Display*, int, int, int, void*, void*);
GLAPI PFNGLXQUERYHYPERPIPEBESTATTRIBSGIXPROC glad_glXQueryHyperpipeBestAttribSGIX;
#define glXQueryHyperpipeBestAttribSGIX glad_glXQueryHyperpipeBestAttribSGIX
typedef int (APIENTRYP PFNGLXHYPERPIPEATTRIBSGIXPROC)(Display*, int, int, int, void*);
GLAPI PFNGLXHYPERPIPEATTRIBSGIXPROC glad_glXHyperpipeAttribSGIX;
#define glXHyperpipeAttribSGIX glad_glXHyperpipeAttribSGIX
typedef int (APIENTRYP PFNGLXQUERYHYPERPIPEATTRIBSGIXPROC)(Display*, int, int, int, void*);
GLAPI PFNGLXQUERYHYPERPIPEATTRIBSGIXPROC glad_glXQueryHyperpipeAttribSGIX;
#define glXQueryHyperpipeAttribSGIX glad_glXQueryHyperpipeAttribSGIX
#endif
#ifndef GLX_ARB_robustness_share_group_isolation
#define GLX_ARB_robustness_share_group_isolation 1
GLAPI int GLAD_GLX_ARB_robustness_share_group_isolation;
#endif
#ifndef GLX_INTEL_swap_event
#define GLX_INTEL_swap_event 1
GLAPI int GLAD_GLX_INTEL_swap_event;
#endif
#ifndef GLX_SGIX_video_resize
#define GLX_SGIX_video_resize 1
GLAPI int GLAD_GLX_SGIX_video_resize;
typedef int (APIENTRYP PFNGLXBINDCHANNELTOWINDOWSGIXPROC)(Display*, int, int, Window);
GLAPI PFNGLXBINDCHANNELTOWINDOWSGIXPROC glad_glXBindChannelToWindowSGIX;
#define glXBindChannelToWindowSGIX glad_glXBindChannelToWindowSGIX
typedef int (APIENTRYP PFNGLXCHANNELRECTSGIXPROC)(Display*, int, int, int, int, int, int);
GLAPI PFNGLXCHANNELRECTSGIXPROC glad_glXChannelRectSGIX;
#define glXChannelRectSGIX glad_glXChannelRectSGIX
typedef int (APIENTRYP PFNGLXQUERYCHANNELRECTSGIXPROC)(Display*, int, int, int*, int*, int*, int*);
GLAPI PFNGLXQUERYCHANNELRECTSGIXPROC glad_glXQueryChannelRectSGIX;
#define glXQueryChannelRectSGIX glad_glXQueryChannelRectSGIX
typedef int (APIENTRYP PFNGLXQUERYCHANNELDELTASSGIXPROC)(Display*, int, int, int*, int*, int*, int*);
GLAPI PFNGLXQUERYCHANNELDELTASSGIXPROC glad_glXQueryChannelDeltasSGIX;
#define glXQueryChannelDeltasSGIX glad_glXQueryChannelDeltasSGIX
typedef int (APIENTRYP PFNGLXCHANNELRECTSYNCSGIXPROC)(Display*, int, int, GLenum);
GLAPI PFNGLXCHANNELRECTSYNCSGIXPROC glad_glXChannelRectSyncSGIX;
#define glXChannelRectSyncSGIX glad_glXChannelRectSyncSGIX
#endif
#ifndef GLX_EXT_create_context_es2_profile
#define GLX_EXT_create_context_es2_profile 1
GLAPI int GLAD_GLX_EXT_create_context_es2_profile;
#endif
#ifndef GLX_ARB_robustness_application_isolation
#define GLX_ARB_robustness_application_isolation 1
GLAPI int GLAD_GLX_ARB_robustness_application_isolation;
#endif
#ifndef GLX_NV_copy_image
#define GLX_NV_copy_image 1
GLAPI int GLAD_GLX_NV_copy_image;
typedef void (APIENTRYP PFNGLXCOPYIMAGESUBDATANVPROC)(Display*, GLXContext, GLuint, GLenum, GLint, GLint, GLint, GLint, GLXContext, GLuint, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei);
GLAPI PFNGLXCOPYIMAGESUBDATANVPROC glad_glXCopyImageSubDataNV;
#define glXCopyImageSubDataNV glad_glXCopyImageSubDataNV
#endif
#ifndef GLX_OML_sync_control
#define GLX_OML_sync_control 1
GLAPI int GLAD_GLX_OML_sync_control;
typedef Bool (APIENTRYP PFNGLXGETSYNCVALUESOMLPROC)(Display*, GLXDrawable, int64_t*, int64_t*, int64_t*);
GLAPI PFNGLXGETSYNCVALUESOMLPROC glad_glXGetSyncValuesOML;
#define glXGetSyncValuesOML glad_glXGetSyncValuesOML
typedef Bool (APIENTRYP PFNGLXGETMSCRATEOMLPROC)(Display*, GLXDrawable, int32_t*, int32_t*);
GLAPI PFNGLXGETMSCRATEOMLPROC glad_glXGetMscRateOML;
#define glXGetMscRateOML glad_glXGetMscRateOML
typedef int64_t (APIENTRYP PFNGLXSWAPBUFFERSMSCOMLPROC)(Display*, GLXDrawable, int64_t, int64_t, int64_t);
GLAPI PFNGLXSWAPBUFFERSMSCOMLPROC glad_glXSwapBuffersMscOML;
#define glXSwapBuffersMscOML glad_glXSwapBuffersMscOML
typedef Bool (APIENTRYP PFNGLXWAITFORMSCOMLPROC)(Display*, GLXDrawable, int64_t, int64_t, int64_t, int64_t*, int64_t*, int64_t*);
GLAPI PFNGLXWAITFORMSCOMLPROC glad_glXWaitForMscOML;
#define glXWaitForMscOML glad_glXWaitForMscOML
typedef Bool (APIENTRYP PFNGLXWAITFORSBCOMLPROC)(Display*, GLXDrawable, int64_t, int64_t*, int64_t*, int64_t*);
GLAPI PFNGLXWAITFORSBCOMLPROC glad_glXWaitForSbcOML;
#define glXWaitForSbcOML glad_glXWaitForSbcOML
#endif
#ifndef GLX_EXT_framebuffer_sRGB
#define GLX_EXT_framebuffer_sRGB 1
GLAPI int GLAD_GLX_EXT_framebuffer_sRGB;
#endif
#ifndef GLX_SGI_make_current_read
#define GLX_SGI_make_current_read 1
GLAPI int GLAD_GLX_SGI_make_current_read;
typedef Bool (APIENTRYP PFNGLXMAKECURRENTREADSGIPROC)(Display*, GLXDrawable, GLXDrawable, GLXContext);
GLAPI PFNGLXMAKECURRENTREADSGIPROC glad_glXMakeCurrentReadSGI;
#define glXMakeCurrentReadSGI glad_glXMakeCurrentReadSGI
typedef GLXDrawable (APIENTRYP PFNGLXGETCURRENTREADDRAWABLESGIPROC)();
GLAPI PFNGLXGETCURRENTREADDRAWABLESGIPROC glad_glXGetCurrentReadDrawableSGI;
#define glXGetCurrentReadDrawableSGI glad_glXGetCurrentReadDrawableSGI
#endif
#ifndef GLX_MESA_swap_control
#define GLX_MESA_swap_control 1
GLAPI int GLAD_GLX_MESA_swap_control;
typedef int (APIENTRYP PFNGLXSWAPINTERVALMESAPROC)(int);
GLAPI PFNGLXSWAPINTERVALMESAPROC glad_glXSwapIntervalMESA;
#define glXSwapIntervalMESA glad_glXSwapIntervalMESA
#endif
#ifndef GLX_SGI_swap_control
#define GLX_SGI_swap_control 1
GLAPI int GLAD_GLX_SGI_swap_control;
typedef int (APIENTRYP PFNGLXSWAPINTERVALSGIPROC)(int);
GLAPI PFNGLXSWAPINTERVALSGIPROC glad_glXSwapIntervalSGI;
#define glXSwapIntervalSGI glad_glXSwapIntervalSGI
#endif
#ifndef GLX_EXT_fbconfig_packed_float
#define GLX_EXT_fbconfig_packed_float 1
GLAPI int GLAD_GLX_EXT_fbconfig_packed_float;
#endif
#ifndef GLX_EXT_buffer_age
#define GLX_EXT_buffer_age 1
GLAPI int GLAD_GLX_EXT_buffer_age;
#endif
#ifndef GLX_3DFX_multisample
#define GLX_3DFX_multisample 1
GLAPI int GLAD_GLX_3DFX_multisample;
#endif
#ifndef GLX_EXT_visual_info
#define GLX_EXT_visual_info 1
GLAPI int GLAD_GLX_EXT_visual_info;
#endif
#ifndef GLX_SGI_video_sync
#define GLX_SGI_video_sync 1
GLAPI int GLAD_GLX_SGI_video_sync;
typedef int (APIENTRYP PFNGLXGETVIDEOSYNCSGIPROC)(unsigned int*);
GLAPI PFNGLXGETVIDEOSYNCSGIPROC glad_glXGetVideoSyncSGI;
#define glXGetVideoSyncSGI glad_glXGetVideoSyncSGI
typedef int (APIENTRYP PFNGLXWAITVIDEOSYNCSGIPROC)(int, int, unsigned int*);
GLAPI PFNGLXWAITVIDEOSYNCSGIPROC glad_glXWaitVideoSyncSGI;
#define glXWaitVideoSyncSGI glad_glXWaitVideoSyncSGI
#endif
#ifndef GLX_MESA_agp_offset
#define GLX_MESA_agp_offset 1
GLAPI int GLAD_GLX_MESA_agp_offset;
typedef unsigned int (APIENTRYP PFNGLXGETAGPOFFSETMESAPROC)(const void*);
GLAPI PFNGLXGETAGPOFFSETMESAPROC glad_glXGetAGPOffsetMESA;
#define glXGetAGPOffsetMESA glad_glXGetAGPOffsetMESA
#endif
#ifndef GLX_SGIS_multisample
#define GLX_SGIS_multisample 1
GLAPI int GLAD_GLX_SGIS_multisample;
#endif
#ifndef GLX_MESA_set_3dfx_mode
#define GLX_MESA_set_3dfx_mode 1
GLAPI int GLAD_GLX_MESA_set_3dfx_mode;
typedef Bool (APIENTRYP PFNGLXSET3DFXMODEMESAPROC)(int);
GLAPI PFNGLXSET3DFXMODEMESAPROC glad_glXSet3DfxModeMESA;
#define glXSet3DfxModeMESA glad_glXSet3DfxModeMESA
#endif
#ifndef GLX_EXT_texture_from_pixmap
#define GLX_EXT_texture_from_pixmap 1
GLAPI int GLAD_GLX_EXT_texture_from_pixmap;
typedef void (APIENTRYP PFNGLXBINDTEXIMAGEEXTPROC)(Display*, GLXDrawable, int, const int*);
GLAPI PFNGLXBINDTEXIMAGEEXTPROC glad_glXBindTexImageEXT;
#define glXBindTexImageEXT glad_glXBindTexImageEXT
typedef void (APIENTRYP PFNGLXRELEASETEXIMAGEEXTPROC)(Display*, GLXDrawable, int);
GLAPI PFNGLXRELEASETEXIMAGEEXTPROC glad_glXReleaseTexImageEXT;
#define glXReleaseTexImageEXT glad_glXReleaseTexImageEXT
#endif
#ifndef GLX_NV_video_capture
#define GLX_NV_video_capture 1
GLAPI int GLAD_GLX_NV_video_capture;
typedef int (APIENTRYP PFNGLXBINDVIDEOCAPTUREDEVICENVPROC)(Display*, unsigned int, GLXVideoCaptureDeviceNV);
GLAPI PFNGLXBINDVIDEOCAPTUREDEVICENVPROC glad_glXBindVideoCaptureDeviceNV;
#define glXBindVideoCaptureDeviceNV glad_glXBindVideoCaptureDeviceNV
typedef GLXVideoCaptureDeviceNV* (APIENTRYP PFNGLXENUMERATEVIDEOCAPTUREDEVICESNVPROC)(Display*, int, int*);
GLAPI PFNGLXENUMERATEVIDEOCAPTUREDEVICESNVPROC glad_glXEnumerateVideoCaptureDevicesNV;
#define glXEnumerateVideoCaptureDevicesNV glad_glXEnumerateVideoCaptureDevicesNV
typedef void (APIENTRYP PFNGLXLOCKVIDEOCAPTUREDEVICENVPROC)(Display*, GLXVideoCaptureDeviceNV);
GLAPI PFNGLXLOCKVIDEOCAPTUREDEVICENVPROC glad_glXLockVideoCaptureDeviceNV;
#define glXLockVideoCaptureDeviceNV glad_glXLockVideoCaptureDeviceNV
typedef int (APIENTRYP PFNGLXQUERYVIDEOCAPTUREDEVICENVPROC)(Display*, GLXVideoCaptureDeviceNV, int, int*);
GLAPI PFNGLXQUERYVIDEOCAPTUREDEVICENVPROC glad_glXQueryVideoCaptureDeviceNV;
#define glXQueryVideoCaptureDeviceNV glad_glXQueryVideoCaptureDeviceNV
typedef void (APIENTRYP PFNGLXRELEASEVIDEOCAPTUREDEVICENVPROC)(Display*, GLXVideoCaptureDeviceNV);
GLAPI PFNGLXRELEASEVIDEOCAPTUREDEVICENVPROC glad_glXReleaseVideoCaptureDeviceNV;
#define glXReleaseVideoCaptureDeviceNV glad_glXReleaseVideoCaptureDeviceNV
#endif
#ifndef GLX_ARB_multisample
#define GLX_ARB_multisample 1
GLAPI int GLAD_GLX_ARB_multisample;
#endif
#ifndef GLX_NV_delay_before_swap
#define GLX_NV_delay_before_swap 1
GLAPI int GLAD_GLX_NV_delay_before_swap;
typedef Bool (APIENTRYP PFNGLXDELAYBEFORESWAPNVPROC)(Display*, GLXDrawable, GLfloat);
GLAPI PFNGLXDELAYBEFORESWAPNVPROC glad_glXDelayBeforeSwapNV;
#define glXDelayBeforeSwapNV glad_glXDelayBeforeSwapNV
#endif
#ifndef GLX_SGIX_swap_group
#define GLX_SGIX_swap_group 1
GLAPI int GLAD_GLX_SGIX_swap_group;
typedef void (APIENTRYP PFNGLXJOINSWAPGROUPSGIXPROC)(Display*, GLXDrawable, GLXDrawable);
GLAPI PFNGLXJOINSWAPGROUPSGIXPROC glad_glXJoinSwapGroupSGIX;
#define glXJoinSwapGroupSGIX glad_glXJoinSwapGroupSGIX
#endif
#ifndef GLX_EXT_swap_control
#define GLX_EXT_swap_control 1
GLAPI int GLAD_GLX_EXT_swap_control;
typedef void (APIENTRYP PFNGLXSWAPINTERVALEXTPROC)(Display*, GLXDrawable, int);
GLAPI PFNGLXSWAPINTERVALEXTPROC glad_glXSwapIntervalEXT;
#define glXSwapIntervalEXT glad_glXSwapIntervalEXT
#endif
#ifndef GLX_SGIX_video_source
#define GLX_SGIX_video_source 1
GLAPI int GLAD_GLX_SGIX_video_source;
#ifdef _VL_H_
typedef GLXVideoSourceSGIX (APIENTRYP PFNGLXCREATEGLXVIDEOSOURCESGIXPROC)(Display*, int, VLServer, VLPath, int, VLNode);
GLAPI PFNGLXCREATEGLXVIDEOSOURCESGIXPROC glad_glXCreateGLXVideoSourceSGIX;
#define glXCreateGLXVideoSourceSGIX glad_glXCreateGLXVideoSourceSGIX
typedef void (APIENTRYP PFNGLXDESTROYGLXVIDEOSOURCESGIXPROC)(Display*, GLXVideoSourceSGIX);
GLAPI PFNGLXDESTROYGLXVIDEOSOURCESGIXPROC glad_glXDestroyGLXVideoSourceSGIX;
#define glXDestroyGLXVideoSourceSGIX glad_glXDestroyGLXVideoSourceSGIX
#endif
#endif
#ifndef GLX_MESA_query_renderer
#define GLX_MESA_query_renderer 1
GLAPI int GLAD_GLX_MESA_query_renderer;
typedef Bool (APIENTRYP PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC)(int, unsigned int*);
GLAPI PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC glad_glXQueryCurrentRendererIntegerMESA;
#define glXQueryCurrentRendererIntegerMESA glad_glXQueryCurrentRendererIntegerMESA
typedef const char* (APIENTRYP PFNGLXQUERYCURRENTRENDERERSTRINGMESAPROC)(int);
GLAPI PFNGLXQUERYCURRENTRENDERERSTRINGMESAPROC glad_glXQueryCurrentRendererStringMESA;
#define glXQueryCurrentRendererStringMESA glad_glXQueryCurrentRendererStringMESA
typedef Bool (APIENTRYP PFNGLXQUERYRENDERERINTEGERMESAPROC)(Display*, int, int, int, unsigned int*);
GLAPI PFNGLXQUERYRENDERERINTEGERMESAPROC glad_glXQueryRendererIntegerMESA;
#define glXQueryRendererIntegerMESA glad_glXQueryRendererIntegerMESA
typedef const char* (APIENTRYP PFNGLXQUERYRENDERERSTRINGMESAPROC)(Display*, int, int, int);
GLAPI PFNGLXQUERYRENDERERSTRINGMESAPROC glad_glXQueryRendererStringMESA;
#define glXQueryRendererStringMESA glad_glXQueryRendererStringMESA
#endif
#ifndef GLX_ARB_create_context
#define GLX_ARB_create_context 1
GLAPI int GLAD_GLX_ARB_create_context;
typedef GLXContext (APIENTRYP PFNGLXCREATECONTEXTATTRIBSARBPROC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
GLAPI PFNGLXCREATECONTEXTATTRIBSARBPROC glad_glXCreateContextAttribsARB;
#define glXCreateContextAttribsARB glad_glXCreateContextAttribsARB
#endif
#ifndef GLX_EXT_create_context_es_profile
#define GLX_EXT_create_context_es_profile 1
GLAPI int GLAD_GLX_EXT_create_context_es_profile;
#endif
#ifndef GLX_SGIX_fbconfig
#define GLX_SGIX_fbconfig 1
GLAPI int GLAD_GLX_SGIX_fbconfig;
typedef int (APIENTRYP PFNGLXGETFBCONFIGATTRIBSGIXPROC)(Display*, GLXFBConfigSGIX, int, int*);
GLAPI PFNGLXGETFBCONFIGATTRIBSGIXPROC glad_glXGetFBConfigAttribSGIX;
#define glXGetFBConfigAttribSGIX glad_glXGetFBConfigAttribSGIX
typedef GLXFBConfigSGIX* (APIENTRYP PFNGLXCHOOSEFBCONFIGSGIXPROC)(Display*, int, int*, int*);
GLAPI PFNGLXCHOOSEFBCONFIGSGIXPROC glad_glXChooseFBConfigSGIX;
#define glXChooseFBConfigSGIX glad_glXChooseFBConfigSGIX
typedef GLXPixmap (APIENTRYP PFNGLXCREATEGLXPIXMAPWITHCONFIGSGIXPROC)(Display*, GLXFBConfigSGIX, Pixmap);
GLAPI PFNGLXCREATEGLXPIXMAPWITHCONFIGSGIXPROC glad_glXCreateGLXPixmapWithConfigSGIX;
#define glXCreateGLXPixmapWithConfigSGIX glad_glXCreateGLXPixmapWithConfigSGIX
typedef GLXContext (APIENTRYP PFNGLXCREATECONTEXTWITHCONFIGSGIXPROC)(Display*, GLXFBConfigSGIX, int, GLXContext, Bool);
GLAPI PFNGLXCREATECONTEXTWITHCONFIGSGIXPROC glad_glXCreateContextWithConfigSGIX;
#define glXCreateContextWithConfigSGIX glad_glXCreateContextWithConfigSGIX
typedef XVisualInfo* (APIENTRYP PFNGLXGETVISUALFROMFBCONFIGSGIXPROC)(Display*, GLXFBConfigSGIX);
GLAPI PFNGLXGETVISUALFROMFBCONFIGSGIXPROC glad_glXGetVisualFromFBConfigSGIX;
#define glXGetVisualFromFBConfigSGIX glad_glXGetVisualFromFBConfigSGIX
typedef GLXFBConfigSGIX (APIENTRYP PFNGLXGETFBCONFIGFROMVISUALSGIXPROC)(Display*, XVisualInfo*);
GLAPI PFNGLXGETFBCONFIGFROMVISUALSGIXPROC glad_glXGetFBConfigFromVisualSGIX;
#define glXGetFBConfigFromVisualSGIX glad_glXGetFBConfigFromVisualSGIX
#endif
#ifndef GLX_MESA_pixmap_colormap
#define GLX_MESA_pixmap_colormap 1
GLAPI int GLAD_GLX_MESA_pixmap_colormap;
typedef GLXPixmap (APIENTRYP PFNGLXCREATEGLXPIXMAPMESAPROC)(Display*, XVisualInfo*, Pixmap, Colormap);
GLAPI PFNGLXCREATEGLXPIXMAPMESAPROC glad_glXCreateGLXPixmapMESA;
#define glXCreateGLXPixmapMESA glad_glXCreateGLXPixmapMESA
#endif
#ifndef GLX_SGIX_visual_select_group
#define GLX_SGIX_visual_select_group 1
GLAPI int GLAD_GLX_SGIX_visual_select_group;
#endif
#ifndef GLX_NV_video_output
#define GLX_NV_video_output 1
GLAPI int GLAD_GLX_NV_video_output;
typedef int (APIENTRYP PFNGLXGETVIDEODEVICENVPROC)(Display*, int, int, GLXVideoDeviceNV*);
GLAPI PFNGLXGETVIDEODEVICENVPROC glad_glXGetVideoDeviceNV;
#define glXGetVideoDeviceNV glad_glXGetVideoDeviceNV
typedef int (APIENTRYP PFNGLXRELEASEVIDEODEVICENVPROC)(Display*, int, GLXVideoDeviceNV);
GLAPI PFNGLXRELEASEVIDEODEVICENVPROC glad_glXReleaseVideoDeviceNV;
#define glXReleaseVideoDeviceNV glad_glXReleaseVideoDeviceNV
typedef int (APIENTRYP PFNGLXBINDVIDEOIMAGENVPROC)(Display*, GLXVideoDeviceNV, GLXPbuffer, int);
GLAPI PFNGLXBINDVIDEOIMAGENVPROC glad_glXBindVideoImageNV;
#define glXBindVideoImageNV glad_glXBindVideoImageNV
typedef int (APIENTRYP PFNGLXRELEASEVIDEOIMAGENVPROC)(Display*, GLXPbuffer);
GLAPI PFNGLXRELEASEVIDEOIMAGENVPROC glad_glXReleaseVideoImageNV;
#define glXReleaseVideoImageNV glad_glXReleaseVideoImageNV
typedef int (APIENTRYP PFNGLXSENDPBUFFERTOVIDEONVPROC)(Display*, GLXPbuffer, int, unsigned long*, GLboolean);
GLAPI PFNGLXSENDPBUFFERTOVIDEONVPROC glad_glXSendPbufferToVideoNV;
#define glXSendPbufferToVideoNV glad_glXSendPbufferToVideoNV
typedef int (APIENTRYP PFNGLXGETVIDEOINFONVPROC)(Display*, int, GLXVideoDeviceNV, unsigned long*, unsigned long*);
GLAPI PFNGLXGETVIDEOINFONVPROC glad_glXGetVideoInfoNV;
#define glXGetVideoInfoNV glad_glXGetVideoInfoNV
#endif
#ifndef GLX_SGIS_blended_overlay
#define GLX_SGIS_blended_overlay 1
GLAPI int GLAD_GLX_SGIS_blended_overlay;
#endif
#ifndef GLX_SGIX_dmbuffer
#define GLX_SGIX_dmbuffer 1
GLAPI int GLAD_GLX_SGIX_dmbuffer;
#ifdef _DM_BUFFER_H_
typedef Bool (APIENTRYP PFNGLXASSOCIATEDMPBUFFERSGIXPROC)(Display*, GLXPbufferSGIX, DMparams*, DMbuffer);
GLAPI PFNGLXASSOCIATEDMPBUFFERSGIXPROC glad_glXAssociateDMPbufferSGIX;
#define glXAssociateDMPbufferSGIX glad_glXAssociateDMPbufferSGIX
#endif
#endif
#ifndef GLX_ARB_create_context_robustness
#define GLX_ARB_create_context_robustness 1
GLAPI int GLAD_GLX_ARB_create_context_robustness;
#endif
#ifndef GLX_SGIX_swap_barrier
#define GLX_SGIX_swap_barrier 1
GLAPI int GLAD_GLX_SGIX_swap_barrier;
typedef void (APIENTRYP PFNGLXBINDSWAPBARRIERSGIXPROC)(Display*, GLXDrawable, int);
GLAPI PFNGLXBINDSWAPBARRIERSGIXPROC glad_glXBindSwapBarrierSGIX;
#define glXBindSwapBarrierSGIX glad_glXBindSwapBarrierSGIX
typedef Bool (APIENTRYP PFNGLXQUERYMAXSWAPBARRIERSSGIXPROC)(Display*, int, int*);
GLAPI PFNGLXQUERYMAXSWAPBARRIERSSGIXPROC glad_glXQueryMaxSwapBarriersSGIX;
#define glXQueryMaxSwapBarriersSGIX glad_glXQueryMaxSwapBarriersSGIX
#endif
#ifndef GLX_EXT_swap_control_tear
#define GLX_EXT_swap_control_tear 1
GLAPI int GLAD_GLX_EXT_swap_control_tear;
#endif
#ifndef GLX_MESA_release_buffers
#define GLX_MESA_release_buffers 1
GLAPI int GLAD_GLX_MESA_release_buffers;
typedef Bool (APIENTRYP PFNGLXRELEASEBUFFERSMESAPROC)(Display*, GLXDrawable);
GLAPI PFNGLXRELEASEBUFFERSMESAPROC glad_glXReleaseBuffersMESA;
#define glXReleaseBuffersMESA glad_glXReleaseBuffersMESA
#endif
#ifndef GLX_EXT_visual_rating
#define GLX_EXT_visual_rating 1
GLAPI int GLAD_GLX_EXT_visual_rating;
#endif
#ifndef GLX_MESA_copy_sub_buffer
#define GLX_MESA_copy_sub_buffer 1
GLAPI int GLAD_GLX_MESA_copy_sub_buffer;
typedef void (APIENTRYP PFNGLXCOPYSUBBUFFERMESAPROC)(Display*, GLXDrawable, int, int, int, int);
GLAPI PFNGLXCOPYSUBBUFFERMESAPROC glad_glXCopySubBufferMESA;
#define glXCopySubBufferMESA glad_glXCopySubBufferMESA
#endif
#ifndef GLX_SGI_cushion
#define GLX_SGI_cushion 1
GLAPI int GLAD_GLX_SGI_cushion;
typedef void (APIENTRYP PFNGLXCUSHIONSGIPROC)(Display*, Window, float);
GLAPI PFNGLXCUSHIONSGIPROC glad_glXCushionSGI;
#define glXCushionSGI glad_glXCushionSGI
#endif
#ifndef GLX_NV_float_buffer
#define GLX_NV_float_buffer 1
GLAPI int GLAD_GLX_NV_float_buffer;
#endif
#ifndef GLX_OML_swap_method
#define GLX_OML_swap_method 1
GLAPI int GLAD_GLX_OML_swap_method;
#endif
#ifndef GLX_NV_present_video
#define GLX_NV_present_video 1
GLAPI int GLAD_GLX_NV_present_video;
typedef unsigned int* (APIENTRYP PFNGLXENUMERATEVIDEODEVICESNVPROC)(Display*, int, int*);
GLAPI PFNGLXENUMERATEVIDEODEVICESNVPROC glad_glXEnumerateVideoDevicesNV;
#define glXEnumerateVideoDevicesNV glad_glXEnumerateVideoDevicesNV
typedef int (APIENTRYP PFNGLXBINDVIDEODEVICENVPROC)(Display*, unsigned int, unsigned int, const int*);
GLAPI PFNGLXBINDVIDEODEVICENVPROC glad_glXBindVideoDeviceNV;
#define glXBindVideoDeviceNV glad_glXBindVideoDeviceNV
#endif
#ifndef GLX_SUN_get_transparent_index
#define GLX_SUN_get_transparent_index 1
GLAPI int GLAD_GLX_SUN_get_transparent_index;
typedef Status (APIENTRYP PFNGLXGETTRANSPARENTINDEXSUNPROC)(Display*, Window, Window, long*);
GLAPI PFNGLXGETTRANSPARENTINDEXSUNPROC glad_glXGetTransparentIndexSUN;
#define glXGetTransparentIndexSUN glad_glXGetTransparentIndexSUN
#endif
#ifndef GLX_AMD_gpu_association
#define GLX_AMD_gpu_association 1
GLAPI int GLAD_GLX_AMD_gpu_association;
#endif
#ifndef GLX_ARB_create_context_profile
#define GLX_ARB_create_context_profile 1
GLAPI int GLAD_GLX_ARB_create_context_profile;
#endif
#ifndef GLX_ARB_get_proc_address
#define GLX_ARB_get_proc_address 1
GLAPI int GLAD_GLX_ARB_get_proc_address;
typedef __GLXextFuncPtr (APIENTRYP PFNGLXGETPROCADDRESSARBPROC)(const GLubyte*);
GLAPI PFNGLXGETPROCADDRESSARBPROC glad_glXGetProcAddressARB;
#define glXGetProcAddressARB glad_glXGetProcAddressARB
#endif
#ifndef GLX_ARB_vertex_buffer_object
#define GLX_ARB_vertex_buffer_object 1
GLAPI int GLAD_GLX_ARB_vertex_buffer_object;
#endif

#ifdef __cplusplus
}
#endif

#endif
