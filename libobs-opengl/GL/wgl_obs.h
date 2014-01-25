#ifndef POINTER_C_GENERATED_HEADER_WINDOWSGL_H
#define POINTER_C_GENERATED_HEADER_WINDOWSGL_H

#ifdef __wglext_h_
#error Attempt to include auto-generated WGL header after wglext.h
#endif

#define __wglext_h_

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <windows.h>

#ifdef CODEGEN_FUNCPTR
#undef CODEGEN_FUNCPTR
#endif /*CODEGEN_FUNCPTR*/
#define CODEGEN_FUNCPTR WINAPI

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


struct _GPU_DEVICE {
    DWORD  cb;
    CHAR   DeviceName[32];
    CHAR   DeviceString[128];
    DWORD  Flags;
    RECT   rcVirtualScreen;
};
DECLARE_HANDLE(HPBUFFERARB);
DECLARE_HANDLE(HPBUFFEREXT);
DECLARE_HANDLE(HVIDEOOUTPUTDEVICENV);
DECLARE_HANDLE(HPVIDEODEV);
DECLARE_HANDLE(HGPUNV);
DECLARE_HANDLE(HVIDEOINPUTDEVICENV);
typedef struct _GPU_DEVICE *PGPU_DEVICE;

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

extern int wgl_ext_ARB_multisample;
extern int wgl_ext_ARB_extensions_string;
extern int wgl_ext_ARB_pixel_format;
extern int wgl_ext_ARB_pixel_format_float;
extern int wgl_ext_ARB_framebuffer_sRGB;
extern int wgl_ext_ARB_create_context;
extern int wgl_ext_ARB_create_context_profile;
extern int wgl_ext_ARB_create_context_robustness;
extern int wgl_ext_EXT_swap_control;
extern int wgl_ext_EXT_pixel_format_packed_float;
extern int wgl_ext_EXT_create_context_es2_profile;
extern int wgl_ext_EXT_swap_control_tear;
extern int wgl_ext_NV_swap_group;

#define WGL_SAMPLES_ARB 0x2042
#define WGL_SAMPLE_BUFFERS_ARB 0x2041

#define WGL_ACCELERATION_ARB 0x2003
#define WGL_ACCUM_ALPHA_BITS_ARB 0x2021
#define WGL_ACCUM_BITS_ARB 0x201D
#define WGL_ACCUM_BLUE_BITS_ARB 0x2020
#define WGL_ACCUM_GREEN_BITS_ARB 0x201F
#define WGL_ACCUM_RED_BITS_ARB 0x201E
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_ALPHA_SHIFT_ARB 0x201C
#define WGL_AUX_BUFFERS_ARB 0x2024
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_BLUE_SHIFT_ARB 0x201A
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_DRAW_TO_BITMAP_ARB 0x2002
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_GENERIC_ACCELERATION_ARB 0x2026
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_GREEN_SHIFT_ARB 0x2018
#define WGL_NEED_PALETTE_ARB 0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB 0x2005
#define WGL_NO_ACCELERATION_ARB 0x2025
#define WGL_NUMBER_OVERLAYS_ARB 0x2008
#define WGL_NUMBER_PIXEL_FORMATS_ARB 0x2000
#define WGL_NUMBER_UNDERLAYS_ARB 0x2009
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_RED_BITS_ARB 0x2015
#define WGL_RED_SHIFT_ARB 0x2016
#define WGL_SHARE_ACCUM_ARB 0x200E
#define WGL_SHARE_DEPTH_ARB 0x200C
#define WGL_SHARE_STENCIL_ARB 0x200D
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_STEREO_ARB 0x2012
#define WGL_SUPPORT_GDI_ARB 0x200F
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_SWAP_COPY_ARB 0x2029
#define WGL_SWAP_EXCHANGE_ARB 0x2028
#define WGL_SWAP_LAYER_BUFFERS_ARB 0x2006
#define WGL_SWAP_METHOD_ARB 0x2007
#define WGL_SWAP_UNDEFINED_ARB 0x202A
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_ARB 0x200A
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_TRANSPARENT_RED_VALUE_ARB 0x2037
#define WGL_TYPE_COLORINDEX_ARB 0x202C
#define WGL_TYPE_RGBA_ARB 0x202B

#define WGL_TYPE_RGBA_FLOAT_ARB 0x21A0

#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20A9

#define WGL_CONTEXT_DEBUG_BIT_ARB 0x00000001
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_LAYER_PLANE_ARB 0x2093
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_ERROR_INVALID_VERSION_ARB 0x2095

#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_ERROR_INVALID_PROFILE_ARB 0x2096

#define WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB 0x00000004
#define WGL_LOSE_CONTEXT_ON_RESET_ARB 0x8252
#define WGL_NO_RESET_NOTIFICATION_ARB 0x8261

#define WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT 0x20A8

#define WGL_CONTEXT_ES2_PROFILE_BIT_EXT 0x00000004


#ifndef WGL_ARB_extensions_string
#define WGL_ARB_extensions_string 1
extern const char * (CODEGEN_FUNCPTR *_ptrc_wglGetExtensionsStringARB)(HDC);
#define wglGetExtensionsStringARB _ptrc_wglGetExtensionsStringARB
#endif /*WGL_ARB_extensions_string*/ 

#ifndef WGL_ARB_pixel_format
#define WGL_ARB_pixel_format 1
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglChoosePixelFormatARB)(HDC, const int *, const FLOAT *, UINT, int *, UINT *);
#define wglChoosePixelFormatARB _ptrc_wglChoosePixelFormatARB
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglGetPixelFormatAttribfvARB)(HDC, int, int, UINT, const int *, FLOAT *);
#define wglGetPixelFormatAttribfvARB _ptrc_wglGetPixelFormatAttribfvARB
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglGetPixelFormatAttribivARB)(HDC, int, int, UINT, const int *, int *);
#define wglGetPixelFormatAttribivARB _ptrc_wglGetPixelFormatAttribivARB
#endif /*WGL_ARB_pixel_format*/ 



#ifndef WGL_ARB_create_context
#define WGL_ARB_create_context 1
extern HGLRC (CODEGEN_FUNCPTR *_ptrc_wglCreateContextAttribsARB)(HDC, HGLRC, const int *);
#define wglCreateContextAttribsARB _ptrc_wglCreateContextAttribsARB
#endif /*WGL_ARB_create_context*/ 



#ifndef WGL_EXT_swap_control
#define WGL_EXT_swap_control 1
extern int (CODEGEN_FUNCPTR *_ptrc_wglGetSwapIntervalEXT)();
#define wglGetSwapIntervalEXT _ptrc_wglGetSwapIntervalEXT
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglSwapIntervalEXT)(int);
#define wglSwapIntervalEXT _ptrc_wglSwapIntervalEXT
#endif /*WGL_EXT_swap_control*/ 




#ifndef WGL_NV_swap_group
#define WGL_NV_swap_group 1
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglBindSwapBarrierNV)(GLuint, GLuint);
#define wglBindSwapBarrierNV _ptrc_wglBindSwapBarrierNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglJoinSwapGroupNV)(HDC, GLuint);
#define wglJoinSwapGroupNV _ptrc_wglJoinSwapGroupNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglQueryFrameCountNV)(HDC, GLuint *);
#define wglQueryFrameCountNV _ptrc_wglQueryFrameCountNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglQueryMaxSwapGroupsNV)(HDC, GLuint *, GLuint *);
#define wglQueryMaxSwapGroupsNV _ptrc_wglQueryMaxSwapGroupsNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglQuerySwapGroupNV)(HDC, GLuint *, GLuint *);
#define wglQuerySwapGroupNV _ptrc_wglQuerySwapGroupNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglResetFrameCountNV)(HDC);
#define wglResetFrameCountNV _ptrc_wglResetFrameCountNV
#endif /*WGL_NV_swap_group*/ 

enum wgl_LoadStatus
{
	wgl_LOAD_FAILED = 0,
	wgl_LOAD_SUCCEEDED = 1,
};

int wgl_LoadFunctions(HDC hdc);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif //POINTER_C_GENERATED_HEADER_WINDOWSGL_H
