/*###############################################################################
#
# Copyright(c) 2021 NVIDIA CORPORATION.All Rights Reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#
###############################################################################*/

#ifndef __NVTRANSFER_D3D11_H__
#define __NVTRANSFER_D3D11_H__

#ifdef _WIN32
#include <d3d11.h>
#endif
#include "nvCVImage.h"
#ifdef _WIN32
#include "nvTransferD3D.h"  // for NvCVImage_ToD3DFormat() and NvCVImage_FromD3DFormat()
#endif

#ifdef __cplusplus
extern "C" {
#endif // ___cplusplus



//! Initialize an NvCVImage from a D3D11 texture.
//! The pixelFormat and component types with be transferred over, and a cudaGraphicsResource will be registered;
//! the NvCVImage destructor will unregister the resource.
//! It is necessary to call NvCVImage_MapResource() after rendering D3D and before calling  NvCVImage_Transfer(),
//! and to call NvCVImage_UnmapResource() before rendering in D3D again.
//! \param[in,out]  im  the image to be initialized.
//! \param[in]      tx  the texture to be used for initialization.
//! \return         NVCV_SUCCESS if successful.
//! \note           This is an experimental API. If you find it useful, please respond to XXX@YYY.com,
//!                 otherwise we may drop support.
#ifdef _WIN32
/* EXPERIMENTAL */ NvCV_Status NvCV_API NvCVImage_InitFromD3D11Texture(NvCVImage *im, struct ID3D11Texture2D *tx);
#endif



#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __NVTRANSFER_D3D11_H__

