#include "libvr/ISuperResolutionAdapter.h"
#include <iostream>
#include "nvVideoEffects.h"
#include "gpu/cuda_utils.h"

namespace libvr {

class SuperResolutionAdapter : public ISuperResolutionAdapter {
public:
    SuperResolutionAdapter() {
        static const ISuperResolutionAdapter_Vtbl vtbl_impl = {
            StaticInitialize,
            StaticProcess,
            StaticShutdown
        };
        this->vtbl = &vtbl_impl;
        this->user_data = this;
    }
    
    ~SuperResolutionAdapter() {
        Shutdown();
    }

    void Initialize(int width, int height) {
        std::cout << "[SRAdapter] Initializing NVIDIA VFX for resolution: " << width << "x" << height << std::endl;
        unsigned int ver = 0;
        NvCV_Status status = NvVFX_GetVersion(&ver);
        if (status != NVCV_SUCCESS) {
             std::cerr << "[SRAdapter] Failed to get VFX version. dlopen issue?" << std::endl;
             return;
        }
        std::cout << "[SRAdapter] VFX SDK Version: " << ver << std::endl;
        
        status = NvVFX_CreateEffect(NVVFX_FX_SUPER_RES, &effect);
        if (status != NVCV_SUCCESS) {
            std::cerr << "[SRAdapter] Failed to create SuperRes effect: " << status << std::endl;
        } else {
             std::cout << "[SRAdapter] SuperRes Effect Created." << std::endl;
             // Set default mode: High Quality (1)
             NvVFX_SetU32(effect, NVVFX_MODE, 1);
        }
    }

    void Process(const GPUFrameView* src, GPUFrameView* dst, const SRConfig* cfg) {
        if (!effect) return;
        if (!src || !dst) return;
        
        // Update Config if provided
        if (cfg) {
            // NvVFX_SetU32(effect, NVVFX_MODE, cfg->quality_level);
        }
        
        // Zero-Copy Interop (Vulkan -> CUDA)
        if (src->fd >= 0) {
             CUexternalMemory extMem = nullptr;
             CUarray cudaArray = nullptr;
             
             // Important: For real usage, we must sync (wait for Vulkan semaphore).
             // For skeleton, we assume idle/sequenced.
             
             // Size calculation (linear estimate for R8G8B8A8)
             uint64_t size = src->width * src->height * 4; 
             
             if (ImportVulkanMemoryToCuda(src->fd, size, &extMem)) {
                 if (MapCudaArrayFromExternalMemory(extMem, src->width, src->height, 4, &cudaArray)) {
                     // Successfully mapped! 
                     // Now we would wrap cudaArray in NvCVImage using NvCVImage_InitFromCudaArray (if available)
                     // or NvCVImage_Init with correct type.
                     // The NvVFX SDK usually takes NvCVImage.
                     
                     // Helper: 
                     // NvCVImage_Init(&inputImage, src->width, src->height, src->stride, (void*)cudaArray, NVCV_RGBA, NVCV_U8, NVCV_CUDA_ARRAY);
                     
                     std::cout << "[SRAdapter] Zero-Copy Import Successful for FD " << src->fd << std::endl;
                     
                     // Free resources for this frame (In reality, cache this!)
                     FreeCudaResources(extMem, cudaArray);
                 }
             }
        } else {
             // Fallback or Error
             // std::cerr << "[SRAdapter] Invalid FD for Zero-Copy" << std::endl;
        }
    }
    
    void Shutdown() {
        if (effect) {
            NvVFX_DestroyEffect(effect);
            effect = nullptr;
        }
    }

    // Static Trampolines
    static void StaticInitialize(ISuperResolutionAdapter* self, int width, int height) {
        static_cast<SuperResolutionAdapter*>(self->user_data)->Initialize(width, height);
    }
    static void StaticProcess(ISuperResolutionAdapter* self, const GPUFrameView* src, GPUFrameView* dst, const SRConfig* cfg) {
        static_cast<SuperResolutionAdapter*>(self->user_data)->Process(src, dst, cfg);
    }
    static void StaticShutdown(ISuperResolutionAdapter* self) {
        static_cast<SuperResolutionAdapter*>(self->user_data)->Shutdown();
    }

private:
    NvVFX_Handle effect = nullptr;
};


extern "C" ISuperResolutionAdapter* CreateSRAdapter() {
    return new SuperResolutionAdapter();
}

} // namespace libvr
