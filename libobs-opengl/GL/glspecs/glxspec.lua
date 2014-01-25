return {
	["funcData"] = {
		["passthru"] = {
			[==[#ifndef GLEXT_64_TYPES_DEFINED
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
#endif]==],
			[==[typedef struct __GLXFBConfigRec *GLXFBConfig;]==],
			[==[typedef XID GLXContextID;]==],
			[==[typedef struct __GLXcontextRec *GLXContext;]==],
			[==[typedef XID GLXPixmap;]==],
			[==[typedef XID GLXDrawable;]==],
			[==[typedef XID GLXPbuffer;]==],
			[==[typedef void (APIENTRY *__GLXextFuncPtr)(void);]==],
			[==[typedef XID GLXVideoCaptureDeviceNV;]==],
			[==[typedef unsigned int GLXVideoDeviceNV;]==],
			[==[typedef XID GLXVideoSourceSGIX;]==],
			[==[typedef struct __GLXFBConfigRec *GLXFBConfigSGIX;]==],
			[==[typedef XID GLXPbufferSGIX;]==],
			[==[typedef struct {
    char    pipeName[80]; /* Should be [GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX] */
    int     networkId;
} GLXHyperpipeNetworkSGIX;]==],
			[==[typedef struct {
    char    pipeName[80]; /* Should be [GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX] */
    int     channel;
    unsigned int participationType;
    int     timeSlice;
} GLXHyperpipeConfigSGIX;]==],
			[==[typedef struct {
    char pipeName[80]; /* Should be [GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX] */
    int srcXOrigin, srcYOrigin, srcWidth, srcHeight;
    int destXOrigin, destYOrigin, destWidth, destHeight;
} GLXPipeRect;]==],
			[==[typedef struct {
    char pipeName[80]; /* Should be [GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX] */
    int XOrigin, YOrigin, maxHeight, maxWidth;
} GLXPipeRectLimits;]==],
		},
		["functions"] = {
			{
				["extensions"] = {
					[==[OML_sync_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[target_sbc]==],
						["ctype"] = [==[int64_t]==],
					},
					{
						["name"] = [==[ust]==],
						["ctype"] = [==[int64_t *]==],
					},
					{
						["name"] = [==[msc]==],
						["ctype"] = [==[int64_t *]==],
					},
					{
						["name"] = [==[sbc]==],
						["ctype"] = [==[int64_t *]==],
					},
				},
				["name"] = [==[WaitForSbcOML]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[OML_sync_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[ust]==],
						["ctype"] = [==[int64_t *]==],
					},
					{
						["name"] = [==[msc]==],
						["ctype"] = [==[int64_t *]==],
					},
					{
						["name"] = [==[sbc]==],
						["ctype"] = [==[int64_t *]==],
					},
				},
				["name"] = [==[GetSyncValuesOML]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[EXT_import_context]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[context]==],
						["ctype"] = [==[GLXContext]==],
					},
				},
				["name"] = [==[FreeContextEXT]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[EXT_texture_from_pixmap]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[buffer]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[ReleaseTexImageEXT]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[SGIX_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[pbuf]==],
						["ctype"] = [==[GLXPbufferSGIX]==],
					},
					{
						["name"] = [==[attribute]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[value]==],
						["ctype"] = [==[unsigned int *]==],
					},
				},
				["name"] = [==[QueryGLXPbufferSGIX]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[EXT_texture_from_pixmap]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[buffer]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[attrib_list]==],
						["ctype"] = [==[const int *]==],
					},
				},
				["name"] = [==[BindTexImageEXT]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[ARB_get_proc_address]==],
				},
				["parameters"] = {
					{
						["name"] = [==[procName]==],
						["ctype"] = [==[const GLubyte *]==],
					},
				},
				["name"] = [==[GetProcAddressARB]==],
				["return_ctype"] = [==[__GLXextFuncPtr]==],
			},
			{
				["extensions"] = {
					[==[SGIX_hyperpipe]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[hpId]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[npipes]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[QueryHyperpipeConfigSGIX]==],
				["return_ctype"] = [==[GLXHyperpipeConfigSGIX *]==],
			},
			{
				["extensions"] = {
					[==[NV_swap_group]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[group]==],
						["ctype"] = [==[GLuint *]==],
					},
					{
						["name"] = [==[barrier]==],
						["ctype"] = [==[GLuint *]==],
					},
				},
				["name"] = [==[QuerySwapGroupNV]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[SGIX_hyperpipe]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[hpId]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[BindHyperpipeSGIX]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGIX_video_resize]==],
				},
				["parameters"] = {
					{
						["name"] = [==[display]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[channel]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[dx]==],
						["ctype"] = [==[int *]==],
					},
					{
						["name"] = [==[dy]==],
						["ctype"] = [==[int *]==],
					},
					{
						["name"] = [==[dw]==],
						["ctype"] = [==[int *]==],
					},
					{
						["name"] = [==[dh]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[QueryChannelRectSGIX]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGIX_video_resize]==],
				},
				["parameters"] = {
					{
						["name"] = [==[display]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[channel]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[x]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[y]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[w]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[h]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[ChannelRectSGIX]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[NV_swap_group]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[group]==],
						["ctype"] = [==[GLuint]==],
					},
				},
				["name"] = [==[JoinSwapGroupNV]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[SGI_cushion]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[window]==],
						["ctype"] = [==[Window]==],
					},
					{
						["name"] = [==[cushion]==],
						["ctype"] = [==[float]==],
					},
				},
				["name"] = [==[CushionSGI]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[NV_video_capture]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[video_capture_slot]==],
						["ctype"] = [==[unsigned int]==],
					},
					{
						["name"] = [==[device]==],
						["ctype"] = [==[GLXVideoCaptureDeviceNV]==],
					},
				},
				["name"] = [==[BindVideoCaptureDeviceNV]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGIX_fbconfig]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[attrib_list]==],
						["ctype"] = [==[int *]==],
					},
					{
						["name"] = [==[nelements]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[ChooseFBConfigSGIX]==],
				["return_ctype"] = [==[GLXFBConfigSGIX *]==],
			},
			{
				["extensions"] = {
					[==[EXT_import_context]==],
				},
				["parameters"] = {
					{
						["name"] = [==[context]==],
						["ctype"] = [==[const GLXContext]==],
					},
				},
				["name"] = [==[GetContextIDEXT]==],
				["return_ctype"] = [==[GLXContextID]==],
			},
			{
				["extensions"] = {
					[==[NV_video_capture]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[device]==],
						["ctype"] = [==[GLXVideoCaptureDeviceNV]==],
					},
					{
						["name"] = [==[attribute]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[value]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[QueryVideoCaptureDeviceNV]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[MESA_agp_offset]==],
				},
				["parameters"] = {
					{
						["name"] = [==[pointer]==],
						["ctype"] = [==[const void *]==],
					},
				},
				["name"] = [==[GetAGPOffsetMESA]==],
				["return_ctype"] = [==[unsigned int]==],
			},
			{
				["extensions"] = {
					[==[NV_present_video]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[video_slot]==],
						["ctype"] = [==[unsigned int]==],
					},
					{
						["name"] = [==[video_device]==],
						["ctype"] = [==[unsigned int]==],
					},
					{
						["name"] = [==[attrib_list]==],
						["ctype"] = [==[const int *]==],
					},
				},
				["name"] = [==[BindVideoDeviceNV]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGIX_video_resize]==],
				},
				["parameters"] = {
					{
						["name"] = [==[display]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[channel]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[synctype]==],
						["ctype"] = [==[GLenum]==],
					},
				},
				["name"] = [==[ChannelRectSyncSGIX]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGIX_fbconfig]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[vis]==],
						["ctype"] = [==[XVisualInfo *]==],
					},
				},
				["name"] = [==[GetFBConfigFromVisualSGIX]==],
				["return_ctype"] = [==[GLXFBConfigSGIX]==],
			},
			{
				["extensions"] = {
					[==[NV_video_output]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[VideoDevice]==],
						["ctype"] = [==[GLXVideoDeviceNV]==],
					},
					{
						["name"] = [==[pbuf]==],
						["ctype"] = [==[GLXPbuffer]==],
					},
					{
						["name"] = [==[iVideoBuffer]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[BindVideoImageNV]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGI_swap_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[interval]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[SwapIntervalSGI]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGI_make_current_read]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[draw]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[read]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[ctx]==],
						["ctype"] = [==[GLXContext]==],
					},
				},
				["name"] = [==[MakeCurrentReadSGI]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[OML_sync_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[target_msc]==],
						["ctype"] = [==[int64_t]==],
					},
					{
						["name"] = [==[divisor]==],
						["ctype"] = [==[int64_t]==],
					},
					{
						["name"] = [==[remainder]==],
						["ctype"] = [==[int64_t]==],
					},
				},
				["name"] = [==[SwapBuffersMscOML]==],
				["return_ctype"] = [==[int64_t]==],
			},
			{
				["extensions"] = {
					[==[SGIX_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[mask]==],
						["ctype"] = [==[unsigned long *]==],
					},
				},
				["name"] = [==[GetSelectedEventSGIX]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[NV_video_output]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[VideoDevice]==],
						["ctype"] = [==[GLXVideoDeviceNV]==],
					},
				},
				["name"] = [==[ReleaseVideoDeviceNV]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[ARB_create_context]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[config]==],
						["ctype"] = [==[GLXFBConfig]==],
					},
					{
						["name"] = [==[share_context]==],
						["ctype"] = [==[GLXContext]==],
					},
					{
						["name"] = [==[direct]==],
						["ctype"] = [==[Bool]==],
					},
					{
						["name"] = [==[attrib_list]==],
						["ctype"] = [==[const int *]==],
					},
				},
				["name"] = [==[CreateContextAttribsARB]==],
				["return_ctype"] = [==[GLXContext]==],
			},
			{
				["extensions"] = {
					[==[EXT_import_context]==],
				},
				["parameters"] = {
				},
				["name"] = [==[GetCurrentDisplayEXT]==],
				["return_ctype"] = [==[Display *]==],
			},
			{
				["extensions"] = {
					[==[SGIX_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[pbuf]==],
						["ctype"] = [==[GLXPbufferSGIX]==],
					},
				},
				["name"] = [==[DestroyGLXPbufferSGIX]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[SGIX_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[config]==],
						["ctype"] = [==[GLXFBConfigSGIX]==],
					},
					{
						["name"] = [==[width]==],
						["ctype"] = [==[unsigned int]==],
					},
					{
						["name"] = [==[height]==],
						["ctype"] = [==[unsigned int]==],
					},
					{
						["name"] = [==[attrib_list]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[CreateGLXPbufferSGIX]==],
				["return_ctype"] = [==[GLXPbufferSGIX]==],
			},
			{
				["extensions"] = {
					[==[NV_video_capture]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[nelements]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[EnumerateVideoCaptureDevicesNV]==],
				["return_ctype"] = [==[GLXVideoCaptureDeviceNV *]==],
			},
			{
				["extensions"] = {
					[==[OML_sync_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[numerator]==],
						["ctype"] = [==[int32_t *]==],
					},
					{
						["name"] = [==[denominator]==],
						["ctype"] = [==[int32_t *]==],
					},
				},
				["name"] = [==[GetMscRateOML]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[SGIX_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[mask]==],
						["ctype"] = [==[unsigned long]==],
					},
				},
				["name"] = [==[SelectEventSGIX]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[MESA_copy_sub_buffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[x]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[y]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[width]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[height]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[CopySubBufferMESA]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[SGIX_dmbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[pbuffer]==],
						["ctype"] = [==[GLXPbufferSGIX]==],
					},
					{
						["name"] = [==[params]==],
						["ctype"] = [==[DMparams *]==],
					},
					{
						["name"] = [==[dmbuffer]==],
						["ctype"] = [==[DMbuffer]==],
					},
				},
				["name"] = [==[AssociateDMPbufferSGIX]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[SGIX_hyperpipe]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[networkId]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[npipes]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[cfg]==],
						["ctype"] = [==[GLXHyperpipeConfigSGIX *]==],
					},
					{
						["name"] = [==[hpId]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[HyperpipeConfigSGIX]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[EXT_swap_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[interval]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[SwapIntervalEXT]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[EXT_import_context]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[context]==],
						["ctype"] = [==[GLXContext]==],
					},
					{
						["name"] = [==[attribute]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[value]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[QueryContextInfoEXT]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGIX_fbconfig]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[config]==],
						["ctype"] = [==[GLXFBConfigSGIX]==],
					},
				},
				["name"] = [==[GetVisualFromFBConfigSGIX]==],
				["return_ctype"] = [==[XVisualInfo *]==],
			},
			{
				["extensions"] = {
					[==[NV_present_video]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[nelements]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[EnumerateVideoDevicesNV]==],
				["return_ctype"] = [==[unsigned int *]==],
			},
			{
				["extensions"] = {
					[==[NV_swap_group]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[ResetFrameCountNV]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[NV_copy_image]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[srcCtx]==],
						["ctype"] = [==[GLXContext]==],
					},
					{
						["name"] = [==[srcName]==],
						["ctype"] = [==[GLuint]==],
					},
					{
						["name"] = [==[srcTarget]==],
						["ctype"] = [==[GLenum]==],
					},
					{
						["name"] = [==[srcLevel]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[srcX]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[srcY]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[srcZ]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[dstCtx]==],
						["ctype"] = [==[GLXContext]==],
					},
					{
						["name"] = [==[dstName]==],
						["ctype"] = [==[GLuint]==],
					},
					{
						["name"] = [==[dstTarget]==],
						["ctype"] = [==[GLenum]==],
					},
					{
						["name"] = [==[dstLevel]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[dstX]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[dstY]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[dstZ]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[width]==],
						["ctype"] = [==[GLsizei]==],
					},
					{
						["name"] = [==[height]==],
						["ctype"] = [==[GLsizei]==],
					},
					{
						["name"] = [==[depth]==],
						["ctype"] = [==[GLsizei]==],
					},
				},
				["name"] = [==[CopyImageSubDataNV]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[NV_video_capture]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[device]==],
						["ctype"] = [==[GLXVideoCaptureDeviceNV]==],
					},
				},
				["name"] = [==[ReleaseVideoCaptureDeviceNV]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[NV_swap_group]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[maxGroups]==],
						["ctype"] = [==[GLuint *]==],
					},
					{
						["name"] = [==[maxBarriers]==],
						["ctype"] = [==[GLuint *]==],
					},
				},
				["name"] = [==[QueryMaxSwapGroupsNV]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[NV_video_capture]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[device]==],
						["ctype"] = [==[GLXVideoCaptureDeviceNV]==],
					},
				},
				["name"] = [==[LockVideoCaptureDeviceNV]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[NV_video_output]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[VideoDevice]==],
						["ctype"] = [==[GLXVideoDeviceNV]==],
					},
					{
						["name"] = [==[pulCounterOutputPbuffer]==],
						["ctype"] = [==[unsigned long *]==],
					},
					{
						["name"] = [==[pulCounterOutputVideo]==],
						["ctype"] = [==[unsigned long *]==],
					},
				},
				["name"] = [==[GetVideoInfoNV]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[NV_swap_group]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[count]==],
						["ctype"] = [==[GLuint *]==],
					},
				},
				["name"] = [==[QueryFrameCountNV]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[NV_swap_group]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[group]==],
						["ctype"] = [==[GLuint]==],
					},
					{
						["name"] = [==[barrier]==],
						["ctype"] = [==[GLuint]==],
					},
				},
				["name"] = [==[BindSwapBarrierNV]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[OML_sync_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[target_msc]==],
						["ctype"] = [==[int64_t]==],
					},
					{
						["name"] = [==[divisor]==],
						["ctype"] = [==[int64_t]==],
					},
					{
						["name"] = [==[remainder]==],
						["ctype"] = [==[int64_t]==],
					},
					{
						["name"] = [==[ust]==],
						["ctype"] = [==[int64_t *]==],
					},
					{
						["name"] = [==[msc]==],
						["ctype"] = [==[int64_t *]==],
					},
					{
						["name"] = [==[sbc]==],
						["ctype"] = [==[int64_t *]==],
					},
				},
				["name"] = [==[WaitForMscOML]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[SGI_video_sync]==],
				},
				["parameters"] = {
					{
						["name"] = [==[count]==],
						["ctype"] = [==[unsigned int *]==],
					},
				},
				["name"] = [==[GetVideoSyncSGI]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[EXT_import_context]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[contextID]==],
						["ctype"] = [==[GLXContextID]==],
					},
				},
				["name"] = [==[ImportContextEXT]==],
				["return_ctype"] = [==[GLXContext]==],
			},
			{
				["extensions"] = {
					[==[SGIX_swap_barrier]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[barrier]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[BindSwapBarrierSGIX]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[SGIX_video_resize]==],
				},
				["parameters"] = {
					{
						["name"] = [==[display]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[channel]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[window]==],
						["ctype"] = [==[Window]==],
					},
				},
				["name"] = [==[BindChannelToWindowSGIX]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGIX_swap_group]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
					{
						["name"] = [==[member]==],
						["ctype"] = [==[GLXDrawable]==],
					},
				},
				["name"] = [==[JoinSwapGroupSGIX]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[NV_video_output]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[pbuf]==],
						["ctype"] = [==[GLXPbuffer]==],
					},
					{
						["name"] = [==[iBufferType]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[pulCounterPbuffer]==],
						["ctype"] = [==[unsigned long *]==],
					},
					{
						["name"] = [==[bBlock]==],
						["ctype"] = [==[GLboolean]==],
					},
				},
				["name"] = [==[SendPbufferToVideoNV]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[NV_video_output]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[numVideoDevices]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[pVideoDevice]==],
						["ctype"] = [==[GLXVideoDeviceNV *]==],
					},
				},
				["name"] = [==[GetVideoDeviceNV]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[MESA_set_3dfx_mode]==],
				},
				["parameters"] = {
					{
						["name"] = [==[mode]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[Set3DfxModeMESA]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[SGIX_hyperpipe]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[timeSlice]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[attrib]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[size]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[returnAttribList]==],
						["ctype"] = [==[void *]==],
					},
				},
				["name"] = [==[QueryHyperpipeAttribSGIX]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGIX_hyperpipe]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[npipes]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[QueryHyperpipeNetworkSGIX]==],
				["return_ctype"] = [==[GLXHyperpipeNetworkSGIX *]==],
			},
			{
				["extensions"] = {
					[==[SGI_video_sync]==],
				},
				["parameters"] = {
					{
						["name"] = [==[divisor]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[remainder]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[count]==],
						["ctype"] = [==[unsigned int *]==],
					},
				},
				["name"] = [==[WaitVideoSyncSGI]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGIX_swap_barrier]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[max]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[QueryMaxSwapBarriersSGIX]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[NV_video_output]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[pbuf]==],
						["ctype"] = [==[GLXPbuffer]==],
					},
				},
				["name"] = [==[ReleaseVideoImageNV]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGIX_hyperpipe]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[timeSlice]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[attrib]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[size]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[attribList]==],
						["ctype"] = [==[void *]==],
					},
				},
				["name"] = [==[HyperpipeAttribSGIX]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGIX_video_source]==],
				},
				["parameters"] = {
					{
						["name"] = [==[display]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[server]==],
						["ctype"] = [==[VLServer]==],
					},
					{
						["name"] = [==[path]==],
						["ctype"] = [==[VLPath]==],
					},
					{
						["name"] = [==[nodeClass]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[drainNode]==],
						["ctype"] = [==[VLNode]==],
					},
				},
				["name"] = [==[CreateGLXVideoSourceSGIX]==],
				["return_ctype"] = [==[GLXVideoSourceSGIX]==],
			},
			{
				["extensions"] = {
					[==[MESA_release_buffers]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[drawable]==],
						["ctype"] = [==[GLXDrawable]==],
					},
				},
				["name"] = [==[ReleaseBuffersMESA]==],
				["return_ctype"] = [==[Bool]==],
			},
			{
				["extensions"] = {
					[==[SGI_make_current_read]==],
				},
				["parameters"] = {
				},
				["name"] = [==[GetCurrentReadDrawableSGI]==],
				["return_ctype"] = [==[GLXDrawable]==],
			},
			{
				["extensions"] = {
					[==[MESA_pixmap_colormap]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[visual]==],
						["ctype"] = [==[XVisualInfo *]==],
					},
					{
						["name"] = [==[pixmap]==],
						["ctype"] = [==[Pixmap]==],
					},
					{
						["name"] = [==[cmap]==],
						["ctype"] = [==[Colormap]==],
					},
				},
				["name"] = [==[CreateGLXPixmapMESA]==],
				["return_ctype"] = [==[GLXPixmap]==],
			},
			{
				["extensions"] = {
					[==[SGIX_fbconfig]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[config]==],
						["ctype"] = [==[GLXFBConfigSGIX]==],
					},
					{
						["name"] = [==[pixmap]==],
						["ctype"] = [==[Pixmap]==],
					},
				},
				["name"] = [==[CreateGLXPixmapWithConfigSGIX]==],
				["return_ctype"] = [==[GLXPixmap]==],
			},
			{
				["extensions"] = {
					[==[SGIX_fbconfig]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[config]==],
						["ctype"] = [==[GLXFBConfigSGIX]==],
					},
					{
						["name"] = [==[attribute]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[value]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[GetFBConfigAttribSGIX]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGIX_fbconfig]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[config]==],
						["ctype"] = [==[GLXFBConfigSGIX]==],
					},
					{
						["name"] = [==[render_type]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[share_list]==],
						["ctype"] = [==[GLXContext]==],
					},
					{
						["name"] = [==[direct]==],
						["ctype"] = [==[Bool]==],
					},
				},
				["name"] = [==[CreateContextWithConfigSGIX]==],
				["return_ctype"] = [==[GLXContext]==],
			},
			{
				["extensions"] = {
					[==[SGIX_video_source]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[glxvideosource]==],
						["ctype"] = [==[GLXVideoSourceSGIX]==],
					},
				},
				["name"] = [==[DestroyGLXVideoSourceSGIX]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[SGIX_hyperpipe]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[hpId]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[DestroyHyperpipeConfigSGIX]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SGIX_hyperpipe]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[timeSlice]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[attrib]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[size]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[attribList]==],
						["ctype"] = [==[void *]==],
					},
					{
						["name"] = [==[returnAttribList]==],
						["ctype"] = [==[void *]==],
					},
				},
				["name"] = [==[QueryHyperpipeBestAttribSGIX]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[SUN_get_transparent_index]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dpy]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[overlay]==],
						["ctype"] = [==[Window]==],
					},
					{
						["name"] = [==[underlay]==],
						["ctype"] = [==[Window]==],
					},
					{
						["name"] = [==[pTransparentIndex]==],
						["ctype"] = [==[long *]==],
					},
				},
				["name"] = [==[GetTransparentIndexSUN]==],
				["return_ctype"] = [==[Status]==],
			},
			{
				["extensions"] = {
					[==[SGIX_video_resize]==],
				},
				["parameters"] = {
					{
						["name"] = [==[display]==],
						["ctype"] = [==[Display *]==],
					},
					{
						["name"] = [==[screen]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[channel]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[x]==],
						["ctype"] = [==[int *]==],
					},
					{
						["name"] = [==[y]==],
						["ctype"] = [==[int *]==],
					},
					{
						["name"] = [==[w]==],
						["ctype"] = [==[int *]==],
					},
					{
						["name"] = [==[h]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[QueryChannelDeltasSGIX]==],
				["return_ctype"] = [==[int]==],
			},
		},
	},
	["extensions"] = {
		[==[ARB_get_proc_address]==],
		[==[ARB_multisample]==],
		[==[3DFX_multisample]==],
		[==[ARB_vertex_buffer_object]==],
		[==[ARB_fbconfig_float]==],
		[==[ARB_framebuffer_sRGB]==],
		[==[ARB_create_context]==],
		[==[ARB_create_context_profile]==],
		[==[ARB_create_context_robustness]==],
		[==[ARB_robustness_application_isolation]==],
		[==[ARB_robustness_share_group_isolation]==],
		[==[SGIS_multisample]==],
		[==[EXT_visual_info]==],
		[==[SGI_swap_control]==],
		[==[SGI_video_sync]==],
		[==[SGI_make_current_read]==],
		[==[SGIX_video_source]==],
		[==[EXT_visual_rating]==],
		[==[EXT_import_context]==],
		[==[SGIX_fbconfig]==],
		[==[SGIX_pbuffer]==],
		[==[SGIS_blended_overlay]==],
		[==[SGI_cushion]==],
		[==[SGIS_shared_multisample]==],
		[==[SGIX_video_resize]==],
		[==[SGIX_dmbuffer]==],
		[==[SGIX_swap_group]==],
		[==[SGIX_swap_barrier]==],
		[==[SUN_get_transparent_index]==],
		[==[MESA_copy_sub_buffer]==],
		[==[MESA_pixmap_colormap]==],
		[==[MESA_release_buffers]==],
		[==[MESA_set_3dfx_mode]==],
		[==[SGIX_visual_select_group]==],
		[==[OML_swap_method]==],
		[==[OML_sync_control]==],
		[==[NV_float_buffer]==],
		[==[SGIX_hyperpipe]==],
		[==[MESA_agp_offset]==],
		[==[EXT_fbconfig_packed_float]==],
		[==[EXT_framebuffer_sRGB]==],
		[==[EXT_texture_from_pixmap]==],
		[==[NV_present_video]==],
		[==[NV_video_output]==],
		[==[NV_swap_group]==],
		[==[NV_video_capture]==],
		[==[EXT_swap_control]==],
		[==[NV_copy_image]==],
		[==[INTEL_swap_event]==],
		[==[NV_multisample_coverage]==],
		[==[AMD_gpu_association]==],
		[==[EXT_create_context_es_profile]==],
		[==[EXT_create_context_es2_profile]==],
		[==[EXT_swap_control_tear]==],
		[==[EXT_buffer_age]==],
	},
	["enumerators"] = {
		{
			["value"] = [==[0x00000080]==],
			["name"] = [==[ACCUM_BUFFER_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x20B2]==],
			["name"] = [==[FRAMEBUFFER_SRGB_CAPABLE_EXT]==],
			["extensions"] = {
				[==[EXT_framebuffer_sRGB]==],
			},
		},
		{
			["value"] = [==[0x8016]==],
			["name"] = [==[MAX_PBUFFER_WIDTH_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x8030]==],
			["name"] = [==[HYPERPIPE_ID_SGIX]==],
			["extensions"] = {
				[==[SGIX_hyperpipe]==],
			},
		},
		{
			["value"] = [==[0x20DA]==],
			["name"] = [==[TEXTURE_FORMAT_RGBA_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x20E1]==],
			["name"] = [==[BACK_RIGHT_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x20D7]==],
			["name"] = [==[MIPMAP_TEXTURE_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x8051]==],
			["name"] = [==[SAMPLES_3DFX]==],
			["extensions"] = {
				[==[3DFX_multisample]==],
			},
		},
		{
			["value"] = [==[0x1F02]==],
			["name"] = [==[GPU_OPENGL_VERSION_STRING_AMD]==],
			["extensions"] = {
				[==[AMD_gpu_association]==],
			},
		},
		{
			["value"] = [==[0x8063]==],
			["name"] = [==[SWAP_UNDEFINED_OML]==],
			["extensions"] = {
				[==[OML_swap_method]==],
			},
		},
		{
			["value"] = [==[0x20EA]==],
			["name"] = [==[AUX8_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x8023]==],
			["name"] = [==[PBUFFER_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x801D]==],
			["name"] = [==[WIDTH_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x2092]==],
			["name"] = [==[CONTEXT_MINOR_VERSION_ARB]==],
			["extensions"] = {
				[==[ARB_create_context]==],
			},
		},
		{
			["value"] = [==[0x8061]==],
			["name"] = [==[SWAP_EXCHANGE_OML]==],
			["extensions"] = {
				[==[OML_swap_method]==],
			},
		},
		{
			["value"] = [==[0x20C6]==],
			["name"] = [==[VIDEO_OUT_COLOR_AND_ALPHA_NV]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x20F0]==],
			["name"] = [==[NUM_VIDEO_SLOTS_NV]==],
			["extensions"] = {
				[==[NV_present_video]==],
			},
		},
		{
			["value"] = [==[100000]==],
			["name"] = [==[SAMPLE_BUFFERS_SGIS]==],
			["extensions"] = {
				[==[SGIS_multisample]==],
			},
		},
		{
			["value"] = [==[0x8018]==],
			["name"] = [==[MAX_PBUFFER_PIXELS_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x20E3]==],
			["name"] = [==[AUX1_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x801F]==],
			["name"] = [==[EVENT_MASK_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x8028]==],
			["name"] = [==[VISUAL_SELECT_GROUP_SGIX]==],
			["extensions"] = {
				[==[SGIX_visual_select_group]==],
			},
		},
		{
			["value"] = [==[0x20DC]==],
			["name"] = [==[TEXTURE_2D_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x2]==],
			["name"] = [==[3DFX_FULLSCREEN_MODE_MESA]==],
			["extensions"] = {
				[==[MESA_set_3dfx_mode]==],
			},
		},
		{
			["value"] = [==[0x8261]==],
			["name"] = [==[NO_RESET_NOTIFICATION_ARB]==],
			["extensions"] = {
				[==[ARB_create_context_robustness]==],
			},
		},
		{
			["value"] = [==[0x25]==],
			["name"] = [==[TRANSPARENT_RED_VALUE_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x20CE]==],
			["name"] = [==[UNIQUE_ID_NV]==],
			["extensions"] = {
				[==[NV_video_capture]==],
			},
		},
		{
			["value"] = [==[0x00000001]==],
			["name"] = [==[FRONT_LEFT_BUFFER_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x20E5]==],
			["name"] = [==[AUX3_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x8017]==],
			["name"] = [==[MAX_PBUFFER_HEIGHT_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x21A7]==],
			["name"] = [==[GPU_NUM_RB_AMD]==],
			["extensions"] = {
				[==[AMD_gpu_association]==],
			},
		},
		{
			["value"] = [==[0x00000001]==],
			["name"] = [==[SYNC_SWAP_SGIX]==],
			["extensions"] = {
				[==[SGIX_video_resize]==],
			},
		},
		{
			["value"] = [==[0x8025]==],
			["name"] = [==[BLENDED_RGBA_SGIS]==],
			["extensions"] = {
				[==[SGIS_blended_overlay]==],
			},
		},
		{
			["value"] = [==[0x2095]==],
			["name"] = [==[CONTEXT_ALLOW_BUFFER_BYTE_ORDER_MISMATCH_ARB]==],
			["extensions"] = {
				[==[ARB_vertex_buffer_object]==],
			},
		},
		{
			["value"] = [==[0x8022]==],
			["name"] = [==[WINDOW_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x20DF]==],
			["name"] = [==[FRONT_RIGHT_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x8006]==],
			["name"] = [==[GRAY_SCALE_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x00000004]==],
			["name"] = [==[RGBA_FLOAT_BIT_ARB]==],
			["extensions"] = {
				[==[ARB_fbconfig_float]==],
			},
		},
		{
			["value"] = [==[0x23]==],
			["name"] = [==[TRANSPARENT_TYPE_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x00000000]==],
			["name"] = [==[SYNC_FRAME_SGIX]==],
			["extensions"] = {
				[==[SGIX_video_resize]==],
			},
		},
		{
			["value"] = [==[0x8060]==],
			["name"] = [==[SWAP_METHOD_OML]==],
			["extensions"] = {
				[==[OML_swap_method]==],
			},
		},
		{
			["value"] = [==[0x20C8]==],
			["name"] = [==[VIDEO_OUT_FRAME_NV]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x21A8]==],
			["name"] = [==[GPU_NUM_SPI_AMD]==],
			["extensions"] = {
				[==[AMD_gpu_association]==],
			},
		},
		{
			["value"] = [==[0x8062]==],
			["name"] = [==[SWAP_COPY_OML]==],
			["extensions"] = {
				[==[OML_swap_method]==],
			},
		},
		{
			["value"] = [==[0x00000008]==],
			["name"] = [==[CONTEXT_RESET_ISOLATION_BIT_ARB]==],
			["extensions"] = {
				[==[ARB_robustness_application_isolation]==],
				[==[ARB_robustness_share_group_isolation]==],
			},
		},
		{
			["value"] = [==[0x8020]==],
			["name"] = [==[DAMAGED_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x8256]==],
			["name"] = [==[CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB]==],
			["extensions"] = {
				[==[ARB_create_context_robustness]==],
			},
		},
		{
			["value"] = [==[0x8004]==],
			["name"] = [==[PSEUDO_COLOR_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x20D9]==],
			["name"] = [==[TEXTURE_FORMAT_RGB_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x801E]==],
			["name"] = [==[HEIGHT_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x00000004]==],
			["name"] = [==[CONTEXT_ES_PROFILE_BIT_EXT]==],
			["extensions"] = {
				[==[EXT_create_context_es_profile]==],
			},
		},
		{
			["value"] = [==[0x00000001]==],
			["name"] = [==[CONTEXT_DEBUG_BIT_ARB]==],
			["extensions"] = {
				[==[ARB_create_context]==],
			},
		},
		{
			["value"] = [==[0x21A3]==],
			["name"] = [==[GPU_RAM_AMD]==],
			["extensions"] = {
				[==[AMD_gpu_association]==],
			},
		},
		{
			["value"] = [==[0x8003]==],
			["name"] = [==[DIRECT_COLOR_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x20F4]==],
			["name"] = [==[BACK_BUFFER_AGE_EXT]==],
			["extensions"] = {
				[==[EXT_buffer_age]==],
			},
		},
		{
			["value"] = [==[0x800B]==],
			["name"] = [==[VISUAL_ID_EXT]==],
			["extensions"] = {
				[==[EXT_import_context]==],
			},
		},
		{
			["value"] = [==[0x00000002]==],
			["name"] = [==[CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB]==],
			["extensions"] = {
				[==[ARB_create_context_profile]==],
			},
		},
		{
			["value"] = [==[0x20E2]==],
			["name"] = [==[AUX0_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x20B1]==],
			["name"] = [==[RGBA_UNSIGNED_FLOAT_TYPE_EXT]==],
			["extensions"] = {
				[==[EXT_fbconfig_packed_float]==],
			},
		},
		{
			["value"] = [==[0x28]==],
			["name"] = [==[TRANSPARENT_ALPHA_VALUE_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x8050]==],
			["name"] = [==[SAMPLE_BUFFERS_3DFX]==],
			["extensions"] = {
				[==[3DFX_multisample]==],
			},
		},
		{
			["value"] = [==[0x20D8]==],
			["name"] = [==[TEXTURE_FORMAT_NONE_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x00000003]==],
			["name"] = [==[HYPERPIPE_STEREO_SGIX]==],
			["extensions"] = {
				[==[SGIX_hyperpipe]==],
			},
		},
		{
			["value"] = [==[0x800D]==],
			["name"] = [==[NON_CONFORMANT_VISUAL_EXT]==],
			["extensions"] = {
				[==[EXT_visual_rating]==],
			},
		},
		{
			["value"] = [==[92]==],
			["name"] = [==[BAD_HYPERPIPE_SGIX]==],
			["extensions"] = {
				[==[SGIX_hyperpipe]==],
			},
		},
		{
			["value"] = [==[0x04000000]==],
			["name"] = [==[BUFFER_SWAP_COMPLETE_INTEL_MASK]==],
			["extensions"] = {
				[==[INTEL_swap_event]==],
			},
		},
		{
			["value"] = [==[0x00000002]==],
			["name"] = [==[CONTEXT_FORWARD_COMPATIBLE_BIT_ARB]==],
			["extensions"] = {
				[==[ARB_create_context]==],
			},
		},
		{
			["value"] = [==[0x800A]==],
			["name"] = [==[SHARE_CONTEXT_EXT]==],
			["extensions"] = {
				[==[EXT_import_context]==],
			},
		},
		{
			["value"] = [==[0x801A]==],
			["name"] = [==[OPTIMAL_PBUFFER_HEIGHT_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[100001]==],
			["name"] = [==[SAMPLES_ARB]==],
			["extensions"] = {
				[==[ARB_multisample]==],
			},
		},
		{
			["value"] = [==[0x20B3]==],
			["name"] = [==[COLOR_SAMPLES_NV]==],
			["extensions"] = {
				[==[NV_multisample_coverage]==],
			},
		},
		{
			["value"] = [==[0x20E8]==],
			["name"] = [==[AUX6_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x21A6]==],
			["name"] = [==[GPU_NUM_SIMD_AMD]==],
			["extensions"] = {
				[==[AMD_gpu_association]==],
			},
		},
		{
			["value"] = [==[0x24]==],
			["name"] = [==[TRANSPARENT_INDEX_VALUE_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x20D6]==],
			["name"] = [==[TEXTURE_TARGET_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x00000004]==],
			["name"] = [==[CONTEXT_ROBUST_ACCESS_BIT_ARB]==],
			["extensions"] = {
				[==[ARB_create_context_robustness]==],
			},
		},
		{
			["value"] = [==[0x00000001]==],
			["name"] = [==[PIPE_RECT_SGIX]==],
			["extensions"] = {
				[==[SGIX_hyperpipe]==],
			},
		},
		{
			["value"] = [==[0x1]==],
			["name"] = [==[3DFX_WINDOW_MODE_MESA]==],
			["extensions"] = {
				[==[MESA_set_3dfx_mode]==],
			},
		},
		{
			["value"] = [==[0x20E0]==],
			["name"] = [==[BACK_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x8009]==],
			["name"] = [==[TRANSPARENT_INDEX_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x8024]==],
			["name"] = [==[DIGITAL_MEDIA_PBUFFER_SGIX]==],
			["extensions"] = {
				[==[SGIX_dmbuffer]==],
			},
		},
		{
			["value"] = [==[0x20CF]==],
			["name"] = [==[NUM_VIDEO_CAPTURE_SLOTS_NV]==],
			["extensions"] = {
				[==[NV_video_capture]==],
			},
		},
		{
			["value"] = [==[100001]==],
			["name"] = [==[SAMPLES_SGIS]==],
			["extensions"] = {
				[==[SGIS_multisample]==],
			},
		},
		{
			["value"] = [==[0x20B9]==],
			["name"] = [==[RGBA_FLOAT_TYPE_ARB]==],
			["extensions"] = {
				[==[ARB_fbconfig_float]==],
			},
		},
		{
			["value"] = [==[0x20F3]==],
			["name"] = [==[LATE_SWAPS_TEAR_EXT]==],
			["extensions"] = {
				[==[EXT_swap_control_tear]==],
			},
		},
		{
			["value"] = [==[0x00000004]==],
			["name"] = [==[TEXTURE_RECTANGLE_BIT_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x2091]==],
			["name"] = [==[CONTEXT_MAJOR_VERSION_ARB]==],
			["extensions"] = {
				[==[ARB_create_context]==],
			},
		},
		{
			["value"] = [==[0x08000000]==],
			["name"] = [==[BUFFER_CLOBBER_MASK_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x8001]==],
			["name"] = [==[SLOW_VISUAL_EXT]==],
			["extensions"] = {
				[==[EXT_visual_rating]==],
			},
		},
		{
			["value"] = [==[0x22]==],
			["name"] = [==[X_VISUAL_TYPE_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x8013]==],
			["name"] = [==[FBCONFIG_ID_SGIX]==],
			["extensions"] = {
				[==[SGIX_fbconfig]==],
			},
		},
		{
			["value"] = [==[0x8008]==],
			["name"] = [==[TRANSPARENT_RGB_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x20C4]==],
			["name"] = [==[VIDEO_OUT_ALPHA_NV]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x21A2]==],
			["name"] = [==[GPU_FASTEST_TARGET_GPUS_AMD]==],
			["extensions"] = {
				[==[AMD_gpu_association]==],
			},
		},
		{
			["value"] = [==[0x1F01]==],
			["name"] = [==[GPU_RENDERER_STRING_AMD]==],
			["extensions"] = {
				[==[AMD_gpu_association]==],
			},
		},
		{
			["value"] = [==[0x1F00]==],
			["name"] = [==[GPU_VENDOR_AMD]==],
			["extensions"] = {
				[==[AMD_gpu_association]==],
			},
		},
		{
			["value"] = [==[100001]==],
			["name"] = [==[COVERAGE_SAMPLES_NV]==],
			["extensions"] = {
				[==[NV_multisample_coverage]==],
			},
		},
		{
			["value"] = [==[0x8182]==],
			["name"] = [==[FLIP_COMPLETE_INTEL]==],
			["extensions"] = {
				[==[INTEL_swap_event]==],
			},
		},
		{
			["value"] = [==[0x8005]==],
			["name"] = [==[STATIC_COLOR_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x9126]==],
			["name"] = [==[CONTEXT_PROFILE_MASK_ARB]==],
			["extensions"] = {
				[==[ARB_create_context_profile]==],
			},
		},
		{
			["value"] = [==[0x8181]==],
			["name"] = [==[COPY_COMPLETE_INTEL]==],
			["extensions"] = {
				[==[INTEL_swap_event]==],
			},
		},
		{
			["value"] = [==[0x20D4]==],
			["name"] = [==[Y_INVERTED_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x00000001]==],
			["name"] = [==[HYPERPIPE_DISPLAY_PIPE_SGIX]==],
			["extensions"] = {
				[==[SGIX_hyperpipe]==],
			},
		},
		{
			["value"] = [==[0x20D3]==],
			["name"] = [==[BIND_TO_TEXTURE_TARGETS_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x00000004]==],
			["name"] = [==[BACK_LEFT_BUFFER_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x8180]==],
			["name"] = [==[EXCHANGE_COMPLETE_INTEL]==],
			["extensions"] = {
				[==[INTEL_swap_event]==],
			},
		},
		{
			["value"] = [==[0x00000008]==],
			["name"] = [==[BACK_RIGHT_BUFFER_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x8007]==],
			["name"] = [==[STATIC_GRAY_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x00000004]==],
			["name"] = [==[PBUFFER_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[91]==],
			["name"] = [==[BAD_HYPERPIPE_CONFIG_SGIX]==],
			["extensions"] = {
				[==[SGIX_hyperpipe]==],
			},
		},
		{
			["value"] = [==[0x20F1]==],
			["name"] = [==[SWAP_INTERVAL_EXT]==],
			["extensions"] = {
				[==[EXT_swap_control]==],
			},
		},
		{
			["value"] = [==[0x00000001]==],
			["name"] = [==[WINDOW_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_fbconfig]==],
			},
		},
		{
			["value"] = [==[0x8021]==],
			["name"] = [==[SAVED_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x8002]==],
			["name"] = [==[TRUE_COLOR_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x20CD]==],
			["name"] = [==[DEVICE_ID_NV]==],
			["extensions"] = {
				[==[NV_video_capture]==],
			},
		},
		{
			["value"] = [==[0x00000001]==],
			["name"] = [==[TEXTURE_1D_BIT_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x20CC]==],
			["name"] = [==[VIDEO_OUT_STACKED_FIELDS_2_1_NV]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x00000002]==],
			["name"] = [==[PIPE_RECT_LIMITS_SGIX]==],
			["extensions"] = {
				[==[SGIX_hyperpipe]==],
			},
		},
		{
			["value"] = [==[0x20CB]==],
			["name"] = [==[VIDEO_OUT_STACKED_FIELDS_1_2_NV]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x8014]==],
			["name"] = [==[RGBA_TYPE_SGIX]==],
			["extensions"] = {
				[==[SGIX_fbconfig]==],
			},
		},
		{
			["value"] = [==[0x2094]==],
			["name"] = [==[CONTEXT_FLAGS_ARB]==],
			["extensions"] = {
				[==[ARB_create_context]==],
			},
		},
		{
			["value"] = [==[0x20C9]==],
			["name"] = [==[VIDEO_OUT_FIELD_1_NV]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x20C7]==],
			["name"] = [==[VIDEO_OUT_COLOR_AND_DEPTH_NV]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x00000010]==],
			["name"] = [==[AUX_BUFFERS_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x8026]==],
			["name"] = [==[MULTISAMPLE_SUB_RECT_WIDTH_SGIS]==],
			["extensions"] = {
				[==[SGIS_shared_multisample]==],
			},
		},
		{
			["value"] = [==[0x21A4]==],
			["name"] = [==[GPU_CLOCK_AMD]==],
			["extensions"] = {
				[==[AMD_gpu_association]==],
			},
		},
		{
			["value"] = [==[0x20DD]==],
			["name"] = [==[TEXTURE_RECTANGLE_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x20EB]==],
			["name"] = [==[AUX9_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x20E9]==],
			["name"] = [==[AUX7_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x20E7]==],
			["name"] = [==[AUX5_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x20E6]==],
			["name"] = [==[AUX4_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x20B0]==],
			["name"] = [==[FLOAT_COMPONENTS_NV]==],
			["extensions"] = {
				[==[NV_float_buffer]==],
			},
		},
		{
			["value"] = [==[0x20D1]==],
			["name"] = [==[BIND_TO_TEXTURE_RGBA_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x00000020]==],
			["name"] = [==[DEPTH_BUFFER_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x20E0]==],
			["name"] = [==[BACK_LEFT_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[100000]==],
			["name"] = [==[SAMPLE_BUFFERS_ARB]==],
			["extensions"] = {
				[==[ARB_multisample]==],
			},
		},
		{
			["value"] = [==[0x00000002]==],
			["name"] = [==[FRONT_RIGHT_BUFFER_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x00000100]==],
			["name"] = [==[SAMPLE_BUFFERS_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x20C3]==],
			["name"] = [==[VIDEO_OUT_COLOR_NV]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[80]==],
			["name"] = [==[HYPERPIPE_PIPE_NAME_LENGTH_SGIX]==],
			["extensions"] = {
				[==[SGIX_hyperpipe]==],
			},
		},
		{
			["value"] = [==[0x8019]==],
			["name"] = [==[OPTIMAL_PBUFFER_WIDTH_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x20C5]==],
			["name"] = [==[VIDEO_OUT_DEPTH_NV]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x00000004]==],
			["name"] = [==[CONTEXT_ES2_PROFILE_BIT_EXT]==],
			["extensions"] = {
				[==[EXT_create_context_es2_profile]==],
			},
		},
		{
			["value"] = [==[0x20B2]==],
			["name"] = [==[FRAMEBUFFER_SRGB_CAPABLE_ARB]==],
			["extensions"] = {
				[==[ARB_framebuffer_sRGB]==],
			},
		},
		{
			["value"] = [==[0x00000001]==],
			["name"] = [==[RGBA_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_fbconfig]==],
			},
		},
		{
			["value"] = [==[0x20D5]==],
			["name"] = [==[TEXTURE_FORMAT_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x00000008]==],
			["name"] = [==[RGBA_UNSIGNED_FLOAT_BIT_EXT]==],
			["extensions"] = {
				[==[EXT_fbconfig_packed_float]==],
			},
		},
		{
			["value"] = [==[0x8000]==],
			["name"] = [==[NONE_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
				[==[EXT_visual_rating]==],
			},
		},
		{
			["value"] = [==[0x20]==],
			["name"] = [==[VISUAL_CAVEAT_EXT]==],
			["extensions"] = {
				[==[EXT_visual_rating]==],
			},
		},
		{
			["value"] = [==[0x20DE]==],
			["name"] = [==[FRONT_LEFT_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x00000002]==],
			["name"] = [==[COLOR_INDEX_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_fbconfig]==],
			},
		},
		{
			["value"] = [==[0x20DE]==],
			["name"] = [==[FRONT_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x20D2]==],
			["name"] = [==[BIND_TO_MIPMAP_TEXTURE_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x20DB]==],
			["name"] = [==[TEXTURE_1D_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x00000002]==],
			["name"] = [==[TEXTURE_2D_BIT_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x00000040]==],
			["name"] = [==[STENCIL_BUFFER_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x801C]==],
			["name"] = [==[LARGEST_PBUFFER_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x8012]==],
			["name"] = [==[X_RENDERABLE_SGIX]==],
			["extensions"] = {
				[==[SGIX_fbconfig]==],
			},
		},
		{
			["value"] = [==[0x8015]==],
			["name"] = [==[COLOR_INDEX_TYPE_SGIX]==],
			["extensions"] = {
				[==[SGIX_fbconfig]==],
			},
		},
		{
			["value"] = [==[0x00000004]==],
			["name"] = [==[HYPERPIPE_PIXEL_AVERAGE_SGIX]==],
			["extensions"] = {
				[==[SGIX_hyperpipe]==],
			},
		},
		{
			["value"] = [==[0x20CA]==],
			["name"] = [==[VIDEO_OUT_FIELD_2_NV]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x21A5]==],
			["name"] = [==[GPU_NUM_PIPES_AMD]==],
			["extensions"] = {
				[==[AMD_gpu_association]==],
			},
		},
		{
			["value"] = [==[0x00000002]==],
			["name"] = [==[PIXMAP_BIT_SGIX]==],
			["extensions"] = {
				[==[SGIX_fbconfig]==],
			},
		},
		{
			["value"] = [==[0x20F2]==],
			["name"] = [==[MAX_SWAP_INTERVAL_EXT]==],
			["extensions"] = {
				[==[EXT_swap_control]==],
			},
		},
		{
			["value"] = [==[0x00000002]==],
			["name"] = [==[HYPERPIPE_RENDER_PIPE_SGIX]==],
			["extensions"] = {
				[==[SGIX_hyperpipe]==],
			},
		},
		{
			["value"] = [==[0x8010]==],
			["name"] = [==[DRAWABLE_TYPE_SGIX]==],
			["extensions"] = {
				[==[SGIX_fbconfig]==],
			},
		},
		{
			["value"] = [==[0x20D0]==],
			["name"] = [==[BIND_TO_TEXTURE_RGB_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x801B]==],
			["name"] = [==[PRESERVED_CONTENTS_SGIX]==],
			["extensions"] = {
				[==[SGIX_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x20E4]==],
			["name"] = [==[AUX2_EXT]==],
			["extensions"] = {
				[==[EXT_texture_from_pixmap]==],
			},
		},
		{
			["value"] = [==[0x8252]==],
			["name"] = [==[LOSE_CONTEXT_ON_RESET_ARB]==],
			["extensions"] = {
				[==[ARB_create_context_robustness]==],
			},
		},
		{
			["value"] = [==[0x26]==],
			["name"] = [==[TRANSPARENT_GREEN_VALUE_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x8027]==],
			["name"] = [==[MULTISAMPLE_SUB_RECT_HEIGHT_SGIS]==],
			["extensions"] = {
				[==[SGIS_shared_multisample]==],
			},
		},
		{
			["value"] = [==[0x800C]==],
			["name"] = [==[SCREEN_EXT]==],
			["extensions"] = {
				[==[EXT_import_context]==],
				[==[SGIX_fbconfig]==],
			},
		},
		{
			["value"] = [==[0x27]==],
			["name"] = [==[TRANSPARENT_BLUE_VALUE_EXT]==],
			["extensions"] = {
				[==[EXT_visual_info]==],
			},
		},
		{
			["value"] = [==[0x8011]==],
			["name"] = [==[RENDER_TYPE_SGIX]==],
			["extensions"] = {
				[==[SGIX_fbconfig]==],
			},
		},
		{
			["value"] = [==[0x00000001]==],
			["name"] = [==[CONTEXT_CORE_PROFILE_BIT_ARB]==],
			["extensions"] = {
				[==[ARB_create_context_profile]==],
			},
		},
	},
};
