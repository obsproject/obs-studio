return {
	["funcData"] = {
		["passthru"] = {
			[==[struct _GPU_DEVICE {
    DWORD  cb;
    CHAR   DeviceName[32];
    CHAR   DeviceString[128];
    DWORD  Flags;
    RECT   rcVirtualScreen;
};]==],
			[==[DECLARE_HANDLE(HPBUFFERARB);]==],
			[==[DECLARE_HANDLE(HPBUFFEREXT);]==],
			[==[DECLARE_HANDLE(HVIDEOOUTPUTDEVICENV);]==],
			[==[DECLARE_HANDLE(HPVIDEODEV);]==],
			[==[DECLARE_HANDLE(HGPUNV);]==],
			[==[DECLARE_HANDLE(HVIDEOINPUTDEVICENV);]==],
			[==[typedef struct _GPU_DEVICE *PGPU_DEVICE;]==],
		},
		["functions"] = {
			{
				["extensions"] = {
					[==[I3D_gamma]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iAttribute]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[piValue]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[GetGammaTableParametersI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[EXT_extensions_string]==],
				},
				["parameters"] = {
				},
				["name"] = [==[GetExtensionsStringEXT]==],
				["return_ctype"] = [==[const char *]==],
			},
			{
				["extensions"] = {
					[==[ARB_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iPixelFormat]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[iWidth]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[iHeight]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[piAttribList]==],
						["ctype"] = [==[const int *]==],
					},
				},
				["name"] = [==[CreatePbufferARB]==],
				["return_ctype"] = [==[HPBUFFERARB]==],
			},
			{
				["extensions"] = {
					[==[NV_swap_group]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
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
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_genlock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
				},
				["name"] = [==[EnableGenlockI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_gamma]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iEntries]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[puRed]==],
						["ctype"] = [==[USHORT *]==],
					},
					{
						["name"] = [==[puGreen]==],
						["ctype"] = [==[USHORT *]==],
					},
					{
						["name"] = [==[puBlue]==],
						["ctype"] = [==[USHORT *]==],
					},
				},
				["name"] = [==[GetGammaTableI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_vertex_array_range]==],
				},
				["parameters"] = {
					{
						["name"] = [==[pointer]==],
						["ctype"] = [==[void *]==],
					},
				},
				["name"] = [==[FreeMemoryNV]==],
				["return_ctype"] = [==[void]==],
			},
			{
				["extensions"] = {
					[==[EXT_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iPixelFormat]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[iWidth]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[iHeight]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[piAttribList]==],
						["ctype"] = [==[const int *]==],
					},
				},
				["name"] = [==[CreatePbufferEXT]==],
				["return_ctype"] = [==[HPBUFFEREXT]==],
			},
			{
				["extensions"] = {
					[==[OML_sync_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[target_msc]==],
						["ctype"] = [==[INT64]==],
					},
					{
						["name"] = [==[divisor]==],
						["ctype"] = [==[INT64]==],
					},
					{
						["name"] = [==[remainder]==],
						["ctype"] = [==[INT64]==],
					},
					{
						["name"] = [==[ust]==],
						["ctype"] = [==[INT64 *]==],
					},
					{
						["name"] = [==[msc]==],
						["ctype"] = [==[INT64 *]==],
					},
					{
						["name"] = [==[sbc]==],
						["ctype"] = [==[INT64 *]==],
					},
				},
				["name"] = [==[WaitForMscOML]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_DX_interop]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDevice]==],
						["ctype"] = [==[HANDLE]==],
					},
					{
						["name"] = [==[count]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[hObjects]==],
						["ctype"] = [==[HANDLE *]==],
					},
				},
				["name"] = [==[DXUnlockObjectsNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[ARB_pixel_format]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iPixelFormat]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[iLayerPlane]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[nAttributes]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[piAttributes]==],
						["ctype"] = [==[const int *]==],
					},
					{
						["name"] = [==[pfValues]==],
						["ctype"] = [==[FLOAT *]==],
					},
				},
				["name"] = [==[GetPixelFormatAttribfvARB]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_DX_interop]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hObject]==],
						["ctype"] = [==[HANDLE]==],
					},
					{
						["name"] = [==[access]==],
						["ctype"] = [==[GLenum]==],
					},
				},
				["name"] = [==[DXObjectAccessNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_digital_video_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iAttribute]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[piValue]==],
						["ctype"] = [==[const int *]==],
					},
				},
				["name"] = [==[SetDigitalVideoParametersI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_DX_interop]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDevice]==],
						["ctype"] = [==[HANDLE]==],
					},
					{
						["name"] = [==[hObject]==],
						["ctype"] = [==[HANDLE]==],
					},
				},
				["name"] = [==[DXUnregisterObjectNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[ARB_make_current_read]==],
				},
				["parameters"] = {
				},
				["name"] = [==[GetCurrentReadDCARB]==],
				["return_ctype"] = [==[HDC]==],
			},
			{
				["extensions"] = {
					[==[NV_present_video]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[phDeviceList]==],
						["ctype"] = [==[HVIDEOOUTPUTDEVICENV *]==],
					},
				},
				["name"] = [==[EnumerateVideoDevicesNV]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[NV_gpu_affinity]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hAffinityDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iGpuIndex]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[hGpu]==],
						["ctype"] = [==[HGPUNV *]==],
					},
				},
				["name"] = [==[EnumGpusFromAffinityDCNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_genlock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[uDelay]==],
						["ctype"] = [==[UINT]==],
					},
				},
				["name"] = [==[GenlockSourceDelayI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_DX_interop]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDevice]==],
						["ctype"] = [==[HANDLE]==],
					},
					{
						["name"] = [==[dxObject]==],
						["ctype"] = [==[void *]==],
					},
					{
						["name"] = [==[name]==],
						["ctype"] = [==[GLuint]==],
					},
					{
						["name"] = [==[type]==],
						["ctype"] = [==[GLenum]==],
					},
					{
						["name"] = [==[access]==],
						["ctype"] = [==[GLenum]==],
					},
				},
				["name"] = [==[DXRegisterObjectNV]==],
				["return_ctype"] = [==[HANDLE]==],
			},
			{
				["extensions"] = {
					[==[NV_video_output]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFERARB]==],
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
						["ctype"] = [==[BOOL]==],
					},
				},
				["name"] = [==[SendPbufferToVideoNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_genlock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[uMaxLineDelay]==],
						["ctype"] = [==[UINT *]==],
					},
					{
						["name"] = [==[uMaxPixelDelay]==],
						["ctype"] = [==[UINT *]==],
					},
				},
				["name"] = [==[QueryGenlockMaxSourceDelayI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_video_output]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hpVideoDevice]==],
						["ctype"] = [==[HPVIDEODEV]==],
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
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[ARB_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFERARB]==],
					},
					{
						["name"] = [==[iAttribute]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[piValue]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[QueryPbufferARB]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_vertex_array_range]==],
				},
				["parameters"] = {
					{
						["name"] = [==[size]==],
						["ctype"] = [==[GLsizei]==],
					},
					{
						["name"] = [==[readfreq]==],
						["ctype"] = [==[GLfloat]==],
					},
					{
						["name"] = [==[writefreq]==],
						["ctype"] = [==[GLfloat]==],
					},
					{
						["name"] = [==[priority]==],
						["ctype"] = [==[GLfloat]==],
					},
				},
				["name"] = [==[AllocateMemoryNV]==],
				["return_ctype"] = [==[void *]==],
			},
			{
				["extensions"] = {
					[==[OML_sync_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[ust]==],
						["ctype"] = [==[INT64 *]==],
					},
					{
						["name"] = [==[msc]==],
						["ctype"] = [==[INT64 *]==],
					},
					{
						["name"] = [==[sbc]==],
						["ctype"] = [==[INT64 *]==],
					},
				},
				["name"] = [==[GetSyncValuesOML]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_swap_group]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
				},
				["name"] = [==[ResetFrameCountNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_DX_interop]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dxObject]==],
						["ctype"] = [==[void *]==],
					},
					{
						["name"] = [==[shareHandle]==],
						["ctype"] = [==[HANDLE]==],
					},
				},
				["name"] = [==[DXSetResourceShareHandleNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_video_capture]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[hDevice]==],
						["ctype"] = [==[HVIDEOINPUTDEVICENV]==],
					},
				},
				["name"] = [==[ReleaseVideoCaptureDeviceNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_copy_image]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hSrcRC]==],
						["ctype"] = [==[HGLRC]==],
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
						["name"] = [==[hDstRC]==],
						["ctype"] = [==[HGLRC]==],
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
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_genlock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[uSource]==],
						["ctype"] = [==[UINT]==],
					},
				},
				["name"] = [==[GenlockSourceI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_video_capture]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[hDevice]==],
						["ctype"] = [==[HVIDEOINPUTDEVICENV]==],
					},
				},
				["name"] = [==[LockVideoCaptureDeviceNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[AMD_gpu_association]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hglrc]==],
						["ctype"] = [==[HGLRC]==],
					},
				},
				["name"] = [==[DeleteAssociatedContextAMD]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_swap_group]==],
				},
				["parameters"] = {
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
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_gamma]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iAttribute]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[piValue]==],
						["ctype"] = [==[const int *]==],
					},
				},
				["name"] = [==[SetGammaTableParametersI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_video_capture]==],
				},
				["parameters"] = {
					{
						["name"] = [==[uVideoSlot]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[hDevice]==],
						["ctype"] = [==[HVIDEOINPUTDEVICENV]==],
					},
				},
				["name"] = [==[BindVideoCaptureDeviceNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[EXT_display_color_table]==],
				},
				["parameters"] = {
					{
						["name"] = [==[id]==],
						["ctype"] = [==[GLushort]==],
					},
				},
				["name"] = [==[CreateDisplayColorTableEXT]==],
				["return_ctype"] = [==[GLboolean]==],
			},
			{
				["extensions"] = {
					[==[AMD_gpu_association]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dstCtx]==],
						["ctype"] = [==[HGLRC]==],
					},
					{
						["name"] = [==[srcX0]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[srcY0]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[srcX1]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[srcY1]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[dstX0]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[dstY0]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[dstX1]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[dstY1]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[mask]==],
						["ctype"] = [==[GLbitfield]==],
					},
					{
						["name"] = [==[filter]==],
						["ctype"] = [==[GLenum]==],
					},
				},
				["name"] = [==[BlitContextFramebufferAMD]==],
				["return_ctype"] = [==[VOID]==],
			},
			{
				["extensions"] = {
					[==[ARB_render_texture]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFERARB]==],
					},
					{
						["name"] = [==[iBuffer]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[ReleaseTexImageARB]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_swap_group]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[count]==],
						["ctype"] = [==[GLuint *]==],
					},
				},
				["name"] = [==[QueryFrameCountNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_swap_group]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
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
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[EXT_swap_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[interval]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[SwapIntervalEXT]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[AMD_gpu_association]==],
				},
				["parameters"] = {
				},
				["name"] = [==[GetCurrentAssociatedContextAMD]==],
				["return_ctype"] = [==[HGLRC]==],
			},
			{
				["extensions"] = {
					[==[AMD_gpu_association]==],
				},
				["parameters"] = {
					{
						["name"] = [==[id]==],
						["ctype"] = [==[UINT]==],
					},
				},
				["name"] = [==[CreateAssociatedContextAMD]==],
				["return_ctype"] = [==[HGLRC]==],
			},
			{
				["extensions"] = {
					[==[AMD_gpu_association]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hglrc]==],
						["ctype"] = [==[HGLRC]==],
					},
				},
				["name"] = [==[GetContextGPUIDAMD]==],
				["return_ctype"] = [==[UINT]==],
			},
			{
				["extensions"] = {
					[==[AMD_gpu_association]==],
				},
				["parameters"] = {
					{
						["name"] = [==[id]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[property]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[dataType]==],
						["ctype"] = [==[GLenum]==],
					},
					{
						["name"] = [==[size]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[data]==],
						["ctype"] = [==[void *]==],
					},
				},
				["name"] = [==[GetGPUInfoAMD]==],
				["return_ctype"] = [==[INT]==],
			},
			{
				["extensions"] = {
					[==[AMD_gpu_association]==],
				},
				["parameters"] = {
					{
						["name"] = [==[maxCount]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[ids]==],
						["ctype"] = [==[UINT *]==],
					},
				},
				["name"] = [==[GetGPUIDsAMD]==],
				["return_ctype"] = [==[UINT]==],
			},
			{
				["extensions"] = {
					[==[I3D_genlock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[uRate]==],
						["ctype"] = [==[UINT *]==],
					},
				},
				["name"] = [==[GetGenlockSampleRateI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_image_buffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[pAddress]==],
						["ctype"] = [==[const LPVOID *]==],
					},
					{
						["name"] = [==[count]==],
						["ctype"] = [==[UINT]==],
					},
				},
				["name"] = [==[ReleaseImageBufferEventsI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[EXT_display_color_table]==],
				},
				["parameters"] = {
					{
						["name"] = [==[table]==],
						["ctype"] = [==[const GLushort *]==],
					},
					{
						["name"] = [==[length]==],
						["ctype"] = [==[GLuint]==],
					},
				},
				["name"] = [==[LoadDisplayColorTableEXT]==],
				["return_ctype"] = [==[GLboolean]==],
			},
			{
				["extensions"] = {
					[==[NV_video_capture]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[phDeviceList]==],
						["ctype"] = [==[HVIDEOINPUTDEVICENV *]==],
					},
				},
				["name"] = [==[EnumerateVideoCaptureDevicesNV]==],
				["return_ctype"] = [==[UINT]==],
			},
			{
				["extensions"] = {
					[==[NV_gpu_affinity]==],
				},
				["parameters"] = {
					{
						["name"] = [==[phGpuList]==],
						["ctype"] = [==[const HGPUNV *]==],
					},
				},
				["name"] = [==[CreateAffinityDCNV]==],
				["return_ctype"] = [==[HDC]==],
			},
			{
				["extensions"] = {
					[==[NV_gpu_affinity]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hGpu]==],
						["ctype"] = [==[HGPUNV]==],
					},
					{
						["name"] = [==[iDeviceIndex]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[lpGpuDevice]==],
						["ctype"] = [==[PGPU_DEVICE]==],
					},
				},
				["name"] = [==[EnumGpuDevicesNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_swap_frame_usage]==],
				},
				["parameters"] = {
				},
				["name"] = [==[EndFrameTrackingI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[AMD_gpu_association]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hglrc]==],
						["ctype"] = [==[HGLRC]==],
					},
				},
				["name"] = [==[MakeAssociatedContextCurrentAMD]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_genlock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[uEdge]==],
						["ctype"] = [==[UINT]==],
					},
				},
				["name"] = [==[GenlockSourceEdgeI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[OML_sync_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[fuPlanes]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[target_msc]==],
						["ctype"] = [==[INT64]==],
					},
					{
						["name"] = [==[divisor]==],
						["ctype"] = [==[INT64]==],
					},
					{
						["name"] = [==[remainder]==],
						["ctype"] = [==[INT64]==],
					},
				},
				["name"] = [==[SwapLayerBuffersMscOML]==],
				["return_ctype"] = [==[INT64]==],
			},
			{
				["extensions"] = {
					[==[NV_gpu_affinity]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
				},
				["name"] = [==[DeleteDCNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_swap_group]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[group]==],
						["ctype"] = [==[GLuint]==],
					},
				},
				["name"] = [==[JoinSwapGroupNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[OML_sync_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[numerator]==],
						["ctype"] = [==[INT32 *]==],
					},
					{
						["name"] = [==[denominator]==],
						["ctype"] = [==[INT32 *]==],
					},
				},
				["name"] = [==[GetMscRateOML]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_DX_interop]==],
				},
				["parameters"] = {
					{
						["name"] = [==[dxDevice]==],
						["ctype"] = [==[void *]==],
					},
				},
				["name"] = [==[DXOpenDeviceNV]==],
				["return_ctype"] = [==[HANDLE]==],
			},
			{
				["extensions"] = {
					[==[EXT_pixel_format]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iPixelFormat]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[iLayerPlane]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[nAttributes]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[piAttributes]==],
						["ctype"] = [==[int *]==],
					},
					{
						["name"] = [==[pfValues]==],
						["ctype"] = [==[FLOAT *]==],
					},
				},
				["name"] = [==[GetPixelFormatAttribfvEXT]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_DX_interop]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDevice]==],
						["ctype"] = [==[HANDLE]==],
					},
				},
				["name"] = [==[DXCloseDeviceNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_video_output]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFERARB]==],
					},
					{
						["name"] = [==[iVideoBuffer]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[ReleaseVideoImageNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[OML_sync_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[target_sbc]==],
						["ctype"] = [==[INT64]==],
					},
					{
						["name"] = [==[ust]==],
						["ctype"] = [==[INT64 *]==],
					},
					{
						["name"] = [==[msc]==],
						["ctype"] = [==[INT64 *]==],
					},
					{
						["name"] = [==[sbc]==],
						["ctype"] = [==[INT64 *]==],
					},
				},
				["name"] = [==[WaitForSbcOML]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[ARB_pixel_format]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iPixelFormat]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[iLayerPlane]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[nAttributes]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[piAttributes]==],
						["ctype"] = [==[const int *]==],
					},
					{
						["name"] = [==[piValues]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[GetPixelFormatAttribivARB]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_video_output]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hVideoDevice]==],
						["ctype"] = [==[HPVIDEODEV]==],
					},
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFERARB]==],
					},
					{
						["name"] = [==[iVideoBuffer]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[BindVideoImageNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_video_output]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hVideoDevice]==],
						["ctype"] = [==[HPVIDEODEV]==],
					},
				},
				["name"] = [==[ReleaseVideoDeviceNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_present_video]==],
				},
				["parameters"] = {
					{
						["name"] = [==[iAttribute]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[piValue]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[QueryCurrentContextNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_video_output]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[numDevices]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[hVideoDevice]==],
						["ctype"] = [==[HPVIDEODEV *]==],
					},
				},
				["name"] = [==[GetVideoDeviceNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_present_video]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[uVideoSlot]==],
						["ctype"] = [==[unsigned int]==],
					},
					{
						["name"] = [==[hVideoDevice]==],
						["ctype"] = [==[HVIDEOOUTPUTDEVICENV]==],
					},
					{
						["name"] = [==[piAttribList]==],
						["ctype"] = [==[const int *]==],
					},
				},
				["name"] = [==[BindVideoDeviceNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[3DL_stereo_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[uState]==],
						["ctype"] = [==[UINT]==],
					},
				},
				["name"] = [==[SetStereoEmitterState3DL]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_swap_frame_usage]==],
				},
				["parameters"] = {
					{
						["name"] = [==[pUsage]==],
						["ctype"] = [==[float *]==],
					},
				},
				["name"] = [==[GetFrameUsageI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_gpu_affinity]==],
				},
				["parameters"] = {
					{
						["name"] = [==[iGpuIndex]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[phGpu]==],
						["ctype"] = [==[HGPUNV *]==],
					},
				},
				["name"] = [==[EnumGpusNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_swap_frame_lock]==],
				},
				["parameters"] = {
				},
				["name"] = [==[DisableFrameLockI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_swap_frame_usage]==],
				},
				["parameters"] = {
					{
						["name"] = [==[pFrameCount]==],
						["ctype"] = [==[DWORD *]==],
					},
					{
						["name"] = [==[pMissedFrames]==],
						["ctype"] = [==[DWORD *]==],
					},
					{
						["name"] = [==[pLastMissedUsage]==],
						["ctype"] = [==[float *]==],
					},
				},
				["name"] = [==[QueryFrameTrackingI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_swap_frame_lock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[pFlag]==],
						["ctype"] = [==[BOOL *]==],
					},
				},
				["name"] = [==[IsEnabledFrameLockI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[EXT_display_color_table]==],
				},
				["parameters"] = {
					{
						["name"] = [==[id]==],
						["ctype"] = [==[GLushort]==],
					},
				},
				["name"] = [==[DestroyDisplayColorTableEXT]==],
				["return_ctype"] = [==[VOID]==],
			},
			{
				["extensions"] = {
					[==[I3D_digital_video_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iAttribute]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[piValue]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[GetDigitalVideoParametersI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_swap_frame_lock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[pFlag]==],
						["ctype"] = [==[BOOL *]==],
					},
				},
				["name"] = [==[QueryFrameLockMasterI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_swap_frame_usage]==],
				},
				["parameters"] = {
				},
				["name"] = [==[BeginFrameTrackingI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[ARB_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFERARB]==],
					},
				},
				["name"] = [==[GetPbufferDCARB]==],
				["return_ctype"] = [==[HDC]==],
			},
			{
				["extensions"] = {
					[==[EXT_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFEREXT]==],
					},
				},
				["name"] = [==[GetPbufferDCEXT]==],
				["return_ctype"] = [==[HDC]==],
			},
			{
				["extensions"] = {
					[==[I3D_swap_frame_lock]==],
				},
				["parameters"] = {
				},
				["name"] = [==[EnableFrameLockI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[ARB_pixel_format]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[piAttribIList]==],
						["ctype"] = [==[const int *]==],
					},
					{
						["name"] = [==[pfAttribFList]==],
						["ctype"] = [==[const FLOAT *]==],
					},
					{
						["name"] = [==[nMaxFormats]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[piFormats]==],
						["ctype"] = [==[int *]==],
					},
					{
						["name"] = [==[nNumFormats]==],
						["ctype"] = [==[UINT *]==],
					},
				},
				["name"] = [==[ChoosePixelFormatARB]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[ARB_buffer_region]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hRegion]==],
						["ctype"] = [==[HANDLE]==],
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
				["name"] = [==[SaveBufferRegionARB]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[ARB_render_texture]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFERARB]==],
					},
					{
						["name"] = [==[iBuffer]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[BindTexImageARB]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_DX_interop]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDevice]==],
						["ctype"] = [==[HANDLE]==],
					},
					{
						["name"] = [==[count]==],
						["ctype"] = [==[GLint]==],
					},
					{
						["name"] = [==[hObjects]==],
						["ctype"] = [==[HANDLE *]==],
					},
				},
				["name"] = [==[DXLockObjectsNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_genlock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
				},
				["name"] = [==[DisableGenlockI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[EXT_display_color_table]==],
				},
				["parameters"] = {
					{
						["name"] = [==[id]==],
						["ctype"] = [==[GLushort]==],
					},
				},
				["name"] = [==[BindDisplayColorTableEXT]==],
				["return_ctype"] = [==[GLboolean]==],
			},
			{
				["extensions"] = {
					[==[EXT_pixel_format]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iPixelFormat]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[iLayerPlane]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[nAttributes]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[piAttributes]==],
						["ctype"] = [==[int *]==],
					},
					{
						["name"] = [==[piValues]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[GetPixelFormatAttribivEXT]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_genlock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[uSource]==],
						["ctype"] = [==[UINT *]==],
					},
				},
				["name"] = [==[GetGenlockSourceI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_image_buffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[pAddress]==],
						["ctype"] = [==[LPVOID]==],
					},
				},
				["name"] = [==[DestroyImageBufferI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_image_buffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[dwSize]==],
						["ctype"] = [==[DWORD]==],
					},
					{
						["name"] = [==[uFlags]==],
						["ctype"] = [==[UINT]==],
					},
				},
				["name"] = [==[CreateImageBufferI3D]==],
				["return_ctype"] = [==[LPVOID]==],
			},
			{
				["extensions"] = {
					[==[I3D_genlock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[uDelay]==],
						["ctype"] = [==[UINT *]==],
					},
				},
				["name"] = [==[GetGenlockSourceDelayI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_genlock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[uRate]==],
						["ctype"] = [==[UINT]==],
					},
				},
				["name"] = [==[GenlockSampleRateI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_genlock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[uEdge]==],
						["ctype"] = [==[UINT *]==],
					},
				},
				["name"] = [==[GetGenlockSourceEdgeI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[ARB_buffer_region]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iLayerPlane]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[uType]==],
						["ctype"] = [==[UINT]==],
					},
				},
				["name"] = [==[CreateBufferRegionARB]==],
				["return_ctype"] = [==[HANDLE]==],
			},
			{
				["extensions"] = {
					[==[I3D_gamma]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[iEntries]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[puRed]==],
						["ctype"] = [==[const USHORT *]==],
					},
					{
						["name"] = [==[puGreen]==],
						["ctype"] = [==[const USHORT *]==],
					},
					{
						["name"] = [==[puBlue]==],
						["ctype"] = [==[const USHORT *]==],
					},
				},
				["name"] = [==[SetGammaTableI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[ARB_render_texture]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFERARB]==],
					},
					{
						["name"] = [==[piAttribList]==],
						["ctype"] = [==[const int *]==],
					},
				},
				["name"] = [==[SetPbufferAttribARB]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[NV_video_capture]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[hDevice]==],
						["ctype"] = [==[HVIDEOINPUTDEVICENV]==],
					},
					{
						["name"] = [==[iAttribute]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[piValue]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[QueryVideoCaptureDeviceNV]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[EXT_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFEREXT]==],
					},
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
				},
				["name"] = [==[ReleasePbufferDCEXT]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[ARB_buffer_region]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hRegion]==],
						["ctype"] = [==[HANDLE]==],
					},
				},
				["name"] = [==[DeleteBufferRegionARB]==],
				["return_ctype"] = [==[VOID]==],
			},
			{
				["extensions"] = {
					[==[OML_sync_control]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[target_msc]==],
						["ctype"] = [==[INT64]==],
					},
					{
						["name"] = [==[divisor]==],
						["ctype"] = [==[INT64]==],
					},
					{
						["name"] = [==[remainder]==],
						["ctype"] = [==[INT64]==],
					},
				},
				["name"] = [==[SwapBuffersMscOML]==],
				["return_ctype"] = [==[INT64]==],
			},
			{
				["extensions"] = {
					[==[ARB_extensions_string]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
				},
				["name"] = [==[GetExtensionsStringARB]==],
				["return_ctype"] = [==[const char *]==],
			},
			{
				["extensions"] = {
					[==[EXT_make_current_read]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDrawDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[hReadDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[hglrc]==],
						["ctype"] = [==[HGLRC]==],
					},
				},
				["name"] = [==[MakeContextCurrentEXT]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[EXT_make_current_read]==],
				},
				["parameters"] = {
				},
				["name"] = [==[GetCurrentReadDCEXT]==],
				["return_ctype"] = [==[HDC]==],
			},
			{
				["extensions"] = {
					[==[ARB_make_current_read]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDrawDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[hReadDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[hglrc]==],
						["ctype"] = [==[HGLRC]==],
					},
				},
				["name"] = [==[MakeContextCurrentARB]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[EXT_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFEREXT]==],
					},
				},
				["name"] = [==[DestroyPbufferEXT]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[ARB_buffer_region]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hRegion]==],
						["ctype"] = [==[HANDLE]==],
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
					{
						["name"] = [==[xSrc]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[ySrc]==],
						["ctype"] = [==[int]==],
					},
				},
				["name"] = [==[RestoreBufferRegionARB]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[EXT_swap_control]==],
				},
				["parameters"] = {
				},
				["name"] = [==[GetSwapIntervalEXT]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[EXT_pixel_format]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hdc]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[piAttribIList]==],
						["ctype"] = [==[const int *]==],
					},
					{
						["name"] = [==[pfAttribFList]==],
						["ctype"] = [==[const FLOAT *]==],
					},
					{
						["name"] = [==[nMaxFormats]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[piFormats]==],
						["ctype"] = [==[int *]==],
					},
					{
						["name"] = [==[nNumFormats]==],
						["ctype"] = [==[UINT *]==],
					},
				},
				["name"] = [==[ChoosePixelFormatEXT]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[I3D_genlock]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[pFlag]==],
						["ctype"] = [==[BOOL *]==],
					},
				},
				["name"] = [==[IsEnabledGenlockI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[ARB_create_context]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[hShareContext]==],
						["ctype"] = [==[HGLRC]==],
					},
					{
						["name"] = [==[attribList]==],
						["ctype"] = [==[const int *]==],
					},
				},
				["name"] = [==[CreateContextAttribsARB]==],
				["return_ctype"] = [==[HGLRC]==],
			},
			{
				["extensions"] = {
					[==[ARB_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFERARB]==],
					},
				},
				["name"] = [==[DestroyPbufferARB]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[AMD_gpu_association]==],
				},
				["parameters"] = {
					{
						["name"] = [==[id]==],
						["ctype"] = [==[UINT]==],
					},
					{
						["name"] = [==[hShareContext]==],
						["ctype"] = [==[HGLRC]==],
					},
					{
						["name"] = [==[attribList]==],
						["ctype"] = [==[const int *]==],
					},
				},
				["name"] = [==[CreateAssociatedContextAttribsAMD]==],
				["return_ctype"] = [==[HGLRC]==],
			},
			{
				["extensions"] = {
					[==[EXT_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFEREXT]==],
					},
					{
						["name"] = [==[iAttribute]==],
						["ctype"] = [==[int]==],
					},
					{
						["name"] = [==[piValue]==],
						["ctype"] = [==[int *]==],
					},
				},
				["name"] = [==[QueryPbufferEXT]==],
				["return_ctype"] = [==[BOOL]==],
			},
			{
				["extensions"] = {
					[==[ARB_pbuffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hPbuffer]==],
						["ctype"] = [==[HPBUFFERARB]==],
					},
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
				},
				["name"] = [==[ReleasePbufferDCARB]==],
				["return_ctype"] = [==[int]==],
			},
			{
				["extensions"] = {
					[==[I3D_image_buffer]==],
				},
				["parameters"] = {
					{
						["name"] = [==[hDC]==],
						["ctype"] = [==[HDC]==],
					},
					{
						["name"] = [==[pEvent]==],
						["ctype"] = [==[const HANDLE *]==],
					},
					{
						["name"] = [==[pAddress]==],
						["ctype"] = [==[const LPVOID *]==],
					},
					{
						["name"] = [==[pSize]==],
						["ctype"] = [==[const DWORD *]==],
					},
					{
						["name"] = [==[count]==],
						["ctype"] = [==[UINT]==],
					},
				},
				["name"] = [==[AssociateImageBufferEventsI3D]==],
				["return_ctype"] = [==[BOOL]==],
			},
		},
	},
	["extensions"] = {
		[==[ARB_buffer_region]==],
		[==[ARB_multisample]==],
		[==[ARB_extensions_string]==],
		[==[ARB_pixel_format]==],
		[==[ARB_make_current_read]==],
		[==[ARB_pbuffer]==],
		[==[ARB_render_texture]==],
		[==[ARB_pixel_format_float]==],
		[==[ARB_framebuffer_sRGB]==],
		[==[ARB_create_context]==],
		[==[ARB_create_context_profile]==],
		[==[ARB_create_context_robustness]==],
		[==[ARB_robustness_application_isolation]==],
		[==[ARB_robustness_share_group_isolation]==],
		[==[EXT_display_color_table]==],
		[==[EXT_extensions_string]==],
		[==[EXT_make_current_read]==],
		[==[EXT_pbuffer]==],
		[==[EXT_pixel_format]==],
		[==[EXT_swap_control]==],
		[==[EXT_depth_float]==],
		[==[NV_vertex_array_range]==],
		[==[3DFX_multisample]==],
		[==[EXT_multisample]==],
		[==[OML_sync_control]==],
		[==[I3D_digital_video_control]==],
		[==[I3D_gamma]==],
		[==[I3D_genlock]==],
		[==[I3D_image_buffer]==],
		[==[I3D_swap_frame_lock]==],
		[==[I3D_swap_frame_usage]==],
		[==[ATI_pixel_format_float]==],
		[==[NV_float_buffer]==],
		[==[3DL_stereo_control]==],
		[==[EXT_pixel_format_packed_float]==],
		[==[EXT_framebuffer_sRGB]==],
		[==[NV_present_video]==],
		[==[NV_video_output]==],
		[==[NV_render_depth_texture]==],
		[==[NV_render_texture_rectangle]==],
		[==[NV_swap_group]==],
		[==[NV_gpu_affinity]==],
		[==[AMD_gpu_association]==],
		[==[NV_video_capture]==],
		[==[NV_copy_image]==],
		[==[NV_multisample_coverage]==],
		[==[EXT_create_context_es_profile]==],
		[==[EXT_create_context_es2_profile]==],
		[==[NV_DX_interop]==],
		[==[NV_DX_interop2]==],
		[==[EXT_swap_control_tear]==],
	},
	["enumerators"] = {
		{
			["value"] = [==[0x2012]==],
			["name"] = [==[STEREO_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2043]==],
			["name"] = [==[ERROR_INVALID_PIXEL_TYPE_EXT]==],
			["extensions"] = {
				[==[EXT_make_current_read]==],
			},
		},
		{
			["value"] = [==[0x2040]==],
			["name"] = [==[DEPTH_FLOAT_EXT]==],
			["extensions"] = {
				[==[EXT_depth_float]==],
			},
		},
		{
			["value"] = [==[0x2024]==],
			["name"] = [==[AUX_BUFFERS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2017]==],
			["name"] = [==[GREEN_BITS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2095]==],
			["name"] = [==[ERROR_INVALID_VERSION_ARB]==],
			["extensions"] = {
				[==[ARB_create_context]==],
			},
		},
		{
			["value"] = [==[0x00000001]==],
			["name"] = [==[ACCESS_READ_WRITE_NV]==],
			["extensions"] = {
				[==[NV_DX_interop]==],
			},
		},
		{
			["value"] = [==[0x2084]==],
			["name"] = [==[FRONT_RIGHT_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x207A]==],
			["name"] = [==[TEXTURE_2D_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x20A9]==],
			["name"] = [==[FRAMEBUFFER_SRGB_CAPABLE_EXT]==],
			["extensions"] = {
				[==[EXT_framebuffer_sRGB]==],
			},
		},
		{
			["value"] = [==[0x203B]==],
			["name"] = [==[TRANSPARENT_INDEX_VALUE_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2029]==],
			["name"] = [==[SWAP_COPY_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x20CB]==],
			["name"] = [==[VIDEO_OUT_STACKED_FIELDS_1_2]==],
			["extensions"] = {
				[==[NV_video_output]==],
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
			["value"] = [==[0x2061]==],
			["name"] = [==[SAMPLES_3DFX]==],
			["extensions"] = {
				[==[3DFX_multisample]==],
			},
		},
		{
			["value"] = [==[0x00000000]==],
			["name"] = [==[ACCESS_READ_ONLY_NV]==],
			["extensions"] = {
				[==[NV_DX_interop]==],
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
			["value"] = [==[0x2005]==],
			["name"] = [==[NEED_SYSTEM_PALETTE_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2035]==],
			["name"] = [==[PBUFFER_HEIGHT_ARB]==],
			["extensions"] = {
				[==[ARB_pbuffer]==],
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
			["value"] = [==[0x2019]==],
			["name"] = [==[BLUE_BITS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
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
			["value"] = [==[0x20B9]==],
			["name"] = [==[COLOR_SAMPLES_NV]==],
			["extensions"] = {
				[==[NV_multisample_coverage]==],
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
			["value"] = [==[0x2087]==],
			["name"] = [==[AUX0_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x2025]==],
			["name"] = [==[NO_ACCELERATION_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x202E]==],
			["name"] = [==[MAX_PBUFFER_PIXELS_ARB]==],
			["extensions"] = {
				[==[ARB_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x201C]==],
			["name"] = [==[ALPHA_SHIFT_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2048]==],
			["name"] = [==[GENLOCK_SOURCE_DIGITAL_SYNC_I3D]==],
			["extensions"] = {
				[==[I3D_genlock]==],
			},
		},
		{
			["value"] = [==[0x20B1]==],
			["name"] = [==[BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV]==],
			["extensions"] = {
				[==[NV_float_buffer]==],
			},
		},
		{
			["value"] = [==[0x2042]==],
			["name"] = [==[COVERAGE_SAMPLES_NV]==],
			["extensions"] = {
				[==[NV_multisample_coverage]==],
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
			["value"] = [==[0x2032]==],
			["name"] = [==[OPTIMAL_PBUFFER_HEIGHT_EXT]==],
			["extensions"] = {
				[==[EXT_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x2012]==],
			["name"] = [==[STEREO_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
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
			["value"] = [==[0x2014]==],
			["name"] = [==[COLOR_BITS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2038]==],
			["name"] = [==[TRANSPARENT_GREEN_VALUE_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x00000001]==],
			["name"] = [==[IMAGE_BUFFER_MIN_ACCESS_I3D]==],
			["extensions"] = {
				[==[I3D_image_buffer]==],
			},
		},
		{
			["value"] = [==[0x2025]==],
			["name"] = [==[NO_ACCELERATION_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2073]==],
			["name"] = [==[TEXTURE_TARGET_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x208D]==],
			["name"] = [==[AUX6_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x2033]==],
			["name"] = [==[PBUFFER_LARGEST_EXT]==],
			["extensions"] = {
				[==[EXT_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x2034]==],
			["name"] = [==[PBUFFER_WIDTH_ARB]==],
			["extensions"] = {
				[==[ARB_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x20A4]==],
			["name"] = [==[BIND_TO_TEXTURE_RECTANGLE_DEPTH_NV]==],
			["extensions"] = {
				[==[NV_render_depth_texture]==],
			},
		},
		{
			["value"] = [==[0x20B2]==],
			["name"] = [==[BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV]==],
			["extensions"] = {
				[==[NV_float_buffer]==],
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
			["value"] = [==[0x00000004]==],
			["name"] = [==[CONTEXT_ES2_PROFILE_BIT_EXT]==],
			["extensions"] = {
				[==[EXT_create_context_es2_profile]==],
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
			["value"] = [==[0x2014]==],
			["name"] = [==[COLOR_BITS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2017]==],
			["name"] = [==[GREEN_BITS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
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
			["value"] = [==[0x1F02]==],
			["name"] = [==[GPU_OPENGL_VERSION_STRING_AMD]==],
			["extensions"] = {
				[==[AMD_gpu_association]==],
			},
		},
		{
			["value"] = [==[0x2076]==],
			["name"] = [==[TEXTURE_RGBA_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
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
			["value"] = [==[0x2070]==],
			["name"] = [==[BIND_TO_TEXTURE_RGB_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x20D1]==],
			["name"] = [==[ERROR_MISSING_AFFINITY_MASK_NV]==],
			["extensions"] = {
				[==[NV_gpu_affinity]==],
			},
		},
		{
			["value"] = [==[0x20D0]==],
			["name"] = [==[ERROR_INCOMPATIBLE_AFFINITY_MASKS_NV]==],
			["extensions"] = {
				[==[NV_gpu_affinity]==],
			},
		},
		{
			["value"] = [==[0x20B6]==],
			["name"] = [==[TEXTURE_FLOAT_RG_NV]==],
			["extensions"] = {
				[==[NV_float_buffer]==],
			},
		},
		{
			["value"] = [==[0x20A2]==],
			["name"] = [==[TEXTURE_RECTANGLE_NV]==],
			["extensions"] = {
				[==[NV_render_texture_rectangle]==],
			},
		},
		{
			["value"] = [==[0x2009]==],
			["name"] = [==[NUMBER_UNDERLAYS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x202C]==],
			["name"] = [==[TYPE_COLORINDEX_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
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
			["value"] = [==[0x9126]==],
			["name"] = [==[CONTEXT_PROFILE_MASK_ARB]==],
			["extensions"] = {
				[==[ARB_create_context_profile]==],
			},
		},
		{
			["value"] = [==[0x20A1]==],
			["name"] = [==[BIND_TO_TEXTURE_RECTANGLE_RGBA_NV]==],
			["extensions"] = {
				[==[NV_render_texture_rectangle]==],
			},
		},
		{
			["value"] = [==[0x2006]==],
			["name"] = [==[SWAP_LAYER_BUFFERS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2042]==],
			["name"] = [==[SAMPLES_EXT]==],
			["extensions"] = {
				[==[EXT_multisample]==],
			},
		},
		{
			["value"] = [==[0x2002]==],
			["name"] = [==[DRAW_TO_BITMAP_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2096]==],
			["name"] = [==[ERROR_INVALID_PROFILE_ARB]==],
			["extensions"] = {
				[==[ARB_create_context_profile]==],
			},
		},
		{
			["value"] = [==[0x2002]==],
			["name"] = [==[DRAW_TO_BITMAP_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x20A0]==],
			["name"] = [==[BIND_TO_TEXTURE_RECTANGLE_RGB_NV]==],
			["extensions"] = {
				[==[NV_render_texture_rectangle]==],
			},
		},
		{
			["value"] = [==[0x2027]==],
			["name"] = [==[FULL_ACCELERATION_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2046]==],
			["name"] = [==[GENLOCK_SOURCE_EXTERNAL_FIELD_I3D]==],
			["extensions"] = {
				[==[I3D_genlock]==],
			},
		},
		{
			["value"] = [==[0x2013]==],
			["name"] = [==[PIXEL_TYPE_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x20A7]==],
			["name"] = [==[DEPTH_COMPONENT_NV]==],
			["extensions"] = {
				[==[NV_render_depth_texture]==],
			},
		},
		{
			["value"] = [==[0x2000]==],
			["name"] = [==[NUMBER_PIXEL_FORMATS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x202D]==],
			["name"] = [==[DRAW_TO_PBUFFER_EXT]==],
			["extensions"] = {
				[==[EXT_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x2020]==],
			["name"] = [==[ACCUM_BLUE_BITS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2043]==],
			["name"] = [==[ERROR_INVALID_PIXEL_TYPE_ARB]==],
			["extensions"] = {
				[==[ARB_make_current_read]==],
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
			["value"] = [==[0x201C]==],
			["name"] = [==[ALPHA_SHIFT_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x201E]==],
			["name"] = [==[ACCUM_RED_BITS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2081]==],
			["name"] = [==[TEXTURE_CUBE_MAP_POSITIVE_Z_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x20B5]==],
			["name"] = [==[TEXTURE_FLOAT_R_NV]==],
			["extensions"] = {
				[==[NV_float_buffer]==],
			},
		},
		{
			["value"] = [==[0x204C]==],
			["name"] = [==[GENLOCK_SOURCE_EDGE_BOTH_I3D]==],
			["extensions"] = {
				[==[I3D_genlock]==],
			},
		},
		{
			["value"] = [==[0x2018]==],
			["name"] = [==[GREEN_SHIFT_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x20A5]==],
			["name"] = [==[DEPTH_TEXTURE_FORMAT_NV]==],
			["extensions"] = {
				[==[NV_render_depth_texture]==],
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
			["value"] = [==[0x20A3]==],
			["name"] = [==[BIND_TO_TEXTURE_DEPTH_NV]==],
			["extensions"] = {
				[==[NV_render_depth_texture]==],
			},
		},
		{
			["value"] = [==[0x00000001]==],
			["name"] = [==[FRONT_COLOR_BUFFER_BIT_ARB]==],
			["extensions"] = {
				[==[ARB_buffer_region]==],
			},
		},
		{
			["value"] = [==[0x207B]==],
			["name"] = [==[MIPMAP_LEVEL_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x201D]==],
			["name"] = [==[ACCUM_BITS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x202A]==],
			["name"] = [==[SWAP_UNDEFINED_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x20CC]==],
			["name"] = [==[VIDEO_OUT_STACKED_FIELDS_2_1]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x201E]==],
			["name"] = [==[ACCUM_RED_BITS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x20CA]==],
			["name"] = [==[VIDEO_OUT_FIELD_2]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x204A]==],
			["name"] = [==[GENLOCK_SOURCE_EDGE_FALLING_I3D]==],
			["extensions"] = {
				[==[I3D_genlock]==],
			},
		},
		{
			["value"] = [==[0x2036]==],
			["name"] = [==[PBUFFER_LOST_ARB]==],
			["extensions"] = {
				[==[ARB_pbuffer]==],
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
			["value"] = [==[0x20C8]==],
			["name"] = [==[VIDEO_OUT_FRAME]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x2033]==],
			["name"] = [==[PBUFFER_LARGEST_ARB]==],
			["extensions"] = {
				[==[ARB_pbuffer]==],
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
			["value"] = [==[0x2075]==],
			["name"] = [==[TEXTURE_RGB_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x2030]==],
			["name"] = [==[MAX_PBUFFER_HEIGHT_EXT]==],
			["extensions"] = {
				[==[EXT_pbuffer]==],
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
			["value"] = [==[0x201F]==],
			["name"] = [==[ACCUM_GREEN_BITS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x202B]==],
			["name"] = [==[TYPE_RGBA_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
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
			["value"] = [==[0x2028]==],
			["name"] = [==[SWAP_EXCHANGE_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x202A]==],
			["name"] = [==[SWAP_UNDEFINED_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2029]==],
			["name"] = [==[SWAP_COPY_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
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
			["value"] = [==[0x00000002]==],
			["name"] = [==[IMAGE_BUFFER_LOCK_I3D]==],
			["extensions"] = {
				[==[I3D_image_buffer]==],
			},
		},
		{
			["value"] = [==[0x2011]==],
			["name"] = [==[DOUBLE_BUFFER_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2015]==],
			["name"] = [==[RED_BITS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x20C1]==],
			["name"] = [==[BIND_TO_VIDEO_RGBA_NV]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x20C0]==],
			["name"] = [==[BIND_TO_VIDEO_RGB_NV]==],
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
			["value"] = [==[0x2018]==],
			["name"] = [==[GREEN_SHIFT_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x200C]==],
			["name"] = [==[SHARE_DEPTH_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2001]==],
			["name"] = [==[DRAW_TO_WINDOW_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2039]==],
			["name"] = [==[TRANSPARENT_BLUE_VALUE_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2020]==],
			["name"] = [==[ACCUM_BLUE_BITS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2042]==],
			["name"] = [==[SAMPLES_ARB]==],
			["extensions"] = {
				[==[ARB_multisample]==],
			},
		},
		{
			["value"] = [==[0x2057]==],
			["name"] = [==[STEREO_POLARITY_NORMAL_3DL]==],
			["extensions"] = {
				[==[3DL_stereo_control]==],
			},
		},
		{
			["value"] = [==[0x2077]==],
			["name"] = [==[NO_TEXTURE_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x2060]==],
			["name"] = [==[SAMPLE_BUFFERS_3DFX]==],
			["extensions"] = {
				[==[3DFX_multisample]==],
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
			["value"] = [==[0x2055]==],
			["name"] = [==[STEREO_EMITTER_ENABLE_3DL]==],
			["extensions"] = {
				[==[3DL_stereo_control]==],
			},
		},
		{
			["value"] = [==[0x2001]==],
			["name"] = [==[DRAW_TO_WINDOW_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
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
			["value"] = [==[0x00000002]==],
			["name"] = [==[ACCESS_WRITE_DISCARD_NV]==],
			["extensions"] = {
				[==[NV_DX_interop]==],
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
			["value"] = [==[0x00000002]==],
			["name"] = [==[BACK_COLOR_BUFFER_BIT_ARB]==],
			["extensions"] = {
				[==[ARB_buffer_region]==],
			},
		},
		{
			["value"] = [==[0x2015]==],
			["name"] = [==[RED_BITS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x20B4]==],
			["name"] = [==[BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV]==],
			["extensions"] = {
				[==[NV_float_buffer]==],
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
			["value"] = [==[0x20B3]==],
			["name"] = [==[BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV]==],
			["extensions"] = {
				[==[NV_float_buffer]==],
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
			["value"] = [==[0x20A6]==],
			["name"] = [==[TEXTURE_DEPTH_COMPONENT_NV]==],
			["extensions"] = {
				[==[NV_render_depth_texture]==],
			},
		},
		{
			["value"] = [==[0x2085]==],
			["name"] = [==[BACK_LEFT_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x2023]==],
			["name"] = [==[STENCIL_BITS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x200D]==],
			["name"] = [==[SHARE_STENCIL_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x21A0]==],
			["name"] = [==[TYPE_RGBA_FLOAT_ATI]==],
			["extensions"] = {
				[==[ATI_pixel_format_float]==],
			},
		},
		{
			["value"] = [==[0x2023]==],
			["name"] = [==[STENCIL_BITS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2071]==],
			["name"] = [==[BIND_TO_TEXTURE_RGBA_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x202F]==],
			["name"] = [==[MAX_PBUFFER_WIDTH_ARB]==],
			["extensions"] = {
				[==[ARB_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x20C2]==],
			["name"] = [==[BIND_TO_VIDEO_RGB_AND_DEPTH_NV]==],
			["extensions"] = {
				[==[NV_video_output]==],
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
			["value"] = [==[0x2031]==],
			["name"] = [==[OPTIMAL_PBUFFER_WIDTH_EXT]==],
			["extensions"] = {
				[==[EXT_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x204B]==],
			["name"] = [==[GENLOCK_SOURCE_EDGE_RISING_I3D]==],
			["extensions"] = {
				[==[I3D_genlock]==],
			},
		},
		{
			["value"] = [==[0x207E]==],
			["name"] = [==[TEXTURE_CUBE_MAP_NEGATIVE_X_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x201B]==],
			["name"] = [==[ALPHA_BITS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2007]==],
			["name"] = [==[SWAP_METHOD_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x20C9]==],
			["name"] = [==[VIDEO_OUT_FIELD_1]==],
			["extensions"] = {
				[==[NV_video_output]==],
			},
		},
		{
			["value"] = [==[0x202C]==],
			["name"] = [==[TYPE_COLORINDEX_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2006]==],
			["name"] = [==[SWAP_LAYER_BUFFERS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
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
			["value"] = [==[0x2021]==],
			["name"] = [==[ACCUM_ALPHA_BITS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2047]==],
			["name"] = [==[GENLOCK_SOURCE_EXTERNAL_TTL_I3D]==],
			["extensions"] = {
				[==[I3D_genlock]==],
			},
		},
		{
			["value"] = [==[0x2045]==],
			["name"] = [==[GENLOCK_SOURCE_EXTERNAL_SYNC_I3D]==],
			["extensions"] = {
				[==[I3D_genlock]==],
			},
		},
		{
			["value"] = [==[0x2083]==],
			["name"] = [==[FRONT_LEFT_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x2044]==],
			["name"] = [==[GENLOCK_SOURCE_MULTIVIEW_I3D]==],
			["extensions"] = {
				[==[I3D_genlock]==],
			},
		},
		{
			["value"] = [==[0x204F]==],
			["name"] = [==[GAMMA_EXCLUDE_DESKTOP_I3D]==],
			["extensions"] = {
				[==[I3D_gamma]==],
			},
		},
		{
			["value"] = [==[0x2078]==],
			["name"] = [==[TEXTURE_CUBE_MAP_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x204E]==],
			["name"] = [==[GAMMA_TABLE_SIZE_I3D]==],
			["extensions"] = {
				[==[I3D_gamma]==],
			},
		},
		{
			["value"] = [==[0x207F]==],
			["name"] = [==[TEXTURE_CUBE_MAP_POSITIVE_Y_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x200A]==],
			["name"] = [==[TRANSPARENT_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x202B]==],
			["name"] = [==[TYPE_RGBA_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2049]==],
			["name"] = [==[GENLOCK_SOURCE_DIGITAL_FIELD_I3D]==],
			["extensions"] = {
				[==[I3D_genlock]==],
			},
		},
		{
			["value"] = [==[0x2007]==],
			["name"] = [==[SWAP_METHOD_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2052]==],
			["name"] = [==[DIGITAL_VIDEO_CURSOR_INCLUDED_I3D]==],
			["extensions"] = {
				[==[I3D_digital_video_control]==],
			},
		},
		{
			["value"] = [==[0x2051]==],
			["name"] = [==[DIGITAL_VIDEO_CURSOR_ALPHA_VALUE_I3D]==],
			["extensions"] = {
				[==[I3D_digital_video_control]==],
			},
		},
		{
			["value"] = [==[0x2050]==],
			["name"] = [==[DIGITAL_VIDEO_CURSOR_ALPHA_FRAMEBUFFER_I3D]==],
			["extensions"] = {
				[==[I3D_digital_video_control]==],
			},
		},
		{
			["value"] = [==[0x202E]==],
			["name"] = [==[MAX_PBUFFER_PIXELS_EXT]==],
			["extensions"] = {
				[==[EXT_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x2041]==],
			["name"] = [==[SAMPLE_BUFFERS_EXT]==],
			["extensions"] = {
				[==[EXT_multisample]==],
			},
		},
		{
			["value"] = [==[0x2056]==],
			["name"] = [==[STEREO_EMITTER_DISABLE_3DL]==],
			["extensions"] = {
				[==[3DL_stereo_control]==],
			},
		},
		{
			["value"] = [==[0x2054]==],
			["name"] = [==[ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB]==],
			["extensions"] = {
				[==[ARB_make_current_read]==],
			},
		},
		{
			["value"] = [==[0x2028]==],
			["name"] = [==[SWAP_EXCHANGE_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x208F]==],
			["name"] = [==[AUX8_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x200E]==],
			["name"] = [==[SHARE_ACCUM_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2024]==],
			["name"] = [==[AUX_BUFFERS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2037]==],
			["name"] = [==[TRANSPARENT_RED_VALUE_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2010]==],
			["name"] = [==[SUPPORT_OPENGL_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x200E]==],
			["name"] = [==[SHARE_ACCUM_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2022]==],
			["name"] = [==[DEPTH_BITS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x201A]==],
			["name"] = [==[BLUE_SHIFT_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2021]==],
			["name"] = [==[ACCUM_ALPHA_BITS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x207D]==],
			["name"] = [==[TEXTURE_CUBE_MAP_POSITIVE_X_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x2058]==],
			["name"] = [==[STEREO_POLARITY_INVERT_3DL]==],
			["extensions"] = {
				[==[3DL_stereo_control]==],
			},
		},
		{
			["value"] = [==[0x00000001]==],
			["name"] = [==[CONTEXT_CORE_PROFILE_BIT_ARB]==],
			["extensions"] = {
				[==[ARB_create_context_profile]==],
			},
		},
		{
			["value"] = [==[0x2034]==],
			["name"] = [==[PBUFFER_WIDTH_EXT]==],
			["extensions"] = {
				[==[EXT_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x2041]==],
			["name"] = [==[SAMPLE_BUFFERS_ARB]==],
			["extensions"] = {
				[==[ARB_multisample]==],
			},
		},
		{
			["value"] = [==[0x208C]==],
			["name"] = [==[AUX5_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x20B7]==],
			["name"] = [==[TEXTURE_FLOAT_RGB_NV]==],
			["extensions"] = {
				[==[NV_float_buffer]==],
			},
		},
		{
			["value"] = [==[0x201A]==],
			["name"] = [==[BLUE_SHIFT_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2089]==],
			["name"] = [==[AUX2_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x202D]==],
			["name"] = [==[DRAW_TO_PBUFFER_ARB]==],
			["extensions"] = {
				[==[ARB_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x2027]==],
			["name"] = [==[FULL_ACCELERATION_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2019]==],
			["name"] = [==[BLUE_BITS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2022]==],
			["name"] = [==[DEPTH_BITS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x20A8]==],
			["name"] = [==[TYPE_RGBA_UNSIGNED_FLOAT_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format_packed_float]==],
			},
		},
		{
			["value"] = [==[0x2008]==],
			["name"] = [==[NUMBER_OVERLAYS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2004]==],
			["name"] = [==[NEED_PALETTE_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2016]==],
			["name"] = [==[RED_SHIFT_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
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
			["value"] = [==[0x00000008]==],
			["name"] = [==[STENCIL_BUFFER_BIT_ARB]==],
			["extensions"] = {
				[==[ARB_buffer_region]==],
			},
		},
		{
			["value"] = [==[0x2010]==],
			["name"] = [==[SUPPORT_OPENGL_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x200F]==],
			["name"] = [==[SUPPORT_GDI_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x201D]==],
			["name"] = [==[ACCUM_BITS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x208A]==],
			["name"] = [==[AUX3_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x2004]==],
			["name"] = [==[NEED_PALETTE_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x203A]==],
			["name"] = [==[TRANSPARENT_ALPHA_VALUE_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2026]==],
			["name"] = [==[GENERIC_ACCELERATION_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2005]==],
			["name"] = [==[NEED_SYSTEM_PALETTE_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2013]==],
			["name"] = [==[PIXEL_TYPE_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2072]==],
			["name"] = [==[TEXTURE_FORMAT_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x00000004]==],
			["name"] = [==[DEPTH_BUFFER_BIT_ARB]==],
			["extensions"] = {
				[==[ARB_buffer_region]==],
			},
		},
		{
			["value"] = [==[0x200C]==],
			["name"] = [==[SHARE_DEPTH_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2035]==],
			["name"] = [==[PBUFFER_HEIGHT_EXT]==],
			["extensions"] = {
				[==[EXT_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x2080]==],
			["name"] = [==[TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x2093]==],
			["name"] = [==[CONTEXT_LAYER_PLANE_ARB]==],
			["extensions"] = {
				[==[ARB_create_context]==],
			},
		},
		{
			["value"] = [==[0x2009]==],
			["name"] = [==[NUMBER_UNDERLAYS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2008]==],
			["name"] = [==[NUMBER_OVERLAYS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2053]==],
			["name"] = [==[DIGITAL_VIDEO_GAMMA_CORRECTED_I3D]==],
			["extensions"] = {
				[==[I3D_digital_video_control]==],
			},
		},
		{
			["value"] = [==[0x2003]==],
			["name"] = [==[ACCELERATION_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2086]==],
			["name"] = [==[BACK_RIGHT_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x200B]==],
			["name"] = [==[TRANSPARENT_VALUE_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2000]==],
			["name"] = [==[NUMBER_PIXEL_FORMATS_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x20A9]==],
			["name"] = [==[FRAMEBUFFER_SRGB_CAPABLE_ARB]==],
			["extensions"] = {
				[==[ARB_framebuffer_sRGB]==],
			},
		},
		{
			["value"] = [==[0x202F]==],
			["name"] = [==[MAX_PBUFFER_WIDTH_EXT]==],
			["extensions"] = {
				[==[EXT_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x2011]==],
			["name"] = [==[DOUBLE_BUFFER_EXT]==],
			["extensions"] = {
				[==[EXT_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2030]==],
			["name"] = [==[MAX_PBUFFER_HEIGHT_ARB]==],
			["extensions"] = {
				[==[ARB_pbuffer]==],
			},
		},
		{
			["value"] = [==[0x2082]==],
			["name"] = [==[TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x201F]==],
			["name"] = [==[ACCUM_GREEN_BITS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x208B]==],
			["name"] = [==[AUX4_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x20B8]==],
			["name"] = [==[TEXTURE_FLOAT_RGBA_NV]==],
			["extensions"] = {
				[==[NV_float_buffer]==],
			},
		},
		{
			["value"] = [==[0x207C]==],
			["name"] = [==[CUBE_MAP_FACE_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x200A]==],
			["name"] = [==[TRANSPARENT_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
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
			["value"] = [==[0x21A0]==],
			["name"] = [==[TYPE_RGBA_FLOAT_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format_float]==],
			},
		},
		{
			["value"] = [==[0x2016]==],
			["name"] = [==[RED_SHIFT_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2090]==],
			["name"] = [==[AUX9_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x208E]==],
			["name"] = [==[AUX7_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x200D]==],
			["name"] = [==[SHARE_STENCIL_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2088]==],
			["name"] = [==[AUX1_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x2074]==],
			["name"] = [==[MIPMAP_TEXTURE_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x200F]==],
			["name"] = [==[SUPPORT_GDI_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2079]==],
			["name"] = [==[TEXTURE_1D_ARB]==],
			["extensions"] = {
				[==[ARB_render_texture]==],
			},
		},
		{
			["value"] = [==[0x2003]==],
			["name"] = [==[ACCELERATION_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x2026]==],
			["name"] = [==[GENERIC_ACCELERATION_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
		{
			["value"] = [==[0x201B]==],
			["name"] = [==[ALPHA_BITS_ARB]==],
			["extensions"] = {
				[==[ARB_pixel_format]==],
			},
		},
	},
};
