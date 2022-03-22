
#ifndef WINAPI
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN 1
# endif
# include <windows.h>
#endif

#include <glad/glad.h>

#ifndef __glad_wglext_h_

#ifdef __wglext_h_
#error WGL header already included, remove this include, glad already provides it
#endif

#define __glad_wglext_h_
#define __wglext_h_

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

GLAPI int gladLoadWGL(HDC hdc);

GLAPI void gladLoadWGLLoader(GLADloadproc, HDC hdc);

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
DECLARE_HANDLE(HPGPUNV);
DECLARE_HANDLE(HGPUNV);
DECLARE_HANDLE(HVIDEOINPUTDEVICENV);
typedef struct _GPU_DEVICE GPU_DEVICE;
typedef struct _GPU_DEVICE *PGPU_DEVICE;
#define WGL_COVERAGE_SAMPLES_NV 0x2042
#define WGL_COLOR_SAMPLES_NV 0x20B9
#define WGL_IMAGE_BUFFER_MIN_ACCESS_I3D 0x00000001
#define WGL_IMAGE_BUFFER_LOCK_I3D 0x00000002
#define WGL_FLOAT_COMPONENTS_NV 0x20B0
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV 0x20B1
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV 0x20B2
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV 0x20B3
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV 0x20B4
#define WGL_TEXTURE_FLOAT_R_NV 0x20B5
#define WGL_TEXTURE_FLOAT_RG_NV 0x20B6
#define WGL_TEXTURE_FLOAT_RGB_NV 0x20B7
#define WGL_TEXTURE_FLOAT_RGBA_NV 0x20B8
#define WGL_TYPE_RGBA_FLOAT_ARB 0x21A0
#define WGL_CONTEXT_DEBUG_BIT_ARB 0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB 0x2093
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define ERROR_INVALID_VERSION_ARB 0x2095
#define ERROR_INCOMPATIBLE_AFFINITY_MASKS_NV 0x20D0
#define ERROR_MISSING_AFFINITY_MASK_NV 0x20D1
#define WGL_NUMBER_PIXEL_FORMATS_EXT 0x2000
#define WGL_DRAW_TO_WINDOW_EXT 0x2001
#define WGL_DRAW_TO_BITMAP_EXT 0x2002
#define WGL_ACCELERATION_EXT 0x2003
#define WGL_NEED_PALETTE_EXT 0x2004
#define WGL_NEED_SYSTEM_PALETTE_EXT 0x2005
#define WGL_SWAP_LAYER_BUFFERS_EXT 0x2006
#define WGL_SWAP_METHOD_EXT 0x2007
#define WGL_NUMBER_OVERLAYS_EXT 0x2008
#define WGL_NUMBER_UNDERLAYS_EXT 0x2009
#define WGL_TRANSPARENT_EXT 0x200A
#define WGL_TRANSPARENT_VALUE_EXT 0x200B
#define WGL_SHARE_DEPTH_EXT 0x200C
#define WGL_SHARE_STENCIL_EXT 0x200D
#define WGL_SHARE_ACCUM_EXT 0x200E
#define WGL_SUPPORT_GDI_EXT 0x200F
#define WGL_SUPPORT_OPENGL_EXT 0x2010
#define WGL_DOUBLE_BUFFER_EXT 0x2011
#define WGL_STEREO_EXT 0x2012
#define WGL_PIXEL_TYPE_EXT 0x2013
#define WGL_COLOR_BITS_EXT 0x2014
#define WGL_RED_BITS_EXT 0x2015
#define WGL_RED_SHIFT_EXT 0x2016
#define WGL_GREEN_BITS_EXT 0x2017
#define WGL_GREEN_SHIFT_EXT 0x2018
#define WGL_BLUE_BITS_EXT 0x2019
#define WGL_BLUE_SHIFT_EXT 0x201A
#define WGL_ALPHA_BITS_EXT 0x201B
#define WGL_ALPHA_SHIFT_EXT 0x201C
#define WGL_ACCUM_BITS_EXT 0x201D
#define WGL_ACCUM_RED_BITS_EXT 0x201E
#define WGL_ACCUM_GREEN_BITS_EXT 0x201F
#define WGL_ACCUM_BLUE_BITS_EXT 0x2020
#define WGL_ACCUM_ALPHA_BITS_EXT 0x2021
#define WGL_DEPTH_BITS_EXT 0x2022
#define WGL_STENCIL_BITS_EXT 0x2023
#define WGL_AUX_BUFFERS_EXT 0x2024
#define WGL_NO_ACCELERATION_EXT 0x2025
#define WGL_GENERIC_ACCELERATION_EXT 0x2026
#define WGL_FULL_ACCELERATION_EXT 0x2027
#define WGL_SWAP_EXCHANGE_EXT 0x2028
#define WGL_SWAP_COPY_EXT 0x2029
#define WGL_SWAP_UNDEFINED_EXT 0x202A
#define WGL_TYPE_RGBA_EXT 0x202B
#define WGL_TYPE_COLORINDEX_EXT 0x202C
#define WGL_UNIQUE_ID_NV 0x20CE
#define WGL_NUM_VIDEO_CAPTURE_SLOTS_NV 0x20CF
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV 0x20A0
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV 0x20A1
#define WGL_TEXTURE_RECTANGLE_NV 0x20A2
#define WGL_CONTEXT_ES_PROFILE_BIT_EXT 0x00000004
#define WGL_CONTEXT_RESET_ISOLATION_BIT_ARB 0x00000008
#define WGL_BIND_TO_TEXTURE_RGB_ARB 0x2070
#define WGL_BIND_TO_TEXTURE_RGBA_ARB 0x2071
#define WGL_TEXTURE_FORMAT_ARB 0x2072
#define WGL_TEXTURE_TARGET_ARB 0x2073
#define WGL_MIPMAP_TEXTURE_ARB 0x2074
#define WGL_TEXTURE_RGB_ARB 0x2075
#define WGL_TEXTURE_RGBA_ARB 0x2076
#define WGL_NO_TEXTURE_ARB 0x2077
#define WGL_TEXTURE_CUBE_MAP_ARB 0x2078
#define WGL_TEXTURE_1D_ARB 0x2079
#define WGL_TEXTURE_2D_ARB 0x207A
#define WGL_MIPMAP_LEVEL_ARB 0x207B
#define WGL_CUBE_MAP_FACE_ARB 0x207C
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x207D
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x207E
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x207F
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x2080
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x2081
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x2082
#define WGL_FRONT_LEFT_ARB 0x2083
#define WGL_FRONT_RIGHT_ARB 0x2084
#define WGL_BACK_LEFT_ARB 0x2085
#define WGL_BACK_RIGHT_ARB 0x2086
#define WGL_AUX0_ARB 0x2087
#define WGL_AUX1_ARB 0x2088
#define WGL_AUX2_ARB 0x2089
#define WGL_AUX3_ARB 0x208A
#define WGL_AUX4_ARB 0x208B
#define WGL_AUX5_ARB 0x208C
#define WGL_AUX6_ARB 0x208D
#define WGL_AUX7_ARB 0x208E
#define WGL_AUX8_ARB 0x208F
#define WGL_AUX9_ARB 0x2090
#define WGL_DEPTH_FLOAT_EXT 0x2040
#define WGL_NUMBER_PIXEL_FORMATS_ARB 0x2000
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_DRAW_TO_BITMAP_ARB 0x2002
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_NEED_PALETTE_ARB 0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB 0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB 0x2006
#define WGL_SWAP_METHOD_ARB 0x2007
#define WGL_NUMBER_OVERLAYS_ARB 0x2008
#define WGL_NUMBER_UNDERLAYS_ARB 0x2009
#define WGL_TRANSPARENT_ARB 0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB 0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_SHARE_DEPTH_ARB 0x200C
#define WGL_SHARE_STENCIL_ARB 0x200D
#define WGL_SHARE_ACCUM_ARB 0x200E
#define WGL_SUPPORT_GDI_ARB 0x200F
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_STEREO_ARB 0x2012
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_RED_BITS_ARB 0x2015
#define WGL_RED_SHIFT_ARB 0x2016
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_GREEN_SHIFT_ARB 0x2018
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_BLUE_SHIFT_ARB 0x201A
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_ALPHA_SHIFT_ARB 0x201C
#define WGL_ACCUM_BITS_ARB 0x201D
#define WGL_ACCUM_RED_BITS_ARB 0x201E
#define WGL_ACCUM_GREEN_BITS_ARB 0x201F
#define WGL_ACCUM_BLUE_BITS_ARB 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB 0x2021
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_AUX_BUFFERS_ARB 0x2024
#define WGL_NO_ACCELERATION_ARB 0x2025
#define WGL_GENERIC_ACCELERATION_ARB 0x2026
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_SWAP_EXCHANGE_ARB 0x2028
#define WGL_SWAP_COPY_ARB 0x2029
#define WGL_SWAP_UNDEFINED_ARB 0x202A
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_TYPE_COLORINDEX_ARB 0x202C
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042
#define WGL_GENLOCK_SOURCE_MULTIVIEW_I3D 0x2044
#define WGL_GENLOCK_SOURCE_EXTERNAL_SYNC_I3D 0x2045
#define WGL_GENLOCK_SOURCE_EXTERNAL_FIELD_I3D 0x2046
#define WGL_GENLOCK_SOURCE_EXTERNAL_TTL_I3D 0x2047
#define WGL_GENLOCK_SOURCE_DIGITAL_SYNC_I3D 0x2048
#define WGL_GENLOCK_SOURCE_DIGITAL_FIELD_I3D 0x2049
#define WGL_GENLOCK_SOURCE_EDGE_FALLING_I3D 0x204A
#define WGL_GENLOCK_SOURCE_EDGE_RISING_I3D 0x204B
#define WGL_GENLOCK_SOURCE_EDGE_BOTH_I3D 0x204C
#define WGL_ACCESS_READ_ONLY_NV 0x00000000
#define WGL_ACCESS_READ_WRITE_NV 0x00000001
#define WGL_ACCESS_WRITE_DISCARD_NV 0x00000002
#define WGL_STEREO_EMITTER_ENABLE_3DL 0x2055
#define WGL_STEREO_EMITTER_DISABLE_3DL 0x2056
#define WGL_STEREO_POLARITY_NORMAL_3DL 0x2057
#define WGL_STEREO_POLARITY_INVERT_3DL 0x2058
#define WGL_DRAW_TO_PBUFFER_EXT 0x202D
#define WGL_MAX_PBUFFER_PIXELS_EXT 0x202E
#define WGL_MAX_PBUFFER_WIDTH_EXT 0x202F
#define WGL_MAX_PBUFFER_HEIGHT_EXT 0x2030
#define WGL_OPTIMAL_PBUFFER_WIDTH_EXT 0x2031
#define WGL_OPTIMAL_PBUFFER_HEIGHT_EXT 0x2032
#define WGL_PBUFFER_LARGEST_EXT 0x2033
#define WGL_PBUFFER_WIDTH_EXT 0x2034
#define WGL_PBUFFER_HEIGHT_EXT 0x2035
#define WGL_BIND_TO_VIDEO_RGB_NV 0x20C0
#define WGL_BIND_TO_VIDEO_RGBA_NV 0x20C1
#define WGL_BIND_TO_VIDEO_RGB_AND_DEPTH_NV 0x20C2
#define WGL_VIDEO_OUT_COLOR_NV 0x20C3
#define WGL_VIDEO_OUT_ALPHA_NV 0x20C4
#define WGL_VIDEO_OUT_DEPTH_NV 0x20C5
#define WGL_VIDEO_OUT_COLOR_AND_ALPHA_NV 0x20C6
#define WGL_VIDEO_OUT_COLOR_AND_DEPTH_NV 0x20C7
#define WGL_VIDEO_OUT_FRAME 0x20C8
#define WGL_VIDEO_OUT_FIELD_1 0x20C9
#define WGL_VIDEO_OUT_FIELD_2 0x20CA
#define WGL_VIDEO_OUT_STACKED_FIELDS_1_2 0x20CB
#define WGL_VIDEO_OUT_STACKED_FIELDS_2_1 0x20CC
#define WGL_SAMPLE_BUFFERS_3DFX 0x2060
#define WGL_SAMPLES_3DFX 0x2061
#define WGL_GAMMA_TABLE_SIZE_I3D 0x204E
#define WGL_GAMMA_EXCLUDE_DESKTOP_I3D 0x204F
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20A9
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT 0x20A9
#define WGL_NUM_VIDEO_SLOTS_NV 0x20F0
#define WGL_CONTEXT_ES2_PROFILE_BIT_EXT 0x00000004
#define WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB 0x00000004
#define WGL_LOSE_CONTEXT_ON_RESET_ARB 0x8252
#define WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define WGL_NO_RESET_NOTIFICATION_ARB 0x8261
#define ERROR_INVALID_PIXEL_TYPE_ARB 0x2043
#define ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB 0x2054
#define WGL_SAMPLE_BUFFERS_EXT 0x2041
#define WGL_SAMPLES_EXT 0x2042
#define WGL_BIND_TO_TEXTURE_DEPTH_NV 0x20A3
#define WGL_BIND_TO_TEXTURE_RECTANGLE_DEPTH_NV 0x20A4
#define WGL_DEPTH_TEXTURE_FORMAT_NV 0x20A5
#define WGL_TEXTURE_DEPTH_COMPONENT_NV 0x20A6
#define WGL_DEPTH_COMPONENT_NV 0x20A7
#define WGL_TYPE_RGBA_FLOAT_ATI 0x21A0
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define ERROR_INVALID_PROFILE_ARB 0x2096
#define WGL_DIGITAL_VIDEO_CURSOR_ALPHA_FRAMEBUFFER_I3D 0x2050
#define WGL_DIGITAL_VIDEO_CURSOR_ALPHA_VALUE_I3D 0x2051
#define WGL_DIGITAL_VIDEO_CURSOR_INCLUDED_I3D 0x2052
#define WGL_DIGITAL_VIDEO_GAMMA_CORRECTED_I3D 0x2053
#define WGL_DRAW_TO_PBUFFER_ARB 0x202D
#define WGL_MAX_PBUFFER_PIXELS_ARB 0x202E
#define WGL_MAX_PBUFFER_WIDTH_ARB 0x202F
#define WGL_MAX_PBUFFER_HEIGHT_ARB 0x2030
#define WGL_PBUFFER_LARGEST_ARB 0x2033
#define WGL_PBUFFER_WIDTH_ARB 0x2034
#define WGL_PBUFFER_HEIGHT_ARB 0x2035
#define WGL_PBUFFER_LOST_ARB 0x2036
#define WGL_GPU_VENDOR_AMD 0x1F00
#define WGL_GPU_RENDERER_STRING_AMD 0x1F01
#define WGL_GPU_OPENGL_VERSION_STRING_AMD 0x1F02
#define WGL_GPU_FASTEST_TARGET_GPUS_AMD 0x21A2
#define WGL_GPU_RAM_AMD 0x21A3
#define WGL_GPU_CLOCK_AMD 0x21A4
#define WGL_GPU_NUM_PIPES_AMD 0x21A5
#define WGL_GPU_NUM_SIMD_AMD 0x21A6
#define WGL_GPU_NUM_RB_AMD 0x21A7
#define WGL_GPU_NUM_SPI_AMD 0x21A8
#define WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT 0x20A8
#define ERROR_INVALID_PIXEL_TYPE_EXT 0x2043
#define WGL_FRONT_COLOR_BUFFER_BIT_ARB 0x00000001
#define WGL_BACK_COLOR_BUFFER_BIT_ARB 0x00000002
#define WGL_DEPTH_BUFFER_BIT_ARB 0x00000004
#define WGL_STENCIL_BUFFER_BIT_ARB 0x00000008
#ifndef WGL_NV_multisample_coverage
#define WGL_NV_multisample_coverage 1
GLAPI int GLAD_WGL_NV_multisample_coverage;
#endif
#ifndef WGL_I3D_image_buffer
#define WGL_I3D_image_buffer 1
GLAPI int GLAD_WGL_I3D_image_buffer;
typedef LPVOID (APIENTRYP PFNWGLCREATEIMAGEBUFFERI3DPROC)(HDC, DWORD, UINT);
GLAPI PFNWGLCREATEIMAGEBUFFERI3DPROC glad_wglCreateImageBufferI3D;
#define wglCreateImageBufferI3D glad_wglCreateImageBufferI3D
typedef BOOL (APIENTRYP PFNWGLDESTROYIMAGEBUFFERI3DPROC)(HDC, LPVOID);
GLAPI PFNWGLDESTROYIMAGEBUFFERI3DPROC glad_wglDestroyImageBufferI3D;
#define wglDestroyImageBufferI3D glad_wglDestroyImageBufferI3D
typedef BOOL (APIENTRYP PFNWGLASSOCIATEIMAGEBUFFEREVENTSI3DPROC)(HDC, const HANDLE*, const LPVOID*, const DWORD*, UINT);
GLAPI PFNWGLASSOCIATEIMAGEBUFFEREVENTSI3DPROC glad_wglAssociateImageBufferEventsI3D;
#define wglAssociateImageBufferEventsI3D glad_wglAssociateImageBufferEventsI3D
typedef BOOL (APIENTRYP PFNWGLRELEASEIMAGEBUFFEREVENTSI3DPROC)(HDC, const LPVOID*, UINT);
GLAPI PFNWGLRELEASEIMAGEBUFFEREVENTSI3DPROC glad_wglReleaseImageBufferEventsI3D;
#define wglReleaseImageBufferEventsI3D glad_wglReleaseImageBufferEventsI3D
#endif
#ifndef WGL_I3D_swap_frame_usage
#define WGL_I3D_swap_frame_usage 1
GLAPI int GLAD_WGL_I3D_swap_frame_usage;
typedef BOOL (APIENTRYP PFNWGLGETFRAMEUSAGEI3DPROC)(float*);
GLAPI PFNWGLGETFRAMEUSAGEI3DPROC glad_wglGetFrameUsageI3D;
#define wglGetFrameUsageI3D glad_wglGetFrameUsageI3D
typedef BOOL (APIENTRYP PFNWGLBEGINFRAMETRACKINGI3DPROC)();
GLAPI PFNWGLBEGINFRAMETRACKINGI3DPROC glad_wglBeginFrameTrackingI3D;
#define wglBeginFrameTrackingI3D glad_wglBeginFrameTrackingI3D
typedef BOOL (APIENTRYP PFNWGLENDFRAMETRACKINGI3DPROC)();
GLAPI PFNWGLENDFRAMETRACKINGI3DPROC glad_wglEndFrameTrackingI3D;
#define wglEndFrameTrackingI3D glad_wglEndFrameTrackingI3D
typedef BOOL (APIENTRYP PFNWGLQUERYFRAMETRACKINGI3DPROC)(DWORD*, DWORD*, float*);
GLAPI PFNWGLQUERYFRAMETRACKINGI3DPROC glad_wglQueryFrameTrackingI3D;
#define wglQueryFrameTrackingI3D glad_wglQueryFrameTrackingI3D
#endif
#ifndef WGL_NV_DX_interop2
#define WGL_NV_DX_interop2 1
GLAPI int GLAD_WGL_NV_DX_interop2;
#endif
#ifndef WGL_NV_float_buffer
#define WGL_NV_float_buffer 1
GLAPI int GLAD_WGL_NV_float_buffer;
#endif
#ifndef WGL_NV_delay_before_swap
#define WGL_NV_delay_before_swap 1
GLAPI int GLAD_WGL_NV_delay_before_swap;
typedef BOOL (APIENTRYP PFNWGLDELAYBEFORESWAPNVPROC)(HDC, GLfloat);
GLAPI PFNWGLDELAYBEFORESWAPNVPROC glad_wglDelayBeforeSwapNV;
#define wglDelayBeforeSwapNV glad_wglDelayBeforeSwapNV
#endif
#ifndef WGL_OML_sync_control
#define WGL_OML_sync_control 1
GLAPI int GLAD_WGL_OML_sync_control;
typedef BOOL (APIENTRYP PFNWGLGETSYNCVALUESOMLPROC)(HDC, INT64*, INT64*, INT64*);
GLAPI PFNWGLGETSYNCVALUESOMLPROC glad_wglGetSyncValuesOML;
#define wglGetSyncValuesOML glad_wglGetSyncValuesOML
typedef BOOL (APIENTRYP PFNWGLGETMSCRATEOMLPROC)(HDC, INT32*, INT32*);
GLAPI PFNWGLGETMSCRATEOMLPROC glad_wglGetMscRateOML;
#define wglGetMscRateOML glad_wglGetMscRateOML
typedef INT64 (APIENTRYP PFNWGLSWAPBUFFERSMSCOMLPROC)(HDC, INT64, INT64, INT64);
GLAPI PFNWGLSWAPBUFFERSMSCOMLPROC glad_wglSwapBuffersMscOML;
#define wglSwapBuffersMscOML glad_wglSwapBuffersMscOML
typedef INT64 (APIENTRYP PFNWGLSWAPLAYERBUFFERSMSCOMLPROC)(HDC, int, INT64, INT64, INT64);
GLAPI PFNWGLSWAPLAYERBUFFERSMSCOMLPROC glad_wglSwapLayerBuffersMscOML;
#define wglSwapLayerBuffersMscOML glad_wglSwapLayerBuffersMscOML
typedef BOOL (APIENTRYP PFNWGLWAITFORMSCOMLPROC)(HDC, INT64, INT64, INT64, INT64*, INT64*, INT64*);
GLAPI PFNWGLWAITFORMSCOMLPROC glad_wglWaitForMscOML;
#define wglWaitForMscOML glad_wglWaitForMscOML
typedef BOOL (APIENTRYP PFNWGLWAITFORSBCOMLPROC)(HDC, INT64, INT64*, INT64*, INT64*);
GLAPI PFNWGLWAITFORSBCOMLPROC glad_wglWaitForSbcOML;
#define wglWaitForSbcOML glad_wglWaitForSbcOML
#endif
#ifndef WGL_ARB_pixel_format_float
#define WGL_ARB_pixel_format_float 1
GLAPI int GLAD_WGL_ARB_pixel_format_float;
#endif
#ifndef WGL_ARB_create_context
#define WGL_ARB_create_context 1
GLAPI int GLAD_WGL_ARB_create_context;
typedef HGLRC (APIENTRYP PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
GLAPI PFNWGLCREATECONTEXTATTRIBSARBPROC glad_wglCreateContextAttribsARB;
#define wglCreateContextAttribsARB glad_wglCreateContextAttribsARB
#endif
#ifndef WGL_NV_swap_group
#define WGL_NV_swap_group 1
GLAPI int GLAD_WGL_NV_swap_group;
typedef BOOL (APIENTRYP PFNWGLJOINSWAPGROUPNVPROC)(HDC, GLuint);
GLAPI PFNWGLJOINSWAPGROUPNVPROC glad_wglJoinSwapGroupNV;
#define wglJoinSwapGroupNV glad_wglJoinSwapGroupNV
typedef BOOL (APIENTRYP PFNWGLBINDSWAPBARRIERNVPROC)(GLuint, GLuint);
GLAPI PFNWGLBINDSWAPBARRIERNVPROC glad_wglBindSwapBarrierNV;
#define wglBindSwapBarrierNV glad_wglBindSwapBarrierNV
typedef BOOL (APIENTRYP PFNWGLQUERYSWAPGROUPNVPROC)(HDC, GLuint*, GLuint*);
GLAPI PFNWGLQUERYSWAPGROUPNVPROC glad_wglQuerySwapGroupNV;
#define wglQuerySwapGroupNV glad_wglQuerySwapGroupNV
typedef BOOL (APIENTRYP PFNWGLQUERYMAXSWAPGROUPSNVPROC)(HDC, GLuint*, GLuint*);
GLAPI PFNWGLQUERYMAXSWAPGROUPSNVPROC glad_wglQueryMaxSwapGroupsNV;
#define wglQueryMaxSwapGroupsNV glad_wglQueryMaxSwapGroupsNV
typedef BOOL (APIENTRYP PFNWGLQUERYFRAMECOUNTNVPROC)(HDC, GLuint*);
GLAPI PFNWGLQUERYFRAMECOUNTNVPROC glad_wglQueryFrameCountNV;
#define wglQueryFrameCountNV glad_wglQueryFrameCountNV
typedef BOOL (APIENTRYP PFNWGLRESETFRAMECOUNTNVPROC)(HDC);
GLAPI PFNWGLRESETFRAMECOUNTNVPROC glad_wglResetFrameCountNV;
#define wglResetFrameCountNV glad_wglResetFrameCountNV
#endif
#ifndef WGL_NV_gpu_affinity
#define WGL_NV_gpu_affinity 1
GLAPI int GLAD_WGL_NV_gpu_affinity;
typedef BOOL (APIENTRYP PFNWGLENUMGPUSNVPROC)(UINT, HGPUNV*);
GLAPI PFNWGLENUMGPUSNVPROC glad_wglEnumGpusNV;
#define wglEnumGpusNV glad_wglEnumGpusNV
typedef BOOL (APIENTRYP PFNWGLENUMGPUDEVICESNVPROC)(HGPUNV, UINT, PGPU_DEVICE);
GLAPI PFNWGLENUMGPUDEVICESNVPROC glad_wglEnumGpuDevicesNV;
#define wglEnumGpuDevicesNV glad_wglEnumGpuDevicesNV
typedef HDC (APIENTRYP PFNWGLCREATEAFFINITYDCNVPROC)(const HGPUNV*);
GLAPI PFNWGLCREATEAFFINITYDCNVPROC glad_wglCreateAffinityDCNV;
#define wglCreateAffinityDCNV glad_wglCreateAffinityDCNV
typedef BOOL (APIENTRYP PFNWGLENUMGPUSFROMAFFINITYDCNVPROC)(HDC, UINT, HGPUNV*);
GLAPI PFNWGLENUMGPUSFROMAFFINITYDCNVPROC glad_wglEnumGpusFromAffinityDCNV;
#define wglEnumGpusFromAffinityDCNV glad_wglEnumGpusFromAffinityDCNV
typedef BOOL (APIENTRYP PFNWGLDELETEDCNVPROC)(HDC);
GLAPI PFNWGLDELETEDCNVPROC glad_wglDeleteDCNV;
#define wglDeleteDCNV glad_wglDeleteDCNV
#endif
#ifndef WGL_EXT_pixel_format
#define WGL_EXT_pixel_format 1
GLAPI int GLAD_WGL_EXT_pixel_format;
typedef BOOL (APIENTRYP PFNWGLGETPIXELFORMATATTRIBIVEXTPROC)(HDC, int, int, UINT, int*, int*);
GLAPI PFNWGLGETPIXELFORMATATTRIBIVEXTPROC glad_wglGetPixelFormatAttribivEXT;
#define wglGetPixelFormatAttribivEXT glad_wglGetPixelFormatAttribivEXT
typedef BOOL (APIENTRYP PFNWGLGETPIXELFORMATATTRIBFVEXTPROC)(HDC, int, int, UINT, int*, FLOAT*);
GLAPI PFNWGLGETPIXELFORMATATTRIBFVEXTPROC glad_wglGetPixelFormatAttribfvEXT;
#define wglGetPixelFormatAttribfvEXT glad_wglGetPixelFormatAttribfvEXT
typedef BOOL (APIENTRYP PFNWGLCHOOSEPIXELFORMATEXTPROC)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
GLAPI PFNWGLCHOOSEPIXELFORMATEXTPROC glad_wglChoosePixelFormatEXT;
#define wglChoosePixelFormatEXT glad_wglChoosePixelFormatEXT
#endif
#ifndef WGL_ARB_extensions_string
#define WGL_ARB_extensions_string 1
GLAPI int GLAD_WGL_ARB_extensions_string;
typedef const char* (APIENTRYP PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC);
GLAPI PFNWGLGETEXTENSIONSSTRINGARBPROC glad_wglGetExtensionsStringARB;
#define wglGetExtensionsStringARB glad_wglGetExtensionsStringARB
#endif
#ifndef WGL_NV_video_capture
#define WGL_NV_video_capture 1
GLAPI int GLAD_WGL_NV_video_capture;
typedef BOOL (APIENTRYP PFNWGLBINDVIDEOCAPTUREDEVICENVPROC)(UINT, HVIDEOINPUTDEVICENV);
GLAPI PFNWGLBINDVIDEOCAPTUREDEVICENVPROC glad_wglBindVideoCaptureDeviceNV;
#define wglBindVideoCaptureDeviceNV glad_wglBindVideoCaptureDeviceNV
typedef UINT (APIENTRYP PFNWGLENUMERATEVIDEOCAPTUREDEVICESNVPROC)(HDC, HVIDEOINPUTDEVICENV*);
GLAPI PFNWGLENUMERATEVIDEOCAPTUREDEVICESNVPROC glad_wglEnumerateVideoCaptureDevicesNV;
#define wglEnumerateVideoCaptureDevicesNV glad_wglEnumerateVideoCaptureDevicesNV
typedef BOOL (APIENTRYP PFNWGLLOCKVIDEOCAPTUREDEVICENVPROC)(HDC, HVIDEOINPUTDEVICENV);
GLAPI PFNWGLLOCKVIDEOCAPTUREDEVICENVPROC glad_wglLockVideoCaptureDeviceNV;
#define wglLockVideoCaptureDeviceNV glad_wglLockVideoCaptureDeviceNV
typedef BOOL (APIENTRYP PFNWGLQUERYVIDEOCAPTUREDEVICENVPROC)(HDC, HVIDEOINPUTDEVICENV, int, int*);
GLAPI PFNWGLQUERYVIDEOCAPTUREDEVICENVPROC glad_wglQueryVideoCaptureDeviceNV;
#define wglQueryVideoCaptureDeviceNV glad_wglQueryVideoCaptureDeviceNV
typedef BOOL (APIENTRYP PFNWGLRELEASEVIDEOCAPTUREDEVICENVPROC)(HDC, HVIDEOINPUTDEVICENV);
GLAPI PFNWGLRELEASEVIDEOCAPTUREDEVICENVPROC glad_wglReleaseVideoCaptureDeviceNV;
#define wglReleaseVideoCaptureDeviceNV glad_wglReleaseVideoCaptureDeviceNV
#endif
#ifndef WGL_NV_render_texture_rectangle
#define WGL_NV_render_texture_rectangle 1
GLAPI int GLAD_WGL_NV_render_texture_rectangle;
#endif
#ifndef WGL_EXT_create_context_es_profile
#define WGL_EXT_create_context_es_profile 1
GLAPI int GLAD_WGL_EXT_create_context_es_profile;
#endif
#ifndef WGL_ARB_robustness_share_group_isolation
#define WGL_ARB_robustness_share_group_isolation 1
GLAPI int GLAD_WGL_ARB_robustness_share_group_isolation;
#endif
#ifndef WGL_ARB_render_texture
#define WGL_ARB_render_texture 1
GLAPI int GLAD_WGL_ARB_render_texture;
typedef BOOL (APIENTRYP PFNWGLBINDTEXIMAGEARBPROC)(HPBUFFERARB, int);
GLAPI PFNWGLBINDTEXIMAGEARBPROC glad_wglBindTexImageARB;
#define wglBindTexImageARB glad_wglBindTexImageARB
typedef BOOL (APIENTRYP PFNWGLRELEASETEXIMAGEARBPROC)(HPBUFFERARB, int);
GLAPI PFNWGLRELEASETEXIMAGEARBPROC glad_wglReleaseTexImageARB;
#define wglReleaseTexImageARB glad_wglReleaseTexImageARB
typedef BOOL (APIENTRYP PFNWGLSETPBUFFERATTRIBARBPROC)(HPBUFFERARB, const int*);
GLAPI PFNWGLSETPBUFFERATTRIBARBPROC glad_wglSetPbufferAttribARB;
#define wglSetPbufferAttribARB glad_wglSetPbufferAttribARB
#endif
#ifndef WGL_EXT_depth_float
#define WGL_EXT_depth_float 1
GLAPI int GLAD_WGL_EXT_depth_float;
#endif
#ifndef WGL_EXT_swap_control_tear
#define WGL_EXT_swap_control_tear 1
GLAPI int GLAD_WGL_EXT_swap_control_tear;
#endif
#ifndef WGL_ARB_pixel_format
#define WGL_ARB_pixel_format 1
GLAPI int GLAD_WGL_ARB_pixel_format;
typedef BOOL (APIENTRYP PFNWGLGETPIXELFORMATATTRIBIVARBPROC)(HDC, int, int, UINT, const int*, int*);
GLAPI PFNWGLGETPIXELFORMATATTRIBIVARBPROC glad_wglGetPixelFormatAttribivARB;
#define wglGetPixelFormatAttribivARB glad_wglGetPixelFormatAttribivARB
typedef BOOL (APIENTRYP PFNWGLGETPIXELFORMATATTRIBFVARBPROC)(HDC, int, int, UINT, const int*, FLOAT*);
GLAPI PFNWGLGETPIXELFORMATATTRIBFVARBPROC glad_wglGetPixelFormatAttribfvARB;
#define wglGetPixelFormatAttribfvARB glad_wglGetPixelFormatAttribfvARB
typedef BOOL (APIENTRYP PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
GLAPI PFNWGLCHOOSEPIXELFORMATARBPROC glad_wglChoosePixelFormatARB;
#define wglChoosePixelFormatARB glad_wglChoosePixelFormatARB
#endif
#ifndef WGL_ARB_multisample
#define WGL_ARB_multisample 1
GLAPI int GLAD_WGL_ARB_multisample;
#endif
#ifndef WGL_I3D_genlock
#define WGL_I3D_genlock 1
GLAPI int GLAD_WGL_I3D_genlock;
typedef BOOL (APIENTRYP PFNWGLENABLEGENLOCKI3DPROC)(HDC);
GLAPI PFNWGLENABLEGENLOCKI3DPROC glad_wglEnableGenlockI3D;
#define wglEnableGenlockI3D glad_wglEnableGenlockI3D
typedef BOOL (APIENTRYP PFNWGLDISABLEGENLOCKI3DPROC)(HDC);
GLAPI PFNWGLDISABLEGENLOCKI3DPROC glad_wglDisableGenlockI3D;
#define wglDisableGenlockI3D glad_wglDisableGenlockI3D
typedef BOOL (APIENTRYP PFNWGLISENABLEDGENLOCKI3DPROC)(HDC, BOOL*);
GLAPI PFNWGLISENABLEDGENLOCKI3DPROC glad_wglIsEnabledGenlockI3D;
#define wglIsEnabledGenlockI3D glad_wglIsEnabledGenlockI3D
typedef BOOL (APIENTRYP PFNWGLGENLOCKSOURCEI3DPROC)(HDC, UINT);
GLAPI PFNWGLGENLOCKSOURCEI3DPROC glad_wglGenlockSourceI3D;
#define wglGenlockSourceI3D glad_wglGenlockSourceI3D
typedef BOOL (APIENTRYP PFNWGLGETGENLOCKSOURCEI3DPROC)(HDC, UINT*);
GLAPI PFNWGLGETGENLOCKSOURCEI3DPROC glad_wglGetGenlockSourceI3D;
#define wglGetGenlockSourceI3D glad_wglGetGenlockSourceI3D
typedef BOOL (APIENTRYP PFNWGLGENLOCKSOURCEEDGEI3DPROC)(HDC, UINT);
GLAPI PFNWGLGENLOCKSOURCEEDGEI3DPROC glad_wglGenlockSourceEdgeI3D;
#define wglGenlockSourceEdgeI3D glad_wglGenlockSourceEdgeI3D
typedef BOOL (APIENTRYP PFNWGLGETGENLOCKSOURCEEDGEI3DPROC)(HDC, UINT*);
GLAPI PFNWGLGETGENLOCKSOURCEEDGEI3DPROC glad_wglGetGenlockSourceEdgeI3D;
#define wglGetGenlockSourceEdgeI3D glad_wglGetGenlockSourceEdgeI3D
typedef BOOL (APIENTRYP PFNWGLGENLOCKSAMPLERATEI3DPROC)(HDC, UINT);
GLAPI PFNWGLGENLOCKSAMPLERATEI3DPROC glad_wglGenlockSampleRateI3D;
#define wglGenlockSampleRateI3D glad_wglGenlockSampleRateI3D
typedef BOOL (APIENTRYP PFNWGLGETGENLOCKSAMPLERATEI3DPROC)(HDC, UINT*);
GLAPI PFNWGLGETGENLOCKSAMPLERATEI3DPROC glad_wglGetGenlockSampleRateI3D;
#define wglGetGenlockSampleRateI3D glad_wglGetGenlockSampleRateI3D
typedef BOOL (APIENTRYP PFNWGLGENLOCKSOURCEDELAYI3DPROC)(HDC, UINT);
GLAPI PFNWGLGENLOCKSOURCEDELAYI3DPROC glad_wglGenlockSourceDelayI3D;
#define wglGenlockSourceDelayI3D glad_wglGenlockSourceDelayI3D
typedef BOOL (APIENTRYP PFNWGLGETGENLOCKSOURCEDELAYI3DPROC)(HDC, UINT*);
GLAPI PFNWGLGETGENLOCKSOURCEDELAYI3DPROC glad_wglGetGenlockSourceDelayI3D;
#define wglGetGenlockSourceDelayI3D glad_wglGetGenlockSourceDelayI3D
typedef BOOL (APIENTRYP PFNWGLQUERYGENLOCKMAXSOURCEDELAYI3DPROC)(HDC, UINT*, UINT*);
GLAPI PFNWGLQUERYGENLOCKMAXSOURCEDELAYI3DPROC glad_wglQueryGenlockMaxSourceDelayI3D;
#define wglQueryGenlockMaxSourceDelayI3D glad_wglQueryGenlockMaxSourceDelayI3D
#endif
#ifndef WGL_NV_DX_interop
#define WGL_NV_DX_interop 1
GLAPI int GLAD_WGL_NV_DX_interop;
typedef BOOL (APIENTRYP PFNWGLDXSETRESOURCESHAREHANDLENVPROC)(void*, HANDLE);
GLAPI PFNWGLDXSETRESOURCESHAREHANDLENVPROC glad_wglDXSetResourceShareHandleNV;
#define wglDXSetResourceShareHandleNV glad_wglDXSetResourceShareHandleNV
typedef HANDLE (APIENTRYP PFNWGLDXOPENDEVICENVPROC)(void*);
GLAPI PFNWGLDXOPENDEVICENVPROC glad_wglDXOpenDeviceNV;
#define wglDXOpenDeviceNV glad_wglDXOpenDeviceNV
typedef BOOL (APIENTRYP PFNWGLDXCLOSEDEVICENVPROC)(HANDLE);
GLAPI PFNWGLDXCLOSEDEVICENVPROC glad_wglDXCloseDeviceNV;
#define wglDXCloseDeviceNV glad_wglDXCloseDeviceNV
typedef HANDLE (APIENTRYP PFNWGLDXREGISTEROBJECTNVPROC)(HANDLE, void*, GLuint, GLenum, GLenum);
GLAPI PFNWGLDXREGISTEROBJECTNVPROC glad_wglDXRegisterObjectNV;
#define wglDXRegisterObjectNV glad_wglDXRegisterObjectNV
typedef BOOL (APIENTRYP PFNWGLDXUNREGISTEROBJECTNVPROC)(HANDLE, HANDLE);
GLAPI PFNWGLDXUNREGISTEROBJECTNVPROC glad_wglDXUnregisterObjectNV;
#define wglDXUnregisterObjectNV glad_wglDXUnregisterObjectNV
typedef BOOL (APIENTRYP PFNWGLDXOBJECTACCESSNVPROC)(HANDLE, GLenum);
GLAPI PFNWGLDXOBJECTACCESSNVPROC glad_wglDXObjectAccessNV;
#define wglDXObjectAccessNV glad_wglDXObjectAccessNV
typedef BOOL (APIENTRYP PFNWGLDXLOCKOBJECTSNVPROC)(HANDLE, GLint, HANDLE*);
GLAPI PFNWGLDXLOCKOBJECTSNVPROC glad_wglDXLockObjectsNV;
#define wglDXLockObjectsNV glad_wglDXLockObjectsNV
typedef BOOL (APIENTRYP PFNWGLDXUNLOCKOBJECTSNVPROC)(HANDLE, GLint, HANDLE*);
GLAPI PFNWGLDXUNLOCKOBJECTSNVPROC glad_wglDXUnlockObjectsNV;
#define wglDXUnlockObjectsNV glad_wglDXUnlockObjectsNV
#endif
#ifndef WGL_3DL_stereo_control
#define WGL_3DL_stereo_control 1
GLAPI int GLAD_WGL_3DL_stereo_control;
typedef BOOL (APIENTRYP PFNWGLSETSTEREOEMITTERSTATE3DLPROC)(HDC, UINT);
GLAPI PFNWGLSETSTEREOEMITTERSTATE3DLPROC glad_wglSetStereoEmitterState3DL;
#define wglSetStereoEmitterState3DL glad_wglSetStereoEmitterState3DL
#endif
#ifndef WGL_EXT_pbuffer
#define WGL_EXT_pbuffer 1
GLAPI int GLAD_WGL_EXT_pbuffer;
typedef HPBUFFEREXT (APIENTRYP PFNWGLCREATEPBUFFEREXTPROC)(HDC, int, int, int, const int*);
GLAPI PFNWGLCREATEPBUFFEREXTPROC glad_wglCreatePbufferEXT;
#define wglCreatePbufferEXT glad_wglCreatePbufferEXT
typedef HDC (APIENTRYP PFNWGLGETPBUFFERDCEXTPROC)(HPBUFFEREXT);
GLAPI PFNWGLGETPBUFFERDCEXTPROC glad_wglGetPbufferDCEXT;
#define wglGetPbufferDCEXT glad_wglGetPbufferDCEXT
typedef int (APIENTRYP PFNWGLRELEASEPBUFFERDCEXTPROC)(HPBUFFEREXT, HDC);
GLAPI PFNWGLRELEASEPBUFFERDCEXTPROC glad_wglReleasePbufferDCEXT;
#define wglReleasePbufferDCEXT glad_wglReleasePbufferDCEXT
typedef BOOL (APIENTRYP PFNWGLDESTROYPBUFFEREXTPROC)(HPBUFFEREXT);
GLAPI PFNWGLDESTROYPBUFFEREXTPROC glad_wglDestroyPbufferEXT;
#define wglDestroyPbufferEXT glad_wglDestroyPbufferEXT
typedef BOOL (APIENTRYP PFNWGLQUERYPBUFFEREXTPROC)(HPBUFFEREXT, int, int*);
GLAPI PFNWGLQUERYPBUFFEREXTPROC glad_wglQueryPbufferEXT;
#define wglQueryPbufferEXT glad_wglQueryPbufferEXT
#endif
#ifndef WGL_EXT_display_color_table
#define WGL_EXT_display_color_table 1
GLAPI int GLAD_WGL_EXT_display_color_table;
typedef GLboolean (APIENTRYP PFNWGLCREATEDISPLAYCOLORTABLEEXTPROC)(GLushort);
GLAPI PFNWGLCREATEDISPLAYCOLORTABLEEXTPROC glad_wglCreateDisplayColorTableEXT;
#define wglCreateDisplayColorTableEXT glad_wglCreateDisplayColorTableEXT
typedef GLboolean (APIENTRYP PFNWGLLOADDISPLAYCOLORTABLEEXTPROC)(const GLushort*, GLuint);
GLAPI PFNWGLLOADDISPLAYCOLORTABLEEXTPROC glad_wglLoadDisplayColorTableEXT;
#define wglLoadDisplayColorTableEXT glad_wglLoadDisplayColorTableEXT
typedef GLboolean (APIENTRYP PFNWGLBINDDISPLAYCOLORTABLEEXTPROC)(GLushort);
GLAPI PFNWGLBINDDISPLAYCOLORTABLEEXTPROC glad_wglBindDisplayColorTableEXT;
#define wglBindDisplayColorTableEXT glad_wglBindDisplayColorTableEXT
typedef VOID (APIENTRYP PFNWGLDESTROYDISPLAYCOLORTABLEEXTPROC)(GLushort);
GLAPI PFNWGLDESTROYDISPLAYCOLORTABLEEXTPROC glad_wglDestroyDisplayColorTableEXT;
#define wglDestroyDisplayColorTableEXT glad_wglDestroyDisplayColorTableEXT
#endif
#ifndef WGL_NV_video_output
#define WGL_NV_video_output 1
GLAPI int GLAD_WGL_NV_video_output;
typedef BOOL (APIENTRYP PFNWGLGETVIDEODEVICENVPROC)(HDC, int, HPVIDEODEV*);
GLAPI PFNWGLGETVIDEODEVICENVPROC glad_wglGetVideoDeviceNV;
#define wglGetVideoDeviceNV glad_wglGetVideoDeviceNV
typedef BOOL (APIENTRYP PFNWGLRELEASEVIDEODEVICENVPROC)(HPVIDEODEV);
GLAPI PFNWGLRELEASEVIDEODEVICENVPROC glad_wglReleaseVideoDeviceNV;
#define wglReleaseVideoDeviceNV glad_wglReleaseVideoDeviceNV
typedef BOOL (APIENTRYP PFNWGLBINDVIDEOIMAGENVPROC)(HPVIDEODEV, HPBUFFERARB, int);
GLAPI PFNWGLBINDVIDEOIMAGENVPROC glad_wglBindVideoImageNV;
#define wglBindVideoImageNV glad_wglBindVideoImageNV
typedef BOOL (APIENTRYP PFNWGLRELEASEVIDEOIMAGENVPROC)(HPBUFFERARB, int);
GLAPI PFNWGLRELEASEVIDEOIMAGENVPROC glad_wglReleaseVideoImageNV;
#define wglReleaseVideoImageNV glad_wglReleaseVideoImageNV
typedef BOOL (APIENTRYP PFNWGLSENDPBUFFERTOVIDEONVPROC)(HPBUFFERARB, int, unsigned long*, BOOL);
GLAPI PFNWGLSENDPBUFFERTOVIDEONVPROC glad_wglSendPbufferToVideoNV;
#define wglSendPbufferToVideoNV glad_wglSendPbufferToVideoNV
typedef BOOL (APIENTRYP PFNWGLGETVIDEOINFONVPROC)(HPVIDEODEV, unsigned long*, unsigned long*);
GLAPI PFNWGLGETVIDEOINFONVPROC glad_wglGetVideoInfoNV;
#define wglGetVideoInfoNV glad_wglGetVideoInfoNV
#endif
#ifndef WGL_ARB_robustness_application_isolation
#define WGL_ARB_robustness_application_isolation 1
GLAPI int GLAD_WGL_ARB_robustness_application_isolation;
#endif
#ifndef WGL_3DFX_multisample
#define WGL_3DFX_multisample 1
GLAPI int GLAD_WGL_3DFX_multisample;
#endif
#ifndef WGL_I3D_gamma
#define WGL_I3D_gamma 1
GLAPI int GLAD_WGL_I3D_gamma;
typedef BOOL (APIENTRYP PFNWGLGETGAMMATABLEPARAMETERSI3DPROC)(HDC, int, int*);
GLAPI PFNWGLGETGAMMATABLEPARAMETERSI3DPROC glad_wglGetGammaTableParametersI3D;
#define wglGetGammaTableParametersI3D glad_wglGetGammaTableParametersI3D
typedef BOOL (APIENTRYP PFNWGLSETGAMMATABLEPARAMETERSI3DPROC)(HDC, int, const int*);
GLAPI PFNWGLSETGAMMATABLEPARAMETERSI3DPROC glad_wglSetGammaTableParametersI3D;
#define wglSetGammaTableParametersI3D glad_wglSetGammaTableParametersI3D
typedef BOOL (APIENTRYP PFNWGLGETGAMMATABLEI3DPROC)(HDC, int, USHORT*, USHORT*, USHORT*);
GLAPI PFNWGLGETGAMMATABLEI3DPROC glad_wglGetGammaTableI3D;
#define wglGetGammaTableI3D glad_wglGetGammaTableI3D
typedef BOOL (APIENTRYP PFNWGLSETGAMMATABLEI3DPROC)(HDC, int, const USHORT*, const USHORT*, const USHORT*);
GLAPI PFNWGLSETGAMMATABLEI3DPROC glad_wglSetGammaTableI3D;
#define wglSetGammaTableI3D glad_wglSetGammaTableI3D
#endif
#ifndef WGL_ARB_framebuffer_sRGB
#define WGL_ARB_framebuffer_sRGB 1
GLAPI int GLAD_WGL_ARB_framebuffer_sRGB;
#endif
#ifndef WGL_NV_copy_image
#define WGL_NV_copy_image 1
GLAPI int GLAD_WGL_NV_copy_image;
typedef BOOL (APIENTRYP PFNWGLCOPYIMAGESUBDATANVPROC)(HGLRC, GLuint, GLenum, GLint, GLint, GLint, GLint, HGLRC, GLuint, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei);
GLAPI PFNWGLCOPYIMAGESUBDATANVPROC glad_wglCopyImageSubDataNV;
#define wglCopyImageSubDataNV glad_wglCopyImageSubDataNV
#endif
#ifndef WGL_EXT_framebuffer_sRGB
#define WGL_EXT_framebuffer_sRGB 1
GLAPI int GLAD_WGL_EXT_framebuffer_sRGB;
#endif
#ifndef WGL_NV_present_video
#define WGL_NV_present_video 1
GLAPI int GLAD_WGL_NV_present_video;
typedef int (APIENTRYP PFNWGLENUMERATEVIDEODEVICESNVPROC)(HDC, HVIDEOOUTPUTDEVICENV*);
GLAPI PFNWGLENUMERATEVIDEODEVICESNVPROC glad_wglEnumerateVideoDevicesNV;
#define wglEnumerateVideoDevicesNV glad_wglEnumerateVideoDevicesNV
typedef BOOL (APIENTRYP PFNWGLBINDVIDEODEVICENVPROC)(HDC, unsigned int, HVIDEOOUTPUTDEVICENV, const int*);
GLAPI PFNWGLBINDVIDEODEVICENVPROC glad_wglBindVideoDeviceNV;
#define wglBindVideoDeviceNV glad_wglBindVideoDeviceNV
typedef BOOL (APIENTRYP PFNWGLQUERYCURRENTCONTEXTNVPROC)(int, int*);
GLAPI PFNWGLQUERYCURRENTCONTEXTNVPROC glad_wglQueryCurrentContextNV;
#define wglQueryCurrentContextNV glad_wglQueryCurrentContextNV
#endif
#ifndef WGL_EXT_create_context_es2_profile
#define WGL_EXT_create_context_es2_profile 1
GLAPI int GLAD_WGL_EXT_create_context_es2_profile;
#endif
#ifndef WGL_ARB_create_context_robustness
#define WGL_ARB_create_context_robustness 1
GLAPI int GLAD_WGL_ARB_create_context_robustness;
#endif
#ifndef WGL_ARB_make_current_read
#define WGL_ARB_make_current_read 1
GLAPI int GLAD_WGL_ARB_make_current_read;
typedef BOOL (APIENTRYP PFNWGLMAKECONTEXTCURRENTARBPROC)(HDC, HDC, HGLRC);
GLAPI PFNWGLMAKECONTEXTCURRENTARBPROC glad_wglMakeContextCurrentARB;
#define wglMakeContextCurrentARB glad_wglMakeContextCurrentARB
typedef HDC (APIENTRYP PFNWGLGETCURRENTREADDCARBPROC)();
GLAPI PFNWGLGETCURRENTREADDCARBPROC glad_wglGetCurrentReadDCARB;
#define wglGetCurrentReadDCARB glad_wglGetCurrentReadDCARB
#endif
#ifndef WGL_EXT_multisample
#define WGL_EXT_multisample 1
GLAPI int GLAD_WGL_EXT_multisample;
#endif
#ifndef WGL_EXT_extensions_string
#define WGL_EXT_extensions_string 1
GLAPI int GLAD_WGL_EXT_extensions_string;
typedef const char* (APIENTRYP PFNWGLGETEXTENSIONSSTRINGEXTPROC)();
GLAPI PFNWGLGETEXTENSIONSSTRINGEXTPROC glad_wglGetExtensionsStringEXT;
#define wglGetExtensionsStringEXT glad_wglGetExtensionsStringEXT
#endif
#ifndef WGL_NV_render_depth_texture
#define WGL_NV_render_depth_texture 1
GLAPI int GLAD_WGL_NV_render_depth_texture;
#endif
#ifndef WGL_ATI_pixel_format_float
#define WGL_ATI_pixel_format_float 1
GLAPI int GLAD_WGL_ATI_pixel_format_float;
#endif
#ifndef WGL_ARB_create_context_profile
#define WGL_ARB_create_context_profile 1
GLAPI int GLAD_WGL_ARB_create_context_profile;
#endif
#ifndef WGL_EXT_swap_control
#define WGL_EXT_swap_control 1
GLAPI int GLAD_WGL_EXT_swap_control;
typedef BOOL (APIENTRYP PFNWGLSWAPINTERVALEXTPROC)(int);
GLAPI PFNWGLSWAPINTERVALEXTPROC glad_wglSwapIntervalEXT;
#define wglSwapIntervalEXT glad_wglSwapIntervalEXT
typedef int (APIENTRYP PFNWGLGETSWAPINTERVALEXTPROC)();
GLAPI PFNWGLGETSWAPINTERVALEXTPROC glad_wglGetSwapIntervalEXT;
#define wglGetSwapIntervalEXT glad_wglGetSwapIntervalEXT
#endif
#ifndef WGL_I3D_digital_video_control
#define WGL_I3D_digital_video_control 1
GLAPI int GLAD_WGL_I3D_digital_video_control;
typedef BOOL (APIENTRYP PFNWGLGETDIGITALVIDEOPARAMETERSI3DPROC)(HDC, int, int*);
GLAPI PFNWGLGETDIGITALVIDEOPARAMETERSI3DPROC glad_wglGetDigitalVideoParametersI3D;
#define wglGetDigitalVideoParametersI3D glad_wglGetDigitalVideoParametersI3D
typedef BOOL (APIENTRYP PFNWGLSETDIGITALVIDEOPARAMETERSI3DPROC)(HDC, int, const int*);
GLAPI PFNWGLSETDIGITALVIDEOPARAMETERSI3DPROC glad_wglSetDigitalVideoParametersI3D;
#define wglSetDigitalVideoParametersI3D glad_wglSetDigitalVideoParametersI3D
#endif
#ifndef WGL_ARB_pbuffer
#define WGL_ARB_pbuffer 1
GLAPI int GLAD_WGL_ARB_pbuffer;
typedef HPBUFFERARB (APIENTRYP PFNWGLCREATEPBUFFERARBPROC)(HDC, int, int, int, const int*);
GLAPI PFNWGLCREATEPBUFFERARBPROC glad_wglCreatePbufferARB;
#define wglCreatePbufferARB glad_wglCreatePbufferARB
typedef HDC (APIENTRYP PFNWGLGETPBUFFERDCARBPROC)(HPBUFFERARB);
GLAPI PFNWGLGETPBUFFERDCARBPROC glad_wglGetPbufferDCARB;
#define wglGetPbufferDCARB glad_wglGetPbufferDCARB
typedef int (APIENTRYP PFNWGLRELEASEPBUFFERDCARBPROC)(HPBUFFERARB, HDC);
GLAPI PFNWGLRELEASEPBUFFERDCARBPROC glad_wglReleasePbufferDCARB;
#define wglReleasePbufferDCARB glad_wglReleasePbufferDCARB
typedef BOOL (APIENTRYP PFNWGLDESTROYPBUFFERARBPROC)(HPBUFFERARB);
GLAPI PFNWGLDESTROYPBUFFERARBPROC glad_wglDestroyPbufferARB;
#define wglDestroyPbufferARB glad_wglDestroyPbufferARB
typedef BOOL (APIENTRYP PFNWGLQUERYPBUFFERARBPROC)(HPBUFFERARB, int, int*);
GLAPI PFNWGLQUERYPBUFFERARBPROC glad_wglQueryPbufferARB;
#define wglQueryPbufferARB glad_wglQueryPbufferARB
#endif
#ifndef WGL_NV_vertex_array_range
#define WGL_NV_vertex_array_range 1
GLAPI int GLAD_WGL_NV_vertex_array_range;
typedef void* (APIENTRYP PFNWGLALLOCATEMEMORYNVPROC)(GLsizei, GLfloat, GLfloat, GLfloat);
GLAPI PFNWGLALLOCATEMEMORYNVPROC glad_wglAllocateMemoryNV;
#define wglAllocateMemoryNV glad_wglAllocateMemoryNV
typedef void (APIENTRYP PFNWGLFREEMEMORYNVPROC)(void*);
GLAPI PFNWGLFREEMEMORYNVPROC glad_wglFreeMemoryNV;
#define wglFreeMemoryNV glad_wglFreeMemoryNV
#endif
#ifndef WGL_AMD_gpu_association
#define WGL_AMD_gpu_association 1
GLAPI int GLAD_WGL_AMD_gpu_association;
typedef UINT (APIENTRYP PFNWGLGETGPUIDSAMDPROC)(UINT, UINT*);
GLAPI PFNWGLGETGPUIDSAMDPROC glad_wglGetGPUIDsAMD;
#define wglGetGPUIDsAMD glad_wglGetGPUIDsAMD
typedef INT (APIENTRYP PFNWGLGETGPUINFOAMDPROC)(UINT, int, GLenum, UINT, void*);
GLAPI PFNWGLGETGPUINFOAMDPROC glad_wglGetGPUInfoAMD;
#define wglGetGPUInfoAMD glad_wglGetGPUInfoAMD
typedef UINT (APIENTRYP PFNWGLGETCONTEXTGPUIDAMDPROC)(HGLRC);
GLAPI PFNWGLGETCONTEXTGPUIDAMDPROC glad_wglGetContextGPUIDAMD;
#define wglGetContextGPUIDAMD glad_wglGetContextGPUIDAMD
typedef HGLRC (APIENTRYP PFNWGLCREATEASSOCIATEDCONTEXTAMDPROC)(UINT);
GLAPI PFNWGLCREATEASSOCIATEDCONTEXTAMDPROC glad_wglCreateAssociatedContextAMD;
#define wglCreateAssociatedContextAMD glad_wglCreateAssociatedContextAMD
typedef HGLRC (APIENTRYP PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMDPROC)(UINT, HGLRC, const int*);
GLAPI PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMDPROC glad_wglCreateAssociatedContextAttribsAMD;
#define wglCreateAssociatedContextAttribsAMD glad_wglCreateAssociatedContextAttribsAMD
typedef BOOL (APIENTRYP PFNWGLDELETEASSOCIATEDCONTEXTAMDPROC)(HGLRC);
GLAPI PFNWGLDELETEASSOCIATEDCONTEXTAMDPROC glad_wglDeleteAssociatedContextAMD;
#define wglDeleteAssociatedContextAMD glad_wglDeleteAssociatedContextAMD
typedef BOOL (APIENTRYP PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMDPROC)(HGLRC);
GLAPI PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMDPROC glad_wglMakeAssociatedContextCurrentAMD;
#define wglMakeAssociatedContextCurrentAMD glad_wglMakeAssociatedContextCurrentAMD
typedef HGLRC (APIENTRYP PFNWGLGETCURRENTASSOCIATEDCONTEXTAMDPROC)();
GLAPI PFNWGLGETCURRENTASSOCIATEDCONTEXTAMDPROC glad_wglGetCurrentAssociatedContextAMD;
#define wglGetCurrentAssociatedContextAMD glad_wglGetCurrentAssociatedContextAMD
typedef VOID (APIENTRYP PFNWGLBLITCONTEXTFRAMEBUFFERAMDPROC)(HGLRC, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
GLAPI PFNWGLBLITCONTEXTFRAMEBUFFERAMDPROC glad_wglBlitContextFramebufferAMD;
#define wglBlitContextFramebufferAMD glad_wglBlitContextFramebufferAMD
#endif
#ifndef WGL_EXT_pixel_format_packed_float
#define WGL_EXT_pixel_format_packed_float 1
GLAPI int GLAD_WGL_EXT_pixel_format_packed_float;
#endif
#ifndef WGL_EXT_make_current_read
#define WGL_EXT_make_current_read 1
GLAPI int GLAD_WGL_EXT_make_current_read;
typedef BOOL (APIENTRYP PFNWGLMAKECONTEXTCURRENTEXTPROC)(HDC, HDC, HGLRC);
GLAPI PFNWGLMAKECONTEXTCURRENTEXTPROC glad_wglMakeContextCurrentEXT;
#define wglMakeContextCurrentEXT glad_wglMakeContextCurrentEXT
typedef HDC (APIENTRYP PFNWGLGETCURRENTREADDCEXTPROC)();
GLAPI PFNWGLGETCURRENTREADDCEXTPROC glad_wglGetCurrentReadDCEXT;
#define wglGetCurrentReadDCEXT glad_wglGetCurrentReadDCEXT
#endif
#ifndef WGL_I3D_swap_frame_lock
#define WGL_I3D_swap_frame_lock 1
GLAPI int GLAD_WGL_I3D_swap_frame_lock;
typedef BOOL (APIENTRYP PFNWGLENABLEFRAMELOCKI3DPROC)();
GLAPI PFNWGLENABLEFRAMELOCKI3DPROC glad_wglEnableFrameLockI3D;
#define wglEnableFrameLockI3D glad_wglEnableFrameLockI3D
typedef BOOL (APIENTRYP PFNWGLDISABLEFRAMELOCKI3DPROC)();
GLAPI PFNWGLDISABLEFRAMELOCKI3DPROC glad_wglDisableFrameLockI3D;
#define wglDisableFrameLockI3D glad_wglDisableFrameLockI3D
typedef BOOL (APIENTRYP PFNWGLISENABLEDFRAMELOCKI3DPROC)(BOOL*);
GLAPI PFNWGLISENABLEDFRAMELOCKI3DPROC glad_wglIsEnabledFrameLockI3D;
#define wglIsEnabledFrameLockI3D glad_wglIsEnabledFrameLockI3D
typedef BOOL (APIENTRYP PFNWGLQUERYFRAMELOCKMASTERI3DPROC)(BOOL*);
GLAPI PFNWGLQUERYFRAMELOCKMASTERI3DPROC glad_wglQueryFrameLockMasterI3D;
#define wglQueryFrameLockMasterI3D glad_wglQueryFrameLockMasterI3D
#endif
#ifndef WGL_ARB_buffer_region
#define WGL_ARB_buffer_region 1
GLAPI int GLAD_WGL_ARB_buffer_region;
typedef HANDLE (APIENTRYP PFNWGLCREATEBUFFERREGIONARBPROC)(HDC, int, UINT);
GLAPI PFNWGLCREATEBUFFERREGIONARBPROC glad_wglCreateBufferRegionARB;
#define wglCreateBufferRegionARB glad_wglCreateBufferRegionARB
typedef VOID (APIENTRYP PFNWGLDELETEBUFFERREGIONARBPROC)(HANDLE);
GLAPI PFNWGLDELETEBUFFERREGIONARBPROC glad_wglDeleteBufferRegionARB;
#define wglDeleteBufferRegionARB glad_wglDeleteBufferRegionARB
typedef BOOL (APIENTRYP PFNWGLSAVEBUFFERREGIONARBPROC)(HANDLE, int, int, int, int);
GLAPI PFNWGLSAVEBUFFERREGIONARBPROC glad_wglSaveBufferRegionARB;
#define wglSaveBufferRegionARB glad_wglSaveBufferRegionARB
typedef BOOL (APIENTRYP PFNWGLRESTOREBUFFERREGIONARBPROC)(HANDLE, int, int, int, int, int, int);
GLAPI PFNWGLRESTOREBUFFERREGIONARBPROC glad_wglRestoreBufferRegionARB;
#define wglRestoreBufferRegionARB glad_wglRestoreBufferRegionARB
#endif

#ifdef __cplusplus
}
#endif

#endif
