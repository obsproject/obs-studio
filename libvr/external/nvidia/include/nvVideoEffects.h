/*###############################################################################
#
# Copyright (c) 2020 NVIDIA Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
###############################################################################*/

#ifndef __NVVIDEO_EFFECTS_H__
#define __NVVIDEO_EFFECTS_H__

#include "nvCVImage.h"

#ifndef NvVFX_API
  #ifdef _WIN32
    #ifdef NVVFX_API_EXPORT
      #define NvVFX_API __declspec(dllexport) __cdecl
    #else
      #define NvVFX_API
    #endif
  #else //if linux
    #define NvVFX_API   // TODO: Linux code goes here
  #endif // _WIN32 or linux
#endif //NvVFX_API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Forward declaration for CUDA API
typedef struct CUstream_st* CUstream;

//! We use strings as effect selectors.
typedef const char* NvVFX_EffectSelector;

//! We use strings as parameter selectors.
typedef const char* NvVFX_ParameterSelector;

//! Each effect instantiation is associated with an opaque handle.
struct NvVFX_Object;
typedef struct NvVFX_Object NvVFX_Object, *NvVFX_Handle;

///! 
///! Effect may use this handle to manage state objects.
///! 
struct NvVFX_StateObjectHandleBase;
typedef struct NvVFX_StateObjectHandleBase* NvVFX_StateObjectHandle;

//! Get the SDK version
//! \param[in,out]  version    Pointer to an unsigned int set to 
//!                            (major << 24) | (minor << 16) | (build << 8) | 0
//! \return         NVCV_SUCCESS  if the version was set
//! \return         NVCV_ERR_PARAMETER  if version was NULL
NvCV_Status NvVFX_API NvVFX_GetVersion(unsigned int *version);

//! Create an new instantiation of a video effect.
//! \param[in]  code    the selector code for the desired video effect.
//! \param[out] effect  a handle to the Video Effect instantiation.
//! \return     NVCV_SUCCESS   if the operation was successful.
NvCV_Status NvVFX_API NvVFX_CreateEffect(NvVFX_EffectSelector code, NvVFX_Handle *effect);


//! Delete a previously allocated video effect.
//! \param[in]  effect a handle to the video effect to be deleted.
void NvVFX_API NvVFX_DestroyEffect(NvVFX_Handle effect);


//! Set the value of the selected parameter (unsigned int, int, float double, unsigned long long, void*, CUstream).
//! \param[in,out]  effect      The effect to configure.
//! \param[in]      paramName   The selector of the effect parameter to configure.
//! \param[in]      val         The value to be assigned to the selected effect parameter.
//! \return NVCV_SUCCESS       if the operation was successful.
//! \return NVCV_ERR_EFFECT    if an invalid effect handle was supplied.
//! \return NVCV_ERR_SELECTOR  if the chosen effect does not understand the specified selector and data type.
//! \return NVCV_ERR_PARAMETER if the value was out of range.
NvCV_Status NvVFX_API NvVFX_SetU32(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, unsigned int val);
NvCV_Status NvVFX_API NvVFX_SetS32(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, int val);
NvCV_Status NvVFX_API NvVFX_SetF32(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, float val);
NvCV_Status NvVFX_API NvVFX_SetF64(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, double val);
NvCV_Status NvVFX_API NvVFX_SetU64(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, unsigned long long val);
NvCV_Status NvVFX_API NvVFX_SetObject(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, void *ptr);
NvCV_Status NvVFX_API NvVFX_SetStateObjectHandleArray(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, NvVFX_StateObjectHandle* handle);
NvCV_Status NvVFX_API NvVFX_SetCudaStream(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, CUstream stream);

//! Set the selected image descriptor.
//! A shallow copy of the descriptor is made (preserving the pixel pointer), so that an ephemeral NvVFXImage_Init()
//! wrapper may be used in the call to NvVFX_SetImage() if desired, without having to preserve it for the lifetime
//! of the effect. The effect does not take ownership of the pixel buffer.
//! \param[in,out]  effect      The effect to configure.
//! \param[in]      paramName   The selector of the effect image to configure.
//! \param[in]      im          Pointer to the image descriptor to be used for the selected effect image.
//!                             NULL clears the selected internal image descriptor.
//! \return NVCV_SUCCESS       if the operation was successful.
//! \return NVCV_ERR_EFFECT    if an invalid effect handle was supplied.
//! \return NVCV_ERR_SELECTOR  if the chosen effect does not understand the specified image selector.
//! \return NVCV_ERR_PARAMETER if an unexpected NULL pointer was supplied.
NvCV_Status NvVFX_API NvVFX_SetImage(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, NvCVImage *im);

//! Set the value of the selected string, by making a copy in the effect handle.
//! \param[in,out]  effect      The effect to configure.
//! \param[in]      paramName   The selector of the effect string to configure.
//! \param[in]      str         The value to be assigned to the selected effect string. NULL clears the selected string.
//! \return NVCV_SUCCESS       if the operation was successful.
//! \return NVCV_ERR_EFFECT    if an invalid effect handle was supplied.
//! \return NVCV_ERR_SELECTOR  if the chosen effect does not understand the specified string selector.
//! \return NVCV_ERR_PARAMETER if an unexpected NULL pointer was supplied.
NvCV_Status NvVFX_API NvVFX_SetString(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, const char *str);


//! Get the value of the selected parameter (unsigned int, int, float double, unsigned long long, void*, CUstream).
//! These are not typically used except for testing.
//! \param[in]  effect    the effect to be queried.
//! \param[in]  paramName the selector of the effect parameter to retrieve.
//! \param[out] val       a place to store the retrieved parameter.
//! \return NVCV_SUCCESS       if the operation was successful.
//! \return NVCV_ERR_EFFECT    if an invalid effect handle was supplied.
//! \return NVCV_ERR_SELECTOR  if the chosen effect does not understand the specified selector and data type.
//! \return NVCV_ERR_PARAMETER if an unexpected NULL pointer was supplied.
//! \note Typically, these are not used outside of testing.
NvCV_Status NvVFX_API NvVFX_GetU32(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, unsigned int *val);
NvCV_Status NvVFX_API NvVFX_GetS32(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, int *val);
NvCV_Status NvVFX_API NvVFX_GetF32(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, float *val);
NvCV_Status NvVFX_API NvVFX_GetF64(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, double *val);
NvCV_Status NvVFX_API NvVFX_GetU64(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, unsigned long long *val);
NvCV_Status NvVFX_API NvVFX_GetObject(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, void **ptr);
NvCV_Status NvVFX_API NvVFX_GetCudaStream(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, CUstream *stream);

//! Get a copy of the selected image descriptor.
//! If GetImage() is called before SetImage(), the returned descriptor will be filled with zeros.
//! Otherwise, the values will be identical to that in the previous SetImage() call,
//! with the exception of deletePtr, deleteProc and bufferBytes, which will be 0.
//! \param[in]  effect    the effect to be queried.
//! \param[in]  paramName the selector of the effect image to retrieve.
//! \param[out] val       a place to store the selected image descriptor.
//!                       A pointer to an empty NvCVImage (deletePtr==NULL) should be supplied to avoid memory leaks.
//! \return NVCV_SUCCESS       if the operation was successful.
//! \return NVCV_ERR_EFFECT    if an invalid effect handle was supplied.
//! \return NVCV_ERR_SELECTOR  if the chosen effect does not understand the specified image selector.
//! \return NVCV_ERR_PARAMETER if an unexpected NULL pointer was supplied.
//! \note Typically, this is not used outside of testing.
NvCV_Status NvVFX_API NvVFX_GetImage(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, NvCVImage *im);

//! Get the specified string.
//! If GetString() is called before SetString(), the returned string will be empty.
//! Otherwise, the string will be identical to that in the previous SetString() call,
//! though it will be stored in a different location.
//! \param[in]  effect    the effect to be queried.
//! \param[in]  paramName the selector of the effect string to retrieve.
//! \param[out] val       a place to store a pointer to the selected string.
//! \return NVCV_SUCCESS       if the operation was successful.
//! \return NVCV_ERR_EFFECT    if an invalid effect handle was supplied.
//! \return NVCV_ERR_SELECTOR  if the chosen effect does not understand the specified string selector.
//! \return NVCV_ERR_PARAMETER if an unexpected NULL pointer was supplied.
//! \note Typically, this is not used outside of testing.
NvCV_Status NvVFX_API NvVFX_GetString(NvVFX_Handle effect, NvVFX_ParameterSelector paramName, const char **str);

//! Run the selected effect.
//! \param[in]  effect     the effect object handle.
//! \param[in]  async   run the effect asynchronously if nonzero; otherwise run synchronously.
//! \todo       Should async instead be a pointer to a place to store a token that can be useful
//!             for synchronizing two streams alter?
//! \return     NVCV_SUCCESS     if the operation was successful.
//! \return     NVCV_ERR_EFFECT  if an invalid effect handle was supplied.
NvCV_Status NvVFX_API NvVFX_Run(NvVFX_Handle effect, int async);

//! Load the model based on the set params.
//! \param[in]  effect     the effect object handle.
//! \return     NVCV_SUCCESS     if the operation was successful.
//! \return     NVCV_ERR_EFFECT  if an invalid effect handle was supplied.
NvCV_Status NvVFX_API NvVFX_Load(NvVFX_Handle effect);

//! Wrapper for cudaStreamCreate(), if it is desired to avoid linking with the cuda lib.
//! \param[out] stream  A place to store the newly allocated stream.
//! \return     NVCV_SUCCESS         if the operation was successful,
//!             NVCV_ERR_CUDA_VALUE  if not.
NvCV_Status NvVFX_API NvVFX_CudaStreamCreate(CUstream *stream);

//! Wrapper for cudaStreamDestroy(), if it is desired to avoid linking with the cuda lib.
//! \param[in]  stream  The stream to destroy.
//! \return     NVCV_SUCCESS         if the operation was successful,
//!             NVCV_ERR_CUDA_VALUE  if not.
NvCV_Status NvVFX_API NvVFX_CudaStreamDestroy(CUstream stream);

//! Allocate the state object handle for a feature.
//! \param[in]  effect   the effect object handle.
//! \param[in]  handle   handle to the state object
//! \return     NVCV_SUCCESS    if the operation was successful.
//! \return     NVCV_ERR_EFFECT  if an invalid effect handle was supplied.
//! \note This may depend on prior settings of parameters.
NvCV_Status NvVFX_API NvVFX_AllocateState(NvVFX_Handle effect, NvVFX_StateObjectHandle* handle);

//! Deallocate the state object handle for stateful feature.
//! \param[in]  effect   the effect object handle.
//! \param[in]  handle   handle to the state object
//! \return     NVCV_SUCCESS    if the operation was successful.
//! \return     NVCV_ERR_EFFECT  if an invalid effect handle was supplied.
NvCV_Status NvVFX_API NvVFX_DeallocateState(NvVFX_Handle effect, NvVFX_StateObjectHandle handle);

//! Reset the state object handle for stateful feature.
//! \param[in]  effect   the effect object handle.
//! \param[in]  handle   handle to the state object
//! \return     NVCV_SUCCESS    if the operation was successful.
//! \return     NVCV_ERR_EFFECT  if an invalid effect handle was supplied.
NvCV_Status NvVFX_API NvVFX_ResetState(NvVFX_Handle effect, NvVFX_StateObjectHandle handle);


// Filter selectors
#define NVVFX_FX_TRANSFER               "Transfer"
#define NVVFX_FX_GREEN_SCREEN           "GreenScreen"         // Green Screen 
#define NVVFX_FX_BGBLUR                 "BackgroundBlur"     // Background blur
#define NVVFX_FX_ARTIFACT_REDUCTION     "ArtifactReduction"   // Artifact Reduction  
#define NVVFX_FX_SUPER_RES              "SuperRes"            // Super Res 
#define NVVFX_FX_SR_UPSCALE             "Upscale"             // Super Res Upscale 
#define NVVFX_FX_DENOISING              "Denoising"           // Denoising 

// Parameter selectors
#define NVVFX_INPUT_IMAGE_0             "SrcImage0"           //!< There may be multiple input images
#define NVVFX_INPUT_IMAGE               NVVFX_INPUT_IMAGE_0   //!< but there is usually only one input image
#define NVVFX_INPUT_IMAGE_1             "SrcImage1"           //!< Source Image 1
#define NVVFX_OUTPUT_IMAGE_0            "DstImage0"           //!< There may be multiple output images
#define NVVFX_OUTPUT_IMAGE              NVVFX_OUTPUT_IMAGE_0  //!< but there is usually only one output image
#define NVVFX_MODEL_DIRECTORY           "ModelDir"            //!< The directory where the model may be found
#define NVVFX_CUDA_STREAM               "CudaStream"          //!< The CUDA stream to use
#define NVVFX_CUDA_GRAPH                "CudaGraph"           //!< Enable CUDA graph to use
#define NVVFX_INFO                      "Info"                //!< Get info about the effects
#define NVVFX_MAX_INPUT_WIDTH           "MaxInputWidth"       //!< Maximum width of the input supported
#define NVVFX_MAX_INPUT_HEIGHT          "MaxInputHeight"      //!< Maximum height of the input supported
#define NVVFX_MAX_NUMBER_STREAMS        "MaxNumberStreams"  //!< Maximum number of concurrent input streams
#define NVVFX_SCALE                     "Scale"               //!< Scale factor
#define NVVFX_STRENGTH                  "Strength"            //!< Strength for different filters
#define NVVFX_STRENGTH_LEVELS           "StrengthLevels"      //!< Number of strength levels
#define NVVFX_MODE                      "Mode"                //!< Mode for different filters
#define NVVFX_TEMPORAL                  "Temporal"            //!< Temporal mode: 0=image, 1=video
#define NVVFX_GPU                       "GPU"                 //!< Preferred GPU (optional)
#define NVVFX_BATCH_SIZE                "BatchSize"           //!< Batch Size (default 1)
#define NVVFX_MODEL_BATCH               "ModelBatch"          //!< The preferred batching model to use (default 1)
#define NVVFX_STATE                     "State"               //!< State variable  
#define NVVFX_STATE_SIZE                "StateSize"           //!< Number of bytes needed to store state  
#define NVVFX_STATE_COUNT               "NumStateObjects"     //!< Number of active state object handles


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __NVVIDEO_EFFECTS_H__
