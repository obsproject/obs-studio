/*###############################################################################
#
# Copyright 2020-2021 NVIDIA Corporation
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

#ifndef __NVCVIMAGE_H__
#define __NVCVIMAGE_H__

#include "nvCVStatus.h"

#ifdef __cplusplus
extern "C" {
#endif // ___cplusplus


#ifndef   RTX_CAMERA_IMAGE    // Compile with -DRTX_CAMERA_IMAGE=0 to get more functionality and bug fixes.
  #define RTX_CAMERA_IMAGE  0 // Set to 1 for RTXCamera, which needs an old version, that avoids new functionality
#endif // RTX_CAMERA_IMAGE


struct CUstream_st;   // typedef struct CUstream_st *CUstream;

//! The format of pixels in an image.
typedef enum NvCVImage_PixelFormat {
  NVCV_FORMAT_UNKNOWN  = 0,    //!< Unknown pixel format.
  NVCV_Y               = 1,    //!< Luminance (gray).
  NVCV_A               = 2,    //!< Alpha (opacity)
  NVCV_YA              = 3,    //!< { Luminance, Alpha }
  NVCV_RGB             = 4,    //!< { Red, Green, Blue }
  NVCV_BGR             = 5,    //!< { Red, Green, Blue }
  NVCV_RGBA            = 6,    //!< { Red, Green, Blue, Alpha }
  NVCV_BGRA            = 7,    //!< { Red, Green, Blue, Alpha }
#if RTX_CAMERA_IMAGE
  NVCV_YUV420          = 8,    //!< Luminance and subsampled Chrominance { Y, Cb, Cr }
  NVCV_YUV422          = 9,    //!< Luminance and subsampled Chrominance { Y, Cb, Cr }
#else // !RTX_CAMERA_IMAGE
  NVCV_ARGB            = 8,    //!< { Red, Green, Blue, Alpha }
  NVCV_ABGR            = 9,    //!< { Red, Green, Blue, Alpha }
  NVCV_YUV420          = 10,   //!< Luminance and subsampled Chrominance { Y, Cb, Cr }
  NVCV_YUV422          = 11,   //!< Luminance and subsampled Chrominance { Y, Cb, Cr }
#endif // !RTX_CAMERA_IMAGE
  NVCV_YUV444          = 12,   //!< Luminance and full bandwidth Chrominance { Y, Cb, Cr }
} NvCVImage_PixelFormat;


//! The data type used to represent each component of an image.
typedef enum NvCVImage_ComponentType {
  NVCV_TYPE_UNKNOWN  = 0,      //!< Unknown type of component.
  NVCV_U8            = 1,      //!< Unsigned 8-bit integer.
  NVCV_U16           = 2,      //!< Unsigned 16-bit integer.
  NVCV_S16           = 3,      //!< Signed 16-bit integer.
  NVCV_F16           = 4,      //!< 16-bit floating-point.
  NVCV_U32           = 5,      //!< Unsigned 32-bit integer.
  NVCV_S32           = 6,      //!< Signed 32-bit integer.
  NVCV_F32           = 7,      //!< 32-bit floating-point (float).
  NVCV_U64           = 8,      //!< Unsigned 64-bit integer.
  NVCV_S64           = 9,      //!< Signed 64-bit integer.
  NVCV_F64           = 10,     //!< 64-bit floating-point (double).
} NvCVImage_ComponentType;


//! Value for the planar field or layout argument. Two values are currently accommodated for RGB:
//! Interleaved or chunky storage locates all components of a pixel adjacent in memory,
//! e.g. RGBRGBRGB... (denoted [RGB]).
//! Planar storage locates the same component of all pixels adjacent in memory,
//! e.g. RRRRR...GGGGG...BBBBB... (denoted [R][G][B])
//! YUV has many more variants.
//! 4:2:2 can be chunky, planar or semi-planar, with different orderings.
//! 4:2:0 can be planar or semi-planar, with different orderings.
//! Aliases are provided for FOURCCs defined at fourcc.org.
//! Note: the LSB can be used to distinguish between chunky and planar formats.
#define NVCV_INTERLEAVED   0   //!< All components of pixel(x,y) are adjacent (same as chunky) (default for non-YUV).
#define NVCV_CHUNKY        0   //!< All components of pixel(x,y) are adjacent (same as interleaved).
#define NVCV_PLANAR        1   //!< The same component of all pixels are adjacent.
#define NVCV_UYVY          2   //!< [UYVY]    Chunky 4:2:2 (default for 4:2:2)
#define NVCV_VYUY          4   //!< [VYUY]    Chunky 4:2:2
#define NVCV_YUYV          6   //!< [YUYV]    Chunky 4:2:2
#define NVCV_YVYU          8   //!< [YVYU]    Chunky 4:2:2
#define NVCV_CYUV         10   //!< [YUV]     Chunky 4:4:4
#define NVCV_CYVU         12   //!< [YVU]     Chunky 4:4:4
#define NVCV_YUV           3   //!< [Y][U][V] Planar 4:2:2 or 4:2:0 or 4:4:4
#define NVCV_YVU           5   //!< [Y][V][U] Planar 4:2:2 or 4:2:0 or 4:4:4
#define NVCV_YCUV          7   //!< [Y][UV]   Semi-planar 4:2:2 or 4:2:0 (default for 4:2:0)
#define NVCV_YCVU          9   //!< [Y][VU]   Semi-planar 4:2:2 or 4:2:0

//! The following are FOURCC aliases for specific layouts. Note that it is still required to specify the format as well
//! as the layout, e.g. NVCV_YUV420 and NVCV_NV12, even though the NV12 layout is only associated with YUV420 sampling.
#define NVCV_I420  NVCV_YUV    //!< [Y][U][V] Planar 4:2:0
#define NVCV_IYUV  NVCV_YUV    //!< [Y][U][V] Planar 4:2:0
#define NVCV_YV12  NVCV_YVU    //!< [Y][V][U] Planar 4:2:0
#define NVCV_NV12  NVCV_YCUV   //!< [Y][UV]   Semi-planar 4:2:0 (default for 4:2:0)
#define NVCV_NV21  NVCV_YCVU   //!< [Y][VU]   Semi-planar 4:2:0
#define NVCV_YUY2  NVCV_YUYV   //!< [YUYV]    Chunky 4:2:2
#define NVCV_I444  NVCV_YUV    //!< [Y][U][V] Planar 4:4:4
#define NVCV_YM24  NVCV_YUV    //!< [Y][U][V] Planar 4:4:4
#define NVCV_YM42  NVCV_YVU    //!< [Y][V][U] Planar 4:4:4
#define NVCV_NV24  NVCV_YCUV   //!< [Y][UV]   Semi-planar 4:4:4
#define NVCV_NV42  NVCV_YCVU   //!< [Y][VU]   Semi-planar 4:4:4

//! The following are ORed together for the colorspace field for YUV.
//! NVCV_601 and NVCV_709 describe the color axes of YUV.
//! NVCV_VIDEO_RANGE and NVCV_VIDEO_RANGE describe the range, [16, 235] or [0, 255], respectively.
//! NVCV_CHROMA_COSITED and NVCV_CHROMA_INTSTITIAL describe the location of the chroma samples.
#define NVCV_601               0x00   //!< The Rec.601  YUV colorspace, typically used for SD.
#define NVCV_709               0x01   //!< The Rec.709  YUV colorspace, typically used for HD.
#define NVCV_2020              0x02   //!< The Rec.2020 YUV colorspace. (Use for HDR PQ/HLG)
#define NVCV_VIDEO_RANGE       0x00   //!< The video range is [16, 235].
#define NVCV_FULL_RANGE        0x04   //!< The video range is [ 0, 255].
#define NVCV_CHROMA_COSITED    0x00   //!< The chroma is sampled at the same location as the luma samples horizontally.
#define NVCV_CHROMA_INTSTITIAL 0x08   //!< The chroma is sampled between luma samples horizontally.
#define NVCV_CHROMA_TOPLEFT    0x10   //!< The chroma is sampled at the same location as the luma samples horizontally and vertically.
#define NVCV_CHROMA_MPEG2      NVCV_CHROMA_COSITED        //!< As is most video.
#define NVCV_CHROMA_MPEG1      NVCV_CHROMA_INTSTITIAL
#define NVCV_CHROMA_JPEG       NVCV_CHROMA_INTSTITIAL
#define NVCV_CHROMA_H261       NVCV_CHROMA_INTSTITIAL
#define NVCV_CHROMA_INTERSTITIAL  NVCV_CHROMA_INTSTITIAL  //!< Correct spelling

//! This is the value for the gpuMem field or the memSpace argument.
#define NVCV_CPU          0   //!< The buffer is stored in CPU memory.
#define NVCV_GPU          1   //!< The buffer is stored in CUDA memory.
#define NVCV_CUDA         1   //!< The buffer is stored in CUDA memory.
#define NVCV_CPU_PINNED   2   //!< The buffer is stored in pinned CPU memory.
#define NVCV_CUDA_ARRAY   3   //!< A CUDA array is used for storage.

//! Image descriptor.
typedef struct
#ifdef _MSC_VER
__declspec(dllexport)
#endif // _MSC_VER
NvCVImage {
  unsigned int              width;                  //!< The number of pixels horizontally in the image.
  unsigned int              height;                 //!< The number of pixels  vertically  in the image.
  signed int                pitch;                  //!< The byte stride between pixels vertically.
  NvCVImage_PixelFormat     pixelFormat;            //!< The format of the pixels in the image.
  NvCVImage_ComponentType   componentType;          //!< The data type used to represent each component of the image.
  unsigned char             pixelBytes;             //!< The number of bytes in a chunky pixel.
  unsigned char             componentBytes;         //!< The number of bytes in each pixel component.
  unsigned char             numComponents;          //!< The number of components in each pixel.
  unsigned char             planar;                 //!< NVCV_CHUNKY, NVCV_PLANAR, NVCV_UYVY, ....
  unsigned char             gpuMem;                 //!< NVCV_CPU, NVCV_CPU_PINNED, NVCV_CUDA, NVCV_GPU
  unsigned char             colorspace;             //!< An OR of colorspace, range and chroma phase.
  unsigned char             reserved[2];            //!< For structure padding and future expansion. Set to 0.
  void                      *pixels;                //!< Pointer to pixel(0,0) in the image.
  void                      *deletePtr;             //!< Buffer memory to be deleted (can be NULL).
  void                      (*deleteProc)(void *p); //!< Delete procedure to call rather than free().
  unsigned long long        bufferBytes;            //!< The maximum amount of memory available through pixels.


#ifdef __cplusplus

  //! Default constructor: fill with 0.
  inline NvCVImage();

  //! Allocation constructor.
  //! \param[in]  width     the number of pixels horizontally.
  //! \param[in]  height    the number of pixels vertically.
  //! \param[in]  format    the format of the pixels.
  //! \param[in]  type      the type of each pixel component.
  //! \param[in]  layout    One of { NVCV_CHUNKY, NVCV_PLANAR } or one of the YUV layouts.
  //! \param[in]  memSpace  One of { NVCV_CPU, NVCV_CPU_PINNED, NVCV_GPU, NVCV_CUDA }
  //! \param[in]  alignment row byte alignment. Choose 0 or a power of 2.
  //!                       1: yields no gap whatsoever between scanlines;
  //!                       0: default alignment: 4 on CPU, and cudaMallocPitch's choice on GPU.
  //!                       Other common values are 16 or 32 for cache line size.
  inline NvCVImage(unsigned width, unsigned height, NvCVImage_PixelFormat format, NvCVImage_ComponentType type,
          unsigned layout = NVCV_CHUNKY, unsigned memSpace = NVCV_CPU, unsigned alignment = 0);

  //! Subimage constructor.
  //! \param[in]  fullImg   the full image, from which this subImage view is to be created.
  //! \param[in]  x         the left edge of the subImage, in reference to the full image.
  //! \param[in]  y         the top edge  of the subImage, in reference to the full image.
  //! \param[in]  width     the width  of the subImage, in pixels.
  //! \param[in]  height    the height of the subImage, in pixels.
  //! \bug        This does not work for planar or semi-planar formats, neither RGB nor YUV.
  //! \note       This does work for all chunky formats, including UYVY, VYUY, YUYV, YVYU.
  inline NvCVImage(NvCVImage *fullImg, int x, int y, unsigned width, unsigned height);

  //! Destructor
  inline ~NvCVImage();

  //! Copy a rectangular subimage. This works for CPU->CPU, CPU->GPU, GPU->GPU, and GPU->CPU.
  //! \param[in]  src     The source image from which to copy.
  //! \param[in]  srcX    The left coordinate of the src rectangle.
  //! \param[in]  srcY    The top  coordinate of the src rectangle.
  //! \param[in]  dstX    The left coordinate of the dst rectangle.
  //! \param[in]  dstY    The top  coordinate of the dst rectangle.
  //! \param[in]  width   The width  of the rectangle to be copied, in pixels.
  //! \param[in]  height  The height of the rectangle to be copied, in pixels.
  //! \param[in]  stream  the CUDA stream.
  //! \note   NvCVImage_Transfer() can handle more cases.
  //! \return NVCV_SUCCESS         if successful
  //! \return NVCV_ERR_MISMATCH    if the formats are different
  //! \return NVCV_ERR_CUDA        if a CUDA error occurred
  //! \return NVCV_ERR_PIXELFORMAT if the pixel format is not yet accommodated.
  inline NvCV_Status copyFrom(const NvCVImage *src, int srcX, int srcY, int dstX, int dstY,
                              unsigned width, unsigned height, struct CUstream_st* stream = 0);

  //! Copy from one image to another. This works for CPU->CPU, CPU->GPU, GPU->GPU, and GPU->CPU.
  //! \param[in]  src     The source image from which to copy.
  //! \param[in]  stream  the CUDA stream.
  //! \note   NvCVImage_Transfer() can handle more cases.
  //! \return NVCV_SUCCESS         if successful
  //! \return NVCV_ERR_MISMATCH    if the formats are different
  //! \return NVCV_ERR_CUDA        if a CUDA error occurred
  //! \return NVCV_ERR_PIXELFORMAT if the pixel format is not yet accommodated.
  inline NvCV_Status copyFrom(const NvCVImage *src, struct CUstream_st* stream = 0);

#endif // ___cplusplus
} NvCVImage;


//! Integer rectangle.
typedef struct NvCVRect2i   {
  int x;      //!< The left edge of the rectangle.
  int y;      //!< The top  edge of the rectangle.
  int width;  //!< The width  of the rectangle.
  int height; //!< The height of the rectangle.
} NvCVRect2i;


//! Integer point.
typedef struct NvCVPoint2i {
  int x;  //!< The horizontal coordinate.
  int y;  //!< The vertical coordinate
} NvCVPoint2i;


//! Initialize an image. The C++ constructors can initialize this appropriately.
//! This is called by the C++ constructor, but C code should call this explicitly.
//! \param[in,out]  im        the image to initialize.
//! \param[in]      width     the desired width  of the image, in pixels.
//! \param[in]      height    the desired height of the image, in pixels.
//! \param[in]      pitch     the byte stride between pixels vertically.
//! \param[in]      pixels    a pointer to the pixel buffer.
//! \param[in]      format    the format of the pixels.
//! \param[in]      type      the type of the components of the pixels.
//! \param[in]      layout    One of { NVCV_CHUNKY, NVCV_PLANAR } or one of the YUV layouts.
//! \param[in]      memSpace  Location of the buffer: one of { NVCV_CPU, NVCV_CPU_PINNED, NVCV_GPU, NVCV_CUDA }
//! \return NVCV_SUCCESS         if successful
//! \return NVCV_ERR_PIXELFORMAT if the pixel format is not yet accommodated.
NvCV_Status NvCV_API NvCVImage_Init(NvCVImage *im, unsigned width, unsigned height, int pitch, void *pixels,
  NvCVImage_PixelFormat format, NvCVImage_ComponentType type, unsigned layout, unsigned memSpace);


//! Initialize a view into a subset of an existing image.
//! No memory is allocated -- the fullImg buffer is used.
//! \param[in]  subImg  the sub-image view into the existing full image.
//! \param[in]  fullImg the existing full image.
//! \param[in]  x       the left edge of the sub-image, as coordinate of the full image.
//! \param[in]  y       the top  edge of the sub-image, as coordinate of the full image.
//! \param[in]  width   the desired width  of the subImage, in pixels.
//! \param[in]  height  the desired height of the subImage, in pixels.
//! \bug        This does not work in general for planar or semi-planar formats, neither RGB nor YUV.
//!             However, it does work for all formats with the full image, to make a shallow copy, e.g.
//!             NvCVImage_InitView(&subImg, &fullImg, 0, 0, fullImage.width, fullImage.height).
//!             Cropping a planar or semi-planar image can be accomplished with NvCVImage_TransferRect().
//! \note       This does work for all chunky formats, including UYVY, VYUY, YUYV, YVYU.
//! \sa         { NvCVImage_TransferRect }
void NvCV_API NvCVImage_InitView(NvCVImage *subImg, NvCVImage *fullImg, int x, int y, unsigned width, unsigned height);


//! Allocate memory for, and initialize an image. This assumes that the image data structure has nothing meaningful in it.
//! This is called by the C++ constructor, but C code can call this to allocate an image.
//! \param[in,out]  im        the image to initialize.
//! \param[in]      width     the desired width  of the image, in pixels.
//! \param[in]      height    the desired height of the image, in pixels.
//! \param[in]      format    the format of the pixels.
//! \param[in]      type      the type of the components of the pixels.
//! \param[in]      layout    One of { NVCV_CHUNKY, NVCV_PLANAR } or one of the YUV layouts.
//! \param[in]      memSpace  Location of the buffer: one of { NVCV_CPU, NVCV_CPU_PINNED, NVCV_GPU, NVCV_CUDA }
//! \param[in]      alignment row byte alignment. Choose 0 or a power of 2.
//!                           1: yields no gap whatsoever between scanlines;
//!                           0: default alignment: 4 on CPU, and cudaMallocPitch's choice on GPU.
//!                           Other common values are 16 or 32 for cache line size.
//! \return NVCV_SUCCESS         if the operation was successful.
//! \return NVCV_ERR_PIXELFORMAT if the pixel format is not accommodated.
//! \return NVCV_ERR_MEMORY      if there is not enough memory to allocate the buffer.
NvCV_Status NvCV_API NvCVImage_Alloc(NvCVImage *im, unsigned width, unsigned height, NvCVImage_PixelFormat format,
  NvCVImage_ComponentType type, unsigned layout, unsigned memSpace, unsigned alignment);


//! Reallocate memory for, and initialize an image. This assumes that the image is valid.
//! It will check bufferBytes to see if enough memory is already available, and will reshape rather than realloc if true.
//! Otherwise, it will free the previous buffer and reallocate a new one.
//! \param[in,out]  im        the image to initialize.
//! \param[in]      width     the desired width  of the image, in pixels.
//! \param[in]      height    the desired height of the image, in pixels.
//! \param[in]      format    the format of the pixels.
//! \param[in]      type      the type of the components of the pixels.
//! \param[in]      layout    One of { NVCV_CHUNKY, NVCV_PLANAR } or one of the YUV layouts.
//! \param[in]      memSpace  Location of the buffer: one of { NVCV_CPU, NVCV_CPU_PINNED, NVCV_GPU, NVCV_CUDA }
//! \param[in]      alignment row byte alignment. Choose 0 or a power of 2.
//!                           1: yields no gap whatsoever between scanlines;
//!                           0: default alignment: 4 on CPU, and cudaMallocPitch's choice on GPU.
//!                           Other common values are 16 or 32 for cache line size.
//! \return NVCV_SUCCESS         if the operation was successful.
//! \return NVCV_ERR_PIXELFORMAT if the pixel format is not accommodated.
//! \return NVCV_ERR_MEMORY      if there is not enough memory to allocate the buffer.
NvCV_Status NvCV_API NvCVImage_Realloc(NvCVImage *im, unsigned width, unsigned height, NvCVImage_PixelFormat format,
  NvCVImage_ComponentType type, unsigned layout, unsigned memSpace, unsigned alignment);


//! Deallocate the image buffer from the image. The image is not deallocated.
//! param[in,out] im  the image whose buffer is to be deallocated.
void NvCV_API NvCVImage_Dealloc(NvCVImage *im);


//! Deallocate the image buffer from the image asynchronously on the specified stream. The image is not deallocated.
//! param[in,out] im      the image whose buffer is to be deallocated.
//! param[int]    stream  the CUDA stream on which the image buffer is to be deallocated..
void NvCV_API NvCVImage_DeallocAsync(NvCVImage *im, struct CUstream_st *stream);


//! Allocate a new image, with storage (C-style constructor).
//! \param[in]      width     the desired width  of the image, in pixels.
//! \param[in]      height    the desired height of the image, in pixels.
//! \param[in]      format    the format of the pixels.
//! \param[in]      type      the type of the components of the pixels.
//! \param[in]      layout    One of { NVCV_CHUNKY, NVCV_PLANAR } or one of the YUV layouts.
//! \param[in]      memSpace  Location of the buffer: one of { NVCV_CPU, NVCV_CPU_PINNED, NVCV_GPU, NVCV_CUDA }
//! \param[in]      alignment row byte alignment. Choose 0 or a power of 2.
//!                           1: yields no gap whatsoever between scanlines;
//!                           0: default alignment: 4 on CPU, and cudaMallocPitch's choice on GPU.
//!                           Other common values are 16 or 32 for cache line size.
//! \param[out]         *out will be a pointer to the new image if successful; otherwise NULL.
//! \return NVCV_SUCCESS         if the operation was successful.
//! \return NVCV_ERR_PIXELFORMAT if the pixel format is not accommodated.
//! \return NVCV_ERR_MEMORY      if there is not enough memory to allocate the buffer.
NvCV_Status NvCV_API NvCVImage_Create(unsigned width, unsigned height, NvCVImage_PixelFormat format,
  NvCVImage_ComponentType type, unsigned layout, unsigned memSpace, unsigned alignment, NvCVImage **out);


//! Deallocate the image allocated with NvCVImage_Create() (C-style destructor).
void NvCV_API NvCVImage_Destroy(NvCVImage *im);


//! Get offsets for the components of a pixel format.
//! These are not byte offsets, but component offsets.
//! \param[in]  format  the pixel format to be interrogated.
//! \param[out] rOff    a place to store the offset for the red       channel (can be NULL).
//! \param[out] gOff    a place to store the offset for the green     channel (can be NULL).
//! \param[out] bOff    a place to store the offset for the blue      channel (can be NULL).
//! \param[out] aOff    a place to store the offset for the alpha     channel (can be NULL).
//! \param[out] yOff    a place to store the offset for the luminance channel (can be NULL).
void NvCV_API NvCVImage_ComponentOffsets(NvCVImage_PixelFormat format, int *rOff, int *gOff, int *bOff, int *aOff, int *yOff);


//! Transfer one image to another, with a limited set of conversions.
//!
//! If any of the images resides on the GPU, it may run asynchronously,
//! so cudaStreamSynchronize() should be called if it is necessary to run synchronously.
//! The following table indicates (with X) the currently-implemented conversions:
//!    +-------------------+-------------+-------------+-------------+-------------+
//!    |                   |  u8 --> u8  |  u8 --> f32 | f32 --> u8  | f32 --> f32 |
//!    +-------------------+-------------+-------------+-------------+-------------+
//!    | Y      --> Y      |      X      |             |      X      |      X      |
//!    | Y      --> A      |      X      |             |      X      |      X      |
//!    | Y      --> RGB    |      X      |      X      |      X      |      X      |
//!    | Y      --> RGBA   |      X      |      X      |      X      |      X      |
//!    | A      --> Y      |      X      |             |      X      |      X      |
//!    | A      --> A      |      X      |             |      X      |      X      |
//!    | A      --> RGB    |      X      |      X      |      X      |      X      |
//!    | A      --> RGBA   |      X      |             |             |             |
//!    | RGB    --> Y      |      X      |      X      |             |             |
//!    | RGB    --> A      |      X      |      X      |             |             |
//!    | RGB    --> RGB    |      X      |      X      |      X      |      X      |
//!    | RGB    --> RGBA   |      X      |      X      |      X      |      X      |
//!    | RGBA   --> Y      |      X      |      X      |             |             |
//!    | RGBA   --> A      |      X      |             |             |             |
//!    | RGBA   --> RGB    |      X      |      X      |      X      |      X      |
//!    | RGBA   --> RGBA   |      X      |      X      |      X      |      X      |
//!    | RGB    --> YUV420 |      X      |             |      X      |             |
//!    | RGBA   --> YUV420 |      X      |             |      X      |             |
//!    | RGB    --> YUV422 |      X      |             |      X      |             |
//!    | RGBA   --> YUV422 |      X      |             |      X      |             |
//!    | RGB    --> YUV444 |      X      |             |      X      |             |
//!    | RGBA   --> YUV444 |      X      |             |      X      |             |
//!    | YUV420 --> RGB    |      X      |      X      |             |             |
//!    | YUV420 --> RGBA   |      X      |      X      |             |             |
//!    | YUV422 --> RGB    |      X      |      X      |             |             |
//!    | YUV422 --> RGBA   |      X      |      X      |             |             |
//!    | YUV444 --> RGB    |      X      |      X      |             |             |
//!    | YUV444 --> RGBA   |      X      |      X      |             |             |
//!    +-------------------+-------------+-------------+-------------+-------------+
//! where
//! * Either source or destination can be CHUNKY or PLANAR.
//! * Either source or destination can reside on the CPU or the GPU.
//! * The RGB components are in any order (i.e. RGB or BGR; RGBA or BGRA).
//! * For RGBA (or BGRA) destinations, most implementations do not change the alpha channel, so it is recommended to
//!   set it at initialization time with [cuda]memset(im.pixels, -1, im.pitch * im.height) or
//!   [cuda]memset(im.pixels, -1, im.pitch * im.height * im.numComponents) for chunky and planar images respectively.
//! * YUV requires that the colorspace field be set manually prior to Transfer, e.g. typical for layout=NVCV_NV12:
//!   image.colorspace = NVCV_709 | NVCV_VIDEO_RANGE | NVCV_CHROMA_INTSTITIAL;
//! * There are also RGBf16-->RGBf32 and RGBf32-->RGBf16 transfers.
//! * Additionally, when the src and dst formats are the same, all formats are accommodated on CPU and GPU,
//!   and this can be used as a replacement for cudaMemcpy2DAsync() (which it utilizes). This is also true for YUV,
//!   whose src and dst must share the same format, layout and colorspace.
//!
//! When there is some kind of conversion AND the src and dst reside on different processors (CPU, GPU),
//! it is necessary to have a temporary GPU buffer, which is reshaped as needed to match the characteristics
//! of the CPU image. The same temporary image can be used in subsequent calls to NvCVImage_Transfer(),
//! regardless of the shape, format or component type, as it will grow as needed to accommodate
//! the largest memory requirement. The recommended usage for most cases is to supply an empty image
//! as the temporary; if it is not needed, no buffer is allocated. NULL can be supplied as the tmp
//! image, in which case an ephemeral buffer is allocated if needed, with resultant
//! performance degradation for image sequences.
//!
//! \param[in]      src     the source image.
//! \param[out]     dst     the destination image.
//! \param[in]      scale   a scale factor that can be applied when one (but not both) of the images
//!                         is based on floating-point components; this parameter is ignored when all image components
//!                         are represented with integer data types, or all image components are represented with
//!                         floating-point data types.
//! \param[in]      stream  the stream on which to perform the copy. This is ignored if both images reside on the CPU.
//! \param[in,out]  tmp     a temporary buffer that is sometimes needed when transferring images
//!                         between the CPU and GPU in either direction (can be empty or NULL).
//!                         It has the same characteristics as the CPU image, but it resides on the GPU.
//! \return         NVCV_SUCCESS           if successful,
//!                 NVCV_ERR_PIXELFORMAT   if one of the pixel formats is not accommodated.
//!                 NVCV_ERR_CUDA          if a CUDA error has occurred.
//!                 NVCV_ERR_GENERAL       if an otherwise unspecified error has occurred.
NvCV_Status NvCV_API NvCVImage_Transfer(
             const NvCVImage *src, NvCVImage *dst, float scale, struct CUstream_st *stream, NvCVImage *tmp);


//! Transfer a rectangular portion of an image.
//! See NvCVImage_Transfer() for the pixel format combinations that are implemented.
//! \param[in]  src     the source image.
//! \param[in]  srcRect the subRect of the src to be transferred (NULL implies the whole image).
//! \param[out] dst     the destination image.
//! \param[in]  dstPt   location to which the srcRect is to be copied (NULL implies (0,0)).
//! \param[in]  scale   scale factor applied to the magnitude during transfer, typically 1, 255 or 1/255.
//! \param[in]  stream  the CUDA stream.
//! \param[in]  tmp     a staging image.
//! \return     NVCV_SUCCESS  if the operation was completed successfully.
//! \note       The actual transfer region may be smaller, because the rects are clipped against the images.
NvCV_Status NvCV_API NvCVImage_TransferRect(
  const NvCVImage *src, const NvCVRect2i *srcRect, NvCVImage *dst, const NvCVPoint2i *dstPt,
  float scale, struct CUstream_st *stream, NvCVImage *tmp);


//! Transfer from a YUV image.
//! YUVu8 --> RGBu8 and YUVu8 --> RGBf32 are currently available.
//! \param[in]  y             pointer to pixel(0,0) of the luminance channel.
//! \param[in]  yPixBytes     the byte stride between y pixels horizontally.
//! \param[in]  yPitch        the byte stride between y pixels vertically.
//! \param[in]  u             pointer to pixel(0,0) of the u (Cb) chrominance channel.
//! \param[in]  v             pointer to pixel(0,0) of the v (Cr) chrominance channel.
//! \param[in]  uvPixBytes    the byte stride between u or v pixels horizontally.
//! \param[in]  uvPitch       the byte stride between u or v pixels vertically.
//! \param[in]  yuvColorSpace the yuv colorspace, specifying range, chromaticities, and chrominance phase.
//! \param[in]  yuvMemSpace   the memory space where the pixel buffers reside.
//! \param[out] dst           the destination image.
//! \param[in]  dstRect       the destination rectangle (NULL implies the whole image).
//! \param[in]  scale         scale factor applied to the magnitude during transfer, typically 1, 255 or 1/255.
//! \param[in]  stream        the CUDA stream.
//! \param[in]  tmp           a staging image.
//! \return     NVCV_SUCCESS  if the operation was completed successfully.
//! \note       The actual transfer region may be smaller, because the rects are clipped against the images.
//! \note       This is supplied for use with YUV buffers that do not have the standard structure
//!             that are expected for NvCVImage_Transfer() and NvCVImage_TransferRect.
NvCV_Status NvCV_API NvCVImage_TransferFromYUV(
  const void *y,                int yPixBytes,  int yPitch,
  const void *u, const void *v, int uvPixBytes, int uvPitch,
  NvCVImage_PixelFormat yuvFormat, NvCVImage_ComponentType yuvType,
  unsigned yuvColorSpace, unsigned yuvMemSpace,
  NvCVImage *dst, const NvCVRect2i *dstRect, float scale, struct CUstream_st *stream, NvCVImage *tmp);


//! Transfer to a YUV image.
//! RGBu8 --> YUVu8 and RGBf32 --> YUVu8 are currently available.
//! \param[in]  src           the source image.
//! \param[in]  srcRect       the destination rectangle (NULL implies the whole image).
//! \param[out] y             pointer to pixel(0,0) of the luminance channel.
//! \param[in]  yPixBytes     the byte stride between y pixels horizontally.
//! \param[in]  yPitch        the byte stride between y pixels vertically.
//! \param[out] u             pointer to pixel(0,0) of the u (Cb) chrominance channel.
//! \param[out] v             pointer to pixel(0,0) of the v (Cr) chrominance channel.
//! \param[in]  uvPixBytes    the byte stride between u or v pixels horizontally.
//! \param[in]  uvPitch       the byte stride between u or v pixels vertically.
//! \param[in]  yuvColorSpace the yuv colorspace, specifying range, chromaticities, and chrominance phase.
//! \param[in]  yuvMemSpace   the memory space where the pixel buffers reside.
//! \param[in]  scale         scale factor applied to the magnitude during transfer, typically 1, 255 or 1/255.
//! \param[in]  stream        the CUDA stream.
//! \param[in]  tmp           a staging image.
//! \return     NVCV_SUCCESS  if the operation was completed successfully.
//! \note       The actual transfer region may be smaller, because the rects are clipped against the images.
//! \note       This is supplied for use with YUV buffers that do not have the standard structure
//!             that are expected for NvCVImage_Transfer() and NvCVImage_TransferRect.
NvCV_Status NvCV_API NvCVImage_TransferToYUV(
  const NvCVImage *src, const NvCVRect2i *srcRect, 
  const void *y,                int yPixBytes,  int yPitch,
  const void *u, const void *v, int uvPixBytes, int uvPitch,
  NvCVImage_PixelFormat yuvFormat, NvCVImage_ComponentType yuvType,
  unsigned yuvColorSpace, unsigned yuvMemSpace,
  float scale, struct CUstream_st *stream, NvCVImage *tmp);


//! Between rendering by a graphics system and Transfer by CUDA, it is necessary to map the texture resource.
//! There is a fair amount of overhead, so its use should be minimized.
//! Every call to NvCVImage_MapResource() should be matched by a subsequent call to NvCVImage_UnmapResource().
//! \param[in,out]  im      the image to be mapped.
//! \param[in]      stream  the stream on which the mapping is to be performed.
//! \return         NVCV_SUCCESS is the operation was completed successfully.
//! \note           This is an experimental API. If you find it useful, please respond to XXX@YYY.com,
//!                 otherwise we may drop support.
/* EXPERIMENTAL */ NvCV_Status NvCV_API NvCVImage_MapResource(NvCVImage *im, struct CUstream_st *stream);


//! After transfer by CUDA, the texture resource must be unmapped in order to be used by the graphics system again.
//! There is a fair amount of overhead, so its use should be minimized.
//! Every call to NvCVImage_UnmapResource() should correspond to a preceding call to NvCVImage_MapResource().
//! \param[in,out]  im      the image to be mapped.
//! \param[in]      stream  the CUDA stream on which the mapping is to be performed.
//! \return         NVCV_SUCCESS is the operation was completed successfully.
//! \note           This is an experimental API. If you find it useful, please respond to XXX@YYY.com,
//!                 otherwise we may drop support.
/* EXPERIMENTAL */ NvCV_Status NvCV_API NvCVImage_UnmapResource(NvCVImage *im, struct CUstream_st *stream);


//! Composite one source image over another using the given matte.
//! This accommodates all RGB and RGBA formats, with u8 and f32 components.
//! If the bg has alpha, then the dst alpha is updated for use in subsequent composition.
//! \param[in]  fg      the foreground source image.
//! \param[in]  bg      the background source image.
//! \param[in]  mat     the matte  Yu8   (or Au8)   image, indicating where the src should come through.
//! \param[out] dst     the destination image. This can be the same as fg or bg.
//! \param[in]  stream  the CUDA stream on which the composition is to be performed.
//! \return NVCV_SUCCESS         if the operation was successful.
//! \return NVCV_ERR_PIXELFORMAT if the pixel format is not accommodated.
//! \return NVCV_ERR_MISMATCH    if either the fg & bg & dst formats do not match, or if fg & bg & dst & mat are not
//!                              in the same address space (CPU or GPU).
#if RTX_CAMERA_IMAGE == 0
NvCV_Status NvCV_API NvCVImage_Composite(const NvCVImage *fg, const NvCVImage *bg, const NvCVImage *mat, NvCVImage *dst,
            struct CUstream_st *stream);
#else // RTX_CAMERA_IMAGE == 1  // No GPU acceleration
NvCV_Status NvCV_API NvCVImage_Composite(const NvCVImage *fg, const NvCVImage *bg, const NvCVImage *mat, NvCVImage *dst);
#endif // RTX_CAMERA_IMAGE == 1


//! Composite one source image rectangular region over another using the given matte.
//! This accommodates all RGB and RGBA formats, with u8 and f32 components.
//! If the bg has alpha, then the dst alpha is updated for use in subsequent composition.
//! If the background is not opaque, it is recommended that all images be premultiplied by alpha,
//! and mode 1 composition be used, to yield the most meaningful composite matte.
//! \param[in]      fg      the foreground source image.
//! \param[in]      fgOrg   the upper-left corner of the fg image to be composited (NULL implies (0,0)).
//! \param[in]      bg      the background source image.
//! \param[in]      bgOrg   the upper-left corner of the bg image to be composited (NULL implies (0,0)).
//! \param[in]      mat     the matte image, indicating where the src should come through.
//!                         This determines the size of the rectangle to be composited.
//!                         If this is multi-channel, the alpha channel is used as the matte.
//! \param[in]      mode    the composition mode: 0 (straight alpha over) or 1 (premultiplied alpha over).
//! \param[out]     dst     the destination image. This can be the same as fg or bg.
//! \param[in]      dstOrg  the upper-left corner of the dst image to be updated (NULL implies (0,0)).
//! \param[in]      stream  the CUDA stream on which the composition is to be performed.
//! \note   If a smaller region of a matte is desired, a window can be created using
//!         NvCVImage_InitView() for chunky pixels, as illustrated below in NvCVImage_CompositeRectA().
//! \return NVCV_SUCCESS         if the operation was successful.
//! \return NVCV_ERR_PIXELFORMAT if the pixel format is not accommodated.
//! \return NVCV_ERR_MISMATCH    if either the fg & bg & dst formats do not match, or if fg & bg & dst & mat are not
//!                              in the same address space (CPU or GPU).
NvCV_Status NvCV_API NvCVImage_CompositeRect(
    const NvCVImage *fg,  const NvCVPoint2i *fgOrg,
    const NvCVImage *bg,  const NvCVPoint2i *bgOrg,
    const NvCVImage *mat, unsigned mode,
    NvCVImage       *dst, const NvCVPoint2i *dstOrg,
    struct CUstream_st *stream);


//! Composite one RGBA or BGRA source image rectangular region over another RGB, BGR, RGBA or BGRA region.
//! This accommodates all RGB and RGBA formats, with u8 and f32 components.
//! If the bg has alpha, then the dst alpha is updated for use in subsequent composition.
//! If the background is not opaque, it is recommended that all images be premultiplied by alpha,
//! and mode 1 composition be used, to yield the most meaningful composite matte.
//! \param[in]      fg      the foreground RGBA or BGRA source image.
//! \param[in]      fgRect  a sub-rectangle of the fg image (NULL implies the whole image).
//! \param[in]      bg      the background source image.
//! \param[in]      bgOrg   the upper-left corner of the bg image to be composited (NULL implies (0,0)).
//! \param[in]      mode    the composition mode: 0 (straight alpha over) or 1 (premultiplied alpha over).
//! \param[out]     dst     the destination image. This can be the same as fg or bg.
//! \param[in]      dstOrg  the upper-left corner of the dst image to be updated (NULL implies (0,0)).
//! \param[in]      stream  the CUDA stream on which the composition is to be performed.
//! \return NVCV_SUCCESS         if the operation was successful.
//! \return NVCV_ERR_PIXELFORMAT if the pixel format is not accommodated.
//! \return NVCV_ERR_MISMATCH    if either the fg & bg & dst formats do not match, or if fg & bg & dst & mat are not
//!                              in the same address space (CPU or GPU).
//! \bug  fgRect will only work for chunky images, not planar.
#ifdef __cplusplus
inline NvCV_Status NvCVImage_CompositeRectA(
    const NvCVImage *fg, const NvCVRect2i  *fgRect,
    const NvCVImage *bg, const NvCVPoint2i *bgOrg,
    unsigned mode,
    NvCVImage       *dst, const NvCVPoint2i *dstOrg,
    struct CUstream_st *stream
) {
  if (fgRect) {
    NvCVImage fgView(const_cast<NvCVImage*>(fg), fgRect->x, fgRect->y, fgRect->width, fgRect->height);
    return NvCVImage_CompositeRect(&fgView, nullptr, bg, bgOrg, &fgView, mode, dst, dstOrg, stream);
  }
  return NvCVImage_CompositeRect(fg, nullptr, bg, bgOrg, fg, mode, dst, dstOrg, stream);
}
#endif // __cplusplus


//! Composite a source image over a constant color field using the given matte.
//! \param[in]      src     the source image.
//! \param[in]      mat     the matte  image, indicating where the src should come through.
//! \param[in]      bgColor pointer to a location holding the desired flat background color, with the same format
//!                         and component ordering as the dst. This acts as a 1x1 background pixel buffer,
//!                         so should reside in the same memory space (CUDA or CPU) as the other buffers.
//! \param[in,out]  dst     the destination image. May be the same as src.
//! \return NVCV_SUCCESS         if the operation was successful.
//! \return NVCV_ERR_PIXELFORMAT if the pixel format is not accommodated.
//! \return NVCV_ERR_MISMATCH    if fg & mat & dst & bgColor are not in the same address space (CPU or GPU).
//! \note   The bgColor must remain valid until complete; this is an important consideration especially if
//!         the buffers are on the GPU and NvCVImage_CompositeOverConstant() runs asynchronously.
NvCV_Status NvCV_API NvCVImage_CompositeOverConstant(
#if RTX_CAMERA_IMAGE == 0
    const NvCVImage *src, const NvCVImage *mat, const void *bgColor, NvCVImage *dst, struct CUstream_st *stream
#else // RTX_CAMERA_IMAGE == 1
    const NvCVImage *src, const NvCVImage *mat, const unsigned char bgColor[3], NvCVImage *dst
#endif // RTX_CAMERA_IMAGE
);


//! Flip the image vertically.
//! No actual pixels are moved: it is just an accounting procedure.
//! This is extremely low overhead, but requires appropriate interpretation of the pitch.
//! Flipping twice yields the original orientation.
//! \param[in]  src  the source image (NULL implies src == dst).
//! \param[out] dst  the flipped image (can be the same as the src).
//! \return     NVCV_SUCCESS         if successful.
//! \return     NVCV_ERR_PIXELFORMAT for most planar formats.
//! \bug        This does not work for planar or semi-planar formats, neither RGB nor YUV.
//! \note       This does work for all chunky formats, including UYVY, VYUY, YUYV, YVYU.
NvCV_Status NvCV_API NvCVImage_FlipY(const NvCVImage *src, NvCVImage *dst);


//! Get the pointers for YUV, based on the format.
//! \param[in]  im          The image to be deconstructed.
//! \param[out] y           A place to store the pointer to y(0,0).
//! \param[out] u           A place to store the pointer to u(0,0).
//! \param[out] v           A place to store the pointer to v(0,0).
//! \param[out] yPixBytes   A place to store the byte stride between  luma  samples horizontally.
//! \param[out] cPixBytes   A place to store the byte stride between chroma samples horizontally.
//! \param[out] yRowBytes   A place to store the byte stride between  luma  samples vertically.
//! \param[out] cRowBytes   A place to store the byte stride between chroma samples vertically.
//! \return     NVCV_SUCCESS           If the information was gathered successfully.
//!             NVCV_ERR_PIXELFORMAT   Otherwise.
NvCV_Status NvCV_API NvCVImage_GetYUVPointers(NvCVImage *im,
  unsigned char **y, unsigned char **u, unsigned char **v,
  int *yPixBytes, int *cPixBytes, int *yRowBytes, int *cRowBytes);


//! Sharpen an image.
//! The src and dst should be the same type - conversions are not performed.
//! This function is only implemented for NVCV_CHUNKY NVCV_U8 pixels, of format NVCV_RGB or NVCV_BGR.
//! \param[in]  sharpness the sharpness strength, calibrated so that 1 and 2 yields Adobe's Sharpen and Sharpen More.
//! \param[in]  src       the source image to be sharpened.
//! \param[out] dst       the resultant image (may be the same as the src).
//! \param[in]  stream    the CUDA stream on which to perform the computations.
//! \param[in]  tmp       a temporary working image. This can be NULL, but may result in lower performance.
//!                       It is best if it resides on the same processor (CPU or GPU) as the destination.
//! @return     NVCV_SUCCESS          if the operation completed successfully.
//!             NVCV_ERR_MISMATCH     if the source and destination formats are different.
//!             NVCV_ERR_PIXELFORMAT  if the function has not been implemented for the chosen pixel type.

NvCV_Status NvCV_API NvCVImage_Sharpen(float sharpness, const NvCVImage *src, NvCVImage *dst,
  struct CUstream_st *stream, NvCVImage *tmp);


#ifdef __cplusplus
} // extern "C"

/********************************************************************************
 * NvCVImage default constructor
 ********************************************************************************/

NvCVImage::NvCVImage() {
  pixels = nullptr;
  (void)NvCVImage_Alloc(this, 0, 0, NVCV_FORMAT_UNKNOWN, NVCV_TYPE_UNKNOWN, 0, 0, 0);
}

/********************************************************************************
 * NvCVImage allocation constructor
 ********************************************************************************/

NvCVImage::NvCVImage(unsigned width, unsigned height, NvCVImage_PixelFormat format, NvCVImage_ComponentType type,
                       unsigned layout, unsigned memSpace, unsigned alignment) {
  pixels = nullptr;
  (void)NvCVImage_Alloc(this, width, height, format, type, layout, memSpace, alignment);
}

/********************************************************************************
 * Subimage constructor
 ********************************************************************************/

NvCVImage::NvCVImage(NvCVImage *fullImg, int x, int y, unsigned width, unsigned height) {
  NvCVImage_InitView(this, fullImg, x, y, width, height);
}

/********************************************************************************
 * NvCVImage destructor
 ********************************************************************************/

NvCVImage::~NvCVImage() { NvCVImage_Dealloc(this); }

/********************************************************************************
 * copy subimage
 ********************************************************************************/

NvCV_Status NvCVImage::copyFrom(const NvCVImage *src, int srcX, int srcY, int dstX, int dstY, unsigned wd,
                                  unsigned ht, struct CUstream_st* stream) {
#if RTX_CAMERA_IMAGE // This only works for chunky images
  NvCVImage srcView, dstView;
  NvCVImage_InitView(&srcView, const_cast<NvCVImage *>(src), srcX, srcY, wd, ht);
  NvCVImage_InitView(&dstView, this, dstX, dstY, wd, ht);
  return NvCVImage_Transfer(&srcView, &dstView, 1.f, stream, nullptr);
#else // !RTX_CAMERA_IMAGE bug fix for non-chunky images
  NvCVRect2i  srcRect = { (int)srcX, (int)srcY, (int)wd, (int)ht };
  NvCVPoint2i dstPt   = { (int)dstX, (int)dstY };
  return NvCVImage_TransferRect(src, &srcRect, this, &dstPt, 1.f, stream, nullptr);
#endif // RTX_CAMERA_IMAGE
}

/********************************************************************************
 * copy image
 ********************************************************************************/

NvCV_Status NvCVImage::copyFrom(const NvCVImage *src, struct CUstream_st* stream) {
  return NvCVImage_Transfer(src, this, 1.f, stream, nullptr);
}


#endif // ___cplusplus

#endif // __NVCVIMAGE_H__
