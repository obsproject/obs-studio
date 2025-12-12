/*
* Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of NVIDIA CORPORATION nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef NV_VIDEO_EFFECTS_H
#define NV_VIDEO_EFFECTS_H

#include "nvCVImage.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration of the NvVFX_Object struct
typedef struct NvVFX_Object NvVFX_Object;

// NvVFX_Handle is a pointer to the opaque NvVFX_Object struct
typedef NvVFX_Object* NvVFX_Handle;

// NvVFX_StateObjectHandle is a handle to a state object
typedef void* NvVFX_StateObjectHandle;

// The effects 
#define NVVFX_FX_TRANSFER           "Transfer"
#define NVVFX_FX_GREEN_SCREEN       "GreenScreen"
#define NVVFX_FX_BGBLUR             "BackgroundBlur"
#define NVVFX_FX_ARTIFACT_REDUCTION "ArtifactReduction"
#define NVVFX_FX_SUPER_RES          "SuperRes"
#define NVVFX_FX_SR_UPSCALE         "Upscale"
#define NVVFX_FX_DENOISING          "Denoising"
#define NVVFX_FX_AUTO_FRAMING       "AutoFraming"
#define NVVFX_FX_VIDEO_NOISE        "VideoNoise"

// Parameter selectors
#define NVVFX_INPUT_IMAGE           "InputImage"
#define NVVFX_OUTPUT_IMAGE          "OutputImage"
#define NVVFX_MODEL_DIRECTORY       "ModelDir"
#define NVVFX_CUDA_STREAM           "CudaStream"
#define NVVFX_INFO                  "Info"
#define NVVFX_MODE                  "Mode"
#define NVVFX_STRENGTH              "Strength"
#define NVVFX_RESOLUTION            "Resolution"
#define NVVFX_STATE                 "State"
#define NVVFX_STATE_SIZE            "StateSize"
#define NVVFX_ROI                   "ROI"
#define NVVFX_MAX_CHARS             "MaxChars"
#define NVVFX_GPU                   "GPU"

// Error codes (subset of NvCV_Status)
// Reuse NvCV_Status from nvCVImage.h

// API
typedef const char* NvVFX_EffectSelector;
typedef const char* NvVFX_ParameterSelector;

#ifdef _WIN32
  #define NvVFX_API __stdcall
#else
  #define NvVFX_API
#endif

NvCV_Status NvVFX_API NvVFX_GetVersion(unsigned int* version);
NvCV_Status NvVFX_API NvVFX_CreateEffect(NvVFX_EffectSelector code, NvVFX_Handle* effect);
void        NvVFX_API NvVFX_DestroyEffect(NvVFX_Handle effect);
NvCV_Status NvVFX_API NvVFX_SetImage(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, NvCVImage* im);
NvCV_Status NvVFX_API NvVFX_SetString(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, const char* str);
NvCV_Status NvVFX_API NvVFX_SetU32(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, unsigned int val);
NvCV_Status NvVFX_API NvVFX_SetS32(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, int val);
NvCV_Status NvVFX_API NvVFX_SetF32(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, float val);
NvCV_Status NvVFX_API NvVFX_SetF64(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, double val);
NvCV_Status NvVFX_API NvVFX_SetU64(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, unsigned long long val);
NvCV_Status NvVFX_API NvVFX_SetObject(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, void* ptr);
NvCV_Status NvVFX_API NvVFX_SetCudaStream(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, void* stream); // CUstream
NvCV_Status NvVFX_API NvVFX_GetU32(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, unsigned int* val);
NvCV_Status NvVFX_API NvVFX_GetS32(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, int* val);
NvCV_Status NvVFX_API NvVFX_GetF32(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, float* val);
NvCV_Status NvVFX_API NvVFX_GetF64(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, double* val);
NvCV_Status NvVFX_API NvVFX_GetU64(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, unsigned long long* val);
NvCV_Status NvVFX_API NvVFX_GetImage(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, NvCVImage* im);
NvCV_Status NvVFX_API NvVFX_GetObject(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, void** ptr);
NvCV_Status NvVFX_API NvVFX_GetString(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, const char** str);
NvCV_Status NvVFX_API NvVFX_GetCudaStream(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, void** stream);
NvCV_Status NvVFX_API NvVFX_Run(NvVFX_Handle effect, int async);
NvCV_Status NvVFX_API NvVFX_Load(NvVFX_Handle effect);

// Cuda stream helpers
NvCV_Status NvVFX_API NvVFX_CudaStreamCreate(void** stream);
NvCV_Status NvVFX_API NvVFX_CudaStreamDestroy(void* stream);

// State helpers (omitted for brevity)
NvCV_Status NvVFX_API NvVFX_AllocateState(NvVFX_Handle effect, NvVFX_StateObjectHandle* handle);
NvCV_Status NvVFX_API NvVFX_DeallocateState(NvVFX_Handle effect, NvVFX_StateObjectHandle handle);
NvCV_Status NvVFX_API NvVFX_ResetState(NvVFX_Handle effect, NvVFX_StateObjectHandle handle);
NvCV_Status NvVFX_API NvVFX_SetStateObjectHandleArray(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, NvVFX_StateObjectHandle* handle);

#ifdef __cplusplus
}
#endif

#endif // NV_VIDEO_EFFECTS_H
