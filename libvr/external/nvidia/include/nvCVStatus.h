/*###############################################################################
#
# Copyright 2020 NVIDIA Corporation
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

#ifndef __NVCVSTATUS_H__
#define __NVCVSTATUS_H__

#ifndef NvCV_API
  #ifdef _WIN32
    #ifdef NVCV_API_EXPORT
      #define NvCV_API __declspec(dllexport) __cdecl
    #else
      #define NvCV_API
    #endif
  #else //if linux
    #define NvCV_API   // TODO: Linux code goes here
  #endif // _WIN32 or linux
#endif //NvCV_API


#ifdef __cplusplus
extern "C" {
#endif // ___cplusplus


//! Status codes returned from APIs.
typedef enum NvCV_Status {
  NVCV_SUCCESS                   = 0,    //!< The procedure returned successfully.
  NVCV_ERR_GENERAL               = -1,   //!< An otherwise unspecified error has occurred.
  NVCV_ERR_UNIMPLEMENTED         = -2,   //!< The requested feature is not yet implemented.
  NVCV_ERR_MEMORY                = -3,   //!< There is not enough memory for the requested operation.
  NVCV_ERR_EFFECT                = -4,   //!< An invalid effect handle has been supplied.
  NVCV_ERR_SELECTOR              = -5,   //!< The given parameter selector is not valid in this effect filter.
  NVCV_ERR_BUFFER                = -6,   //!< An image buffer has not been specified.
  NVCV_ERR_PARAMETER             = -7,   //!< An invalid parameter value has been supplied for this effect+selector.
  NVCV_ERR_MISMATCH              = -8,   //!< Some parameters are not appropriately matched.
  NVCV_ERR_PIXELFORMAT           = -9,   //!< The specified pixel format is not accommodated.
  NVCV_ERR_MODEL                 = -10,  //!< Error while loading the TRT model.
  NVCV_ERR_LIBRARY               = -11,  //!< Error loading the dynamic library.
  NVCV_ERR_INITIALIZATION        = -12,  //!< The effect has not been properly initialized.
  NVCV_ERR_FILE                  = -13,  //!< The file could not be found.
  NVCV_ERR_FEATURENOTFOUND       = -14,  //!< The requested feature was not found
  NVCV_ERR_MISSINGINPUT          = -15,  //!< A required parameter was not set
  NVCV_ERR_RESOLUTION            = -16,  //!< The specified image resolution is not supported.
  NVCV_ERR_UNSUPPORTEDGPU        = -17,  //!< The GPU is not supported
  NVCV_ERR_WRONGGPU              = -18,  //!< The current GPU is not the one selected.
  NVCV_ERR_UNSUPPORTEDDRIVER     = -19,  //!< The currently installed graphics driver is not supported
  NVCV_ERR_MODELDEPENDENCIES     = -20,  //!< There is no model with dependencies that match this system
  NVCV_ERR_PARSE                 = -21,  //!< There has been a parsing or syntax error while reading a file
  NVCV_ERR_MODELSUBSTITUTION     = -22,  //!< The specified model does not exist and has been substituted.
  NVCV_ERR_READ                  = -23,  //!< An error occurred while reading a file.
  NVCV_ERR_WRITE                 = -24,  //!< An error occurred while writing a file.
  NVCV_ERR_PARAMREADONLY         = -25,  //!< The selected parameter is read-only.
  NVCV_ERR_TRT_ENQUEUE           = -26,  //!< TensorRT enqueue failed.
  NVCV_ERR_TRT_BINDINGS          = -27,  //!< Unexpected TensorRT bindings.
  NVCV_ERR_TRT_CONTEXT           = -28,  //!< An error occurred while creating a TensorRT context.
  NVCV_ERR_TRT_INFER             = -29,  ///< The was a problem creating the inference engine.
  NVCV_ERR_TRT_ENGINE            = -30,  ///< There was a problem deserializing the inference runtime engine.
  NVCV_ERR_NPP                   = -31,  //!< An error has occurred in the NPP library.
  NVCV_ERR_CONFIG                = -32,  //!< No suitable model exists for the specified parameter configuration.
  NVCV_ERR_TOOSMALL              = -33,  //!< A supplied parameter or buffer is not large enough.
  NVCV_ERR_TOOBIG                = -34,  //!< A supplied parameter is too big.
  NVCV_ERR_WRONGSIZE             = -35,  //!< A supplied parameter is not the expected size.
  NVCV_ERR_OBJECTNOTFOUND        = -36,  //!< The specified object was not found.
  NVCV_ERR_SINGULAR              = -37,  //!< A mathematical singularity has been encountered.
  NVCV_ERR_NOTHINGRENDERED       = -38,  //!< Nothing was rendered in the specified region.
  NVCV_ERR_CONVERGENCE           = -39,  //!< An iteration did not converge satisfactorily.

  NVCV_ERR_OPENGL                = -98,  //!< An OpenGL error has occurred.
  NVCV_ERR_DIRECT3D              = -99,  //!< A Direct3D error has occurred.

  NVCV_ERR_CUDA_BASE             = -100,  //!< CUDA errors are offset from this value.
  NVCV_ERR_CUDA_VALUE            = -101,  //!< A CUDA parameter is not within the acceptable range.
  NVCV_ERR_CUDA_MEMORY           = -102,  //!< There is not enough CUDA memory for the requested operation.
  NVCV_ERR_CUDA_PITCH            = -112,  //!< A CUDA pitch is not within the acceptable range.
  NVCV_ERR_CUDA_INIT             = -127,  //!< The CUDA driver and runtime could not be initialized.
  NVCV_ERR_CUDA_LAUNCH           = -819,  //!< The CUDA kernel launch has failed.
  NVCV_ERR_CUDA_KERNEL           = -309,  //!< No suitable kernel image is available for the device.
  NVCV_ERR_CUDA_DRIVER           = -135,  //!< The installed NVIDIA CUDA driver is older than the CUDA runtime library.
  NVCV_ERR_CUDA_UNSUPPORTED      = -901,  //!< The CUDA operation is not supported on the current system or device.
  NVCV_ERR_CUDA_ILLEGAL_ADDRESS  = -800,  //!< CUDA tried to load or store on an invalid memory address.
  NVCV_ERR_CUDA                  = -1099, //!< An otherwise unspecified CUDA error has been reported.
} NvCV_Status;


//! Get an error string corresponding to the given status code.
//! \param[in]  code  the NvCV status code.
//! \return     the corresponding string.
//! \todo Find a cleaner way to do this, because NvCV_API doesn't work.
#ifdef _WIN32
  __declspec(dllexport) const char* __cdecl
#else
  const char* 
#endif // _WIN32 or linux
NvCV_GetErrorStringFromCode(NvCV_Status code);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __NVCVSTATUS_H__
