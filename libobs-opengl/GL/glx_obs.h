#ifndef POINTER_C_GENERATED_HEADER_GLXWIN_H
#define POINTER_C_GENERATED_HEADER_GLXWIN_H

#ifdef __glxext_h_
#error Attempt to include glx_exts after including glxext.h
#endif

#define __glxext_h_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#ifdef CODEGEN_FUNCPTR
#undef CODEGEN_FUNCPTR
#endif /*CODEGEN_FUNCPTR*/
#define CODEGEN_FUNCPTR

#ifndef GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS
#define GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
#define GLvoid void

#endif /*GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS*/


#ifndef GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS
#define GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS


#endif /*GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS*/


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
	typedef struct __GLXFBConfigRec *GLXFBConfig;
	typedef XID GLXContextID;
	typedef struct __GLXcontextRec *GLXContext;
	typedef XID GLXPixmap;
	typedef XID GLXDrawable;
	typedef XID GLXPbuffer;
	typedef void (APIENTRY *__GLXextFuncPtr)(void);
	typedef XID GLXVideoCaptureDeviceNV;
	typedef unsigned int GLXVideoDeviceNV;
	typedef XID GLXVideoSourceSGIX;
	typedef struct __GLXFBConfigRec *GLXFBConfigSGIX;
	typedef XID GLXPbufferSGIX;
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

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

extern int glx_ext_ARB_create_context;
extern int glx_ext_ARB_create_context_profile;
extern int glx_ext_ARB_create_context_robustness;
extern int glx_ext_ARB_fbconfig_float;
extern int glx_ext_ARB_framebuffer_sRGB;
extern int glx_ext_ARB_multisample;
extern int glx_ext_EXT_create_context_es2_profile;
extern int glx_ext_EXT_fbconfig_packed_float;
extern int glx_ext_EXT_framebuffer_sRGB;
extern int glx_ext_EXT_import_context;
extern int glx_ext_EXT_swap_control;
extern int glx_ext_EXT_swap_control_tear;

#define GLX_CONTEXT_DEBUG_BIT_ARB 0x00000001
#define GLX_CONTEXT_FLAGS_ARB 0x2094
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

#define GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define GLX_CONTEXT_PROFILE_MASK_ARB 0x9126

#define GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB 0x00000004
#define GLX_LOSE_CONTEXT_ON_RESET_ARB 0x8252
#define GLX_NO_RESET_NOTIFICATION_ARB 0x8261

#define GLX_RGBA_FLOAT_BIT_ARB 0x00000004
#define GLX_RGBA_FLOAT_TYPE_ARB 0x20B9

#define GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20B2

#define GLX_SAMPLES_ARB 100001
#define GLX_SAMPLE_BUFFERS_ARB 100000

#define GLX_CONTEXT_ES2_PROFILE_BIT_EXT 0x00000004

#define GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT 0x00000008
#define GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT 0x20B1

#define GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT 0x20B2

#define GLX_SCREEN_EXT 0x800C
#define GLX_SHARE_CONTEXT_EXT 0x800A
#define GLX_VISUAL_ID_EXT 0x800B

#define GLX_MAX_SWAP_INTERVAL_EXT 0x20F2
#define GLX_SWAP_INTERVAL_EXT 0x20F1

#define GLX_LATE_SWAPS_TEAR_EXT 0x20F3

#ifndef GLX_ARB_create_context
#define GLX_ARB_create_context 1
extern GLXContext (CODEGEN_FUNCPTR *_ptrc_glXCreateContextAttribsARB)(Display *, GLXFBConfig, GLXContext, Bool, const int *);
#define glXCreateContextAttribsARB _ptrc_glXCreateContextAttribsARB
#endif /*GLX_ARB_create_context*/ 









#ifndef GLX_EXT_import_context
#define GLX_EXT_import_context 1
extern void (CODEGEN_FUNCPTR *_ptrc_glXFreeContextEXT)(Display *, GLXContext);
#define glXFreeContextEXT _ptrc_glXFreeContextEXT
extern GLXContextID (CODEGEN_FUNCPTR *_ptrc_glXGetContextIDEXT)(const GLXContext);
#define glXGetContextIDEXT _ptrc_glXGetContextIDEXT
extern Display * (CODEGEN_FUNCPTR *_ptrc_glXGetCurrentDisplayEXT)();
#define glXGetCurrentDisplayEXT _ptrc_glXGetCurrentDisplayEXT
extern GLXContext (CODEGEN_FUNCPTR *_ptrc_glXImportContextEXT)(Display *, GLXContextID);
#define glXImportContextEXT _ptrc_glXImportContextEXT
extern int (CODEGEN_FUNCPTR *_ptrc_glXQueryContextInfoEXT)(Display *, GLXContext, int, int *);
#define glXQueryContextInfoEXT _ptrc_glXQueryContextInfoEXT
#endif /*GLX_EXT_import_context*/ 

#ifndef GLX_EXT_swap_control
#define GLX_EXT_swap_control 1
extern void (CODEGEN_FUNCPTR *_ptrc_glXSwapIntervalEXT)(Display *, GLXDrawable, int);
#define glXSwapIntervalEXT _ptrc_glXSwapIntervalEXT
#endif /*GLX_EXT_swap_control*/ 


enum glx_LoadStatus
{
	glx_LOAD_FAILED = 0,
	glx_LOAD_SUCCEEDED = 1,
};

int glx_LoadFunctions(Display *display, int screen);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif //POINTER_C_GENERATED_HEADER_GLXWIN_H
