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

#ifndef __NVTRANSFER_D3D_H__
#define __NVTRANSFER_D3D_H__

#ifndef _WINDOWS_
  #define WIN32_LEAN_AND_MEAN
  #include <Windows.h>
#endif // _WINDOWS_
#include <dxgitype.h>
#include "nvCVImage.h"

#ifdef __cplusplus
extern "C" {
#endif // ___cplusplus



//! Utility to determine the D3D format from the NvCVImage format, type and layout.
//! \param[in]  format    the pixel format.
//! \param[in]  type      the component type.
//! \param[in]  layout    the layout.
//! \param[out] d3dFormat a place to store the corresponding D3D format.
//! \return     NVCV_SUCCESS if successful.
//! \note       This is an experimental API. If you find it useful, please respond to XXX@YYY.com, otherwise we may drop support.
/* EXPERIMENTAL */ NvCV_Status NvCV_API NvCVImage_ToD3DFormat(NvCVImage_PixelFormat format, NvCVImage_ComponentType type, unsigned layout, DXGI_FORMAT *d3dFormat);


//! Utility to determine the NvCVImage format, component type and layout from a D3D format.
//! \param[in]  d3dFormat the D3D format to translate.
//! \param[out] format    a place to store the NvCVImage pixel format.
//! \param[out] type      a place to store the NvCVImage component type.
//! \param[out] layout    a place to store the NvCVImage layout.
//! \return     NVCV_SUCCESS if successful.
//! \note       This is an experimental API. If you find it useful, please respond to XXX@YYY.com, otherwise we may drop support.
/* EXPERIMENTAL */ NvCV_Status NvCV_API NvCVImage_FromD3DFormat(DXGI_FORMAT d3dFormat, NvCVImage_PixelFormat *format, NvCVImage_ComponentType *type, unsigned char *layout);


#ifdef __dxgicommon_h__

//! Utility to determine the D3D color space from the NvCVImage color space.
//! \param[in]  nvcvColorSpace  the NvCVImage colro space (NVCV_2020 maps to DXGI HDR spaces).
//! \param[out] pD3dColorSpace  a place to store the resultant D3D color space.
//! \return     NVCV_SUCCESS          if successful.
//! \return     NVCV_ERR_PIXELFORMAT  if there is no equivalent color space.
//! \note       This is an experimental API. If you find it useful, please respond to XXX@YYY.com, otherwise we may drop support.
/* EXPERIMENTAL */ NvCV_Status NvCV_API NvCVImage_ToD3DColorSpace(unsigned char nvcvColorSpace, DXGI_COLOR_SPACE_TYPE *pD3dColorSpace);


//! Utility to determine the NvCVImage color space from the D3D color space.
//! \param[in]  d3dColorSpace   the D3D color space.
//! \param[out] pNvcvColorSpace a place to store the resultant NvCVImage color space.
//! \return     NVCV_SUCCESS          if successful.
//! \return     NVCV_ERR_PIXELFORMAT  if there is no equivalent color space.
//! \note       This is an experimental API. If you find it useful, please respond to XXX@YYY.com, otherwise we may drop support.
/* EXPERIMENTAL */ NvCV_Status NvCV_API NvCVImage_FromD3DColorSpace(DXGI_COLOR_SPACE_TYPE d3dColorSpace, unsigned char *pNvcvColorSpace);

#endif // __dxgicommon_h__


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __NVTRANSFER_D3D_H__

