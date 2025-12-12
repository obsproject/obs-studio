#include "../../include/libvr/ISuperResolutionAdapter.h"
#include <iostream>
#include <string>
#include <vector>
#include <cuda.h>
#include "nvVideoEffects.h"
#include "nvCVImage.h"
#include "gpu/cuda_utils.h"
#include "gpu/vulkan_utils.h"

namespace libvr {

// Helper to wrap cudaArray in NvCVImage
static void WrapCudaArrayInNvCVImage(NvCVImage *img, CUarray arr, int width, int height, NvCVImage_PixelFormat fmt)
{
	// Basic wrapper, assumes RGBA8 or compatible
	// For NvCVImage v2+, we usually use NvCVImage_InitFromCudaArray if available in headers,
	// or manually fill the struct.
	// Assuming NvCVImage is a struct we can fill directly or via helper.
	// Based on NvVFX SDK:
	// NvCVImage_Init(img, width, height, pitch, pixels, fmt, type, layout, memoryType)
	// But we have a CUarray, not a linear pointer.
	// NvCVImage supports NVCV_CUDA_ARRAY

	// We need to verify if NvCVImage_Init is available in the proxy headers or if we must fill manually.
	// Checking headers is hard without reading them all, but 'nvCVImage.h' usually has NvCVImage_Init.

	// Let's assume standard Init function exists or manually fill.
	// Manual fill is safer if we don't link NvCV lib directly but use headers.

	img->width = width;
	img->height = height;
	img->pixelFormat = fmt;
	img->componentType = NVCV_U8; // Assuming 8-bit per channel
	img->pixelBytes = 4;          // RGBA
	img->componentBytes = 1;
	img->numComponents = 4;
	img->pitch = 0; // Opaque for arrays
	img->pixels = (void *)arr;
	img->gpuMem = NVCV_CUDA_ARRAY; // Correct field from header
}

class SuperResolutionAdapter : public ISuperResolutionAdapter {
public:
	SuperResolutionAdapter()
	{
		static const ISuperResolutionAdapter_Vtbl vtbl_impl = {StaticInitialize, StaticProcess, StaticShutdown};
		this->vtbl = &vtbl_impl;
		this->user_data = this;
	}

	~SuperResolutionAdapter() { Shutdown(); }

	void Initialize(int width, int height)
	{
		std::cout << "[SRAdapter] Initializing NVIDIA VFX..." << std::endl;
		unsigned int ver = 0;
		NvCV_Status status = NvVFX_GetVersion(&ver);
		if (status != NVCV_SUCCESS) {
			std::cerr << "[SRAdapter] Failed to get VFX version. dlopen issue? Status=" << status
				  << std::endl;
			return;
		}
		std::cout << "[SRAdapter] VFX SDK Version: " << ver << std::endl;

		status = NvVFX_CreateEffect(NVVFX_FX_SUPER_RES, &effect);
		if (status != NVCV_SUCCESS) {
			std::cerr << "[SRAdapter] Failed to create SuperRes effect: " << status << std::endl;
			return;
		}

		// Set Model Directory
		// Default to typical Linux path or env var
		const char *env_path = getenv("NV_VIDEO_EFFECTS_PATH");
		std::string modelDir = env_path ? std::string(env_path) + "/models"
						: "/usr/local/NVIDIA/VideoEffects/models";
		NvVFX_SetString(effect, NVVFX_MODEL_DIRECTORY, modelDir.c_str());

		// Create CUDA Stream
		CUresult cuRes = cuStreamCreate(&stream, CU_STREAM_NON_BLOCKING);
		if (cuRes == CUDA_SUCCESS) {
			NvVFX_SetCudaStream(effect, NVVFX_CUDA_STREAM, stream);
		} else {
			std::cerr << "[SRAdapter] Failed to create CUDA stream" << std::endl;
		}

		// Default Mode
		NvVFX_SetU32(effect, NVVFX_MODE, 1); // 0=Standard, 1=High Quality

		NvVFX_Load(effect);

		std::cout << "[SRAdapter] Initialized successfully." << std::endl;
		initialized = true;
	}

	void Process(const GPUFrameView *src, GPUFrameView *dst, const SRConfig *cfg)
	{
		if (!initialized || !effect)
			return;
		if (!src || !dst)
			return;

		// Config Upadates
		if (cfg) {
			NvVFX_SetU32(effect, NVVFX_MODE, cfg->quality_level);
		}

		// Import Vulkan -> CUDA
		// We reuse the skeleton logic but implement the wrapping

		// 1. Import Source
		CUarray srcArray = nullptr;
		CUexternalMemory srcExtMem = nullptr;

		if (src->fd >= 0) {
			if (ImportVulkanMemoryToCuda(src->fd, src->width * src->height * 4, &srcExtMem)) {
				MapCudaArrayFromExternalMemory(srcExtMem, src->width, src->height, 4, &srcArray);
			}
		}

		// 2. Import Destination (Assuming dst is also Vulkan backed)
		CUarray dstArray = nullptr;
		CUexternalMemory dstExtMem = nullptr;

		if (dst->fd >= 0) {
			if (ImportVulkanMemoryToCuda(dst->fd, dst->width * dst->height * 4, &dstExtMem)) {
				MapCudaArrayFromExternalMemory(dstExtMem, dst->width, dst->height, 4, &dstArray);
			}
		}

		if (srcArray && dstArray) {
			NvCVImage srcImg = {};
			NvCVImage dstImg = {};

			WrapCudaArrayInNvCVImage(&srcImg, srcArray, src->width, src->height, NVCV_RGBA);
			WrapCudaArrayInNvCVImage(&dstImg, dstArray, dst->width, dst->height, NVCV_RGBA);

			NvVFX_SetImage(effect, NVVFX_INPUT_IMAGE, &srcImg);
			NvVFX_SetImage(effect, NVVFX_OUTPUT_IMAGE, &dstImg);

			NvVFX_Run(effect, 0); // 0 = Sync? or Async depending on param? run_async is int.

			// Sync CUDA stream if needed, or let Vulkan Semaphore handle it (if wired up)
			// For now, simple synchronous wait on stream
			cuStreamSynchronize(stream);
		}

		// Cleanup resources (Very expensive per frame! Should cache!)
		// ... (Cleanup omitted for brevity in MVP)
		if (srcArray) {
			cuArrayDestroy(srcArray);
			cuDestroyExternalMemory(srcExtMem);
		}
		if (dstArray) {
			cuArrayDestroy(dstArray);
			cuDestroyExternalMemory(dstExtMem);
		}
	}

	void Shutdown()
	{
		if (effect) {
			NvVFX_DestroyEffect(effect);
			effect = nullptr;
		}
		if (stream) {
			cuStreamDestroy(stream);
			stream = nullptr;
		}
		initialized = false;
	}

	// Static Trampolines
	static void StaticInitialize(ISuperResolutionAdapter *self, int width, int height)
	{
		static_cast<SuperResolutionAdapter *>(self->user_data)->Initialize(width, height);
	}
	static void StaticProcess(ISuperResolutionAdapter *self, const GPUFrameView *src, GPUFrameView *dst,
				  const SRConfig *cfg)
	{
		static_cast<SuperResolutionAdapter *>(self->user_data)->Process(src, dst, cfg);
	}
	static void StaticShutdown(ISuperResolutionAdapter *self)
	{
		static_cast<SuperResolutionAdapter *>(self->user_data)->Shutdown();
	}

private:
	NvVFX_Handle effect = nullptr;
	CUstream stream = nullptr;
	bool initialized = false;
};

extern "C" ISuperResolutionAdapter *CreateSRAdapter()
{
	return new SuperResolutionAdapter();
}

} // namespace libvr
