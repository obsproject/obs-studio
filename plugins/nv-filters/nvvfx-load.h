#pragma once
#include <Windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <util/platform.h>
#include <dxgitype.h>
#include <util/windows/win-version.h>
#include "nv_sdk_versions.h"

#ifdef __cplusplus
extern "C" {
#endif // ___cplusplus

#ifndef NvVFX_API
#ifdef _WIN32
#ifdef NVVFX_API_EXPORT
#define NvVFX_API __declspec(dllexport) __cdecl
#else
#define NvVFX_API
#endif
#else             //if linux
#define NvVFX_API // TODO: Linux code goes here
#endif            // _WIN32 or linux
#endif            //NvVFX_API

#ifndef NvCV_API
#ifdef _WIN32
#ifdef NVCV_API_EXPORT
#define NvCV_API __declspec(dllexport) __cdecl
#else
#define NvCV_API
#endif
#else            //if linux
#define NvCV_API // TODO: Linux code goes here
#endif           // _WIN32 or linux
#endif           //NvCV_API

#define CUDARTAPI

#ifdef LIBNVVFX_ENABLED
static HMODULE nv_videofx = NULL;
static HMODULE nv_cvimage = NULL;
static HMODULE nv_cudart = NULL;
static HMODULE nv_cuda = NULL;

//! Status codes returned from APIs.
typedef enum NvCV_Status {
	NVCV_SUCCESS = 0,                 //!< The procedure returned successfully.
	NVCV_ERR_GENERAL = -1,            //!< An otherwise unspecified error has occurred.
	NVCV_ERR_UNIMPLEMENTED = -2,      //!< The requested feature is not yet implemented.
	NVCV_ERR_MEMORY = -3,             //!< There is not enough memory for the requested operation.
	NVCV_ERR_EFFECT = -4,             //!< An invalid effect handle has been supplied.
	NVCV_ERR_SELECTOR = -5,           //!< The given parameter selector is not valid in this effect filter.
	NVCV_ERR_BUFFER = -6,             //!< An image buffer has not been specified.
	NVCV_ERR_PARAMETER = -7,          //!< An invalid parameter value has been supplied for this effect+selector.
	NVCV_ERR_MISMATCH = -8,           //!< Some parameters are not appropriately matched.
	NVCV_ERR_PIXELFORMAT = -9,        //!< The specified pixel format is not accommodated.
	NVCV_ERR_MODEL = -10,             //!< Error while loading the TRT model.
	NVCV_ERR_LIBRARY = -11,           //!< Error loading the dynamic library.
	NVCV_ERR_INITIALIZATION = -12,    //!< The effect has not been properly initialized.
	NVCV_ERR_FILE = -13,              //!< The file could not be found.
	NVCV_ERR_FEATURENOTFOUND = -14,   //!< The requested feature was not found
	NVCV_ERR_MISSINGINPUT = -15,      //!< A required parameter was not set
	NVCV_ERR_RESOLUTION = -16,        //!< The specified image resolution is not supported.
	NVCV_ERR_UNSUPPORTEDGPU = -17,    //!< The GPU is not supported
	NVCV_ERR_WRONGGPU = -18,          //!< The current GPU is not the one selected.
	NVCV_ERR_UNSUPPORTEDDRIVER = -19, //!< The currently installed graphics driver is not supported
	NVCV_ERR_MODELDEPENDENCIES = -20, //!< There is no model with dependencies that match this system
	NVCV_ERR_PARSE = -21,             //!< There has been a parsing or syntax error while reading a file
	NVCV_ERR_MODELSUBSTITUTION = -22, //!< The specified model does not exist and has been substituted.
	NVCV_ERR_READ = -23,              //!< An error occurred while reading a file.
	NVCV_ERR_WRITE = -24,             //!< An error occurred while writing a file.
	NVCV_ERR_PARAMREADONLY = -25,     //!< The selected parameter is read-only.
	NVCV_ERR_TRT_ENQUEUE = -26,       //!< TensorRT enqueue failed.
	NVCV_ERR_TRT_BINDINGS = -27,      //!< Unexpected TensorRT bindings.
	NVCV_ERR_TRT_CONTEXT = -28,       //!< An error occurred while creating a TensorRT context.
	NVCV_ERR_TRT_INFER = -29,         ///< The was a problem creating the inference engine.
	NVCV_ERR_TRT_ENGINE = -30,        ///< There was a problem deserializing the inference runtime engine.
	NVCV_ERR_NPP = -31,               //!< An error has occurred in the NPP library.
	NVCV_ERR_CONFIG = -32,            //!< No suitable model exists for the specified parameter configuration.

	NVCV_ERR_DIRECT3D = -99, //!< A Direct3D error has occurred.

	NVCV_ERR_CUDA_BASE = -100,        //!< CUDA errors are offset from this value.
	NVCV_ERR_CUDA_VALUE = -101,       //!< A CUDA parameter is not within the acceptable range.
	NVCV_ERR_CUDA_MEMORY = -102,      //!< There is not enough CUDA memory for the requested operation.
	NVCV_ERR_CUDA_PITCH = -112,       //!< A CUDA pitch is not within the acceptable range.
	NVCV_ERR_CUDA_INIT = -127,        //!< The CUDA driver and runtime could not be initialized.
	NVCV_ERR_CUDA_LAUNCH = -819,      //!< The CUDA kernel launch has failed.
	NVCV_ERR_CUDA_KERNEL = -309,      //!< No suitable kernel image is available for the device.
	NVCV_ERR_CUDA_DRIVER = -135,      //!< The installed NVIDIA CUDA driver is older than the CUDA runtime library.
	NVCV_ERR_CUDA_UNSUPPORTED = -901, //!< The CUDA operation is not supported on the current system or device.
	NVCV_ERR_CUDA_ILLEGAL_ADDRESS = -800, //!< CUDA tried to load or store on an invalid memory address.
	NVCV_ERR_CUDA = -1099,                //!< An otherwise unspecified CUDA error has been reported.
} NvCV_Status;

/**  Filter selectors */
#define NVVFX_FX_TRANSFER "Transfer"
#define NVVFX_FX_GREEN_SCREEN "GreenScreen"             // Green Screen
#define NVVFX_FX_BGBLUR "BackgroundBlur"                // Background blur
#define NVVFX_FX_ARTIFACT_REDUCTION "ArtifactReduction" // Artifact Reduction
#define NVVFX_FX_SUPER_RES "SuperRes"                   // Super Res
#define NVVFX_FX_SR_UPSCALE "Upscale"                   // Super Res Upscale
#define NVVFX_FX_DENOISING "Denoising"                  // Denoising

/** Parameter selectors */
#define NVVFX_INPUT_IMAGE_0 "SrcImage0"
#define NVVFX_INPUT_IMAGE NVVFX_INPUT_IMAGE_0
#define NVVFX_INPUT_IMAGE_1 "SrcImage1"
#define NVVFX_OUTPUT_IMAGE_0 "DstImage0"
#define NVVFX_OUTPUT_IMAGE NVVFX_OUTPUT_IMAGE_0
#define NVVFX_MODEL_DIRECTORY "ModelDir"
#define NVVFX_CUDA_STREAM "CudaStream"              //!< The CUDA stream to use
#define NVVFX_CUDA_GRAPH "CudaGraph"                //!< Enable CUDA graph to use
#define NVVFX_INFO "Info"                           //!< Get info about the effects
#define NVVFX_MAX_INPUT_WIDTH "MaxInputWidth"       //!< Maximum width of the input supported
#define NVVFX_MAX_INPUT_HEIGHT "MaxInputHeight"     //!< Maximum height of the input supported
#define NVVFX_MAX_NUMBER_STREAMS "MaxNumberStreams" //!< Maximum number of concurrent input streams
#define NVVFX_SCALE "Scale"                         //!< Scale factor
#define NVVFX_STRENGTH "Strength"                   //!< Strength for different filters
#define NVVFX_STRENGTH_LEVELS "StrengthLevels"      //!< Number of strength levels
#define NVVFX_MODE "Mode"                           //!< Mode for different filters
#define NVVFX_TEMPORAL "Temporal"                   //!< Temporal mode: 0=image, 1=video
#define NVVFX_GPU "GPU"                             //!< Preferred GPU (optional)
#define NVVFX_BATCH_SIZE "BatchSize"                //!< Batch Size (default 1)
#define NVVFX_MODEL_BATCH "ModelBatch"
#define NVVFX_STATE "State"                 //!< State variable
#define NVVFX_STATE_SIZE "StateSize"        //!< Number of bytes needed to store state
#define NVVFX_STATE_COUNT "NumStateObjects" //!< Number of active state object handles

//! The format of pixels in an image.
typedef enum NvCVImage_PixelFormat {
	NVCV_FORMAT_UNKNOWN = 0, //!< Unknown pixel format.
	NVCV_Y = 1,              //!< Luminance (gray).
	NVCV_A = 2,              //!< Alpha (opacity)
	NVCV_YA = 3,             //!< { Luminance, Alpha }
	NVCV_RGB = 4,            //!< { Red, Green, Blue }
	NVCV_BGR = 5,            //!< { Red, Green, Blue }
	NVCV_RGBA = 6,           //!< { Red, Green, Blue, Alpha }
	NVCV_BGRA = 7,           //!< { Red, Green, Blue, Alpha }
	NVCV_ARGB = 8,           //!< { Red, Green, Blue, Alpha }
	NVCV_ABGR = 9,           //!< { Red, Green, Blue, Alpha }
	NVCV_YUV420 = 10,        //!< Luminance and subsampled Chrominance { Y, Cb, Cr }
	NVCV_YUV422 = 11,        //!< Luminance and subsampled Chrominance { Y, Cb, Cr }
	NVCV_YUV444 = 12,        //!< Luminance and full bandwidth Chrominance { Y, Cb, Cr }
} NvCVImage_PixelFormat;

//! The data type used to represent each component of an image.
typedef enum NvCVImage_ComponentType {
	NVCV_TYPE_UNKNOWN = 0, //!< Unknown type of component.
	NVCV_U8 = 1,           //!< Unsigned 8-bit integer.
	NVCV_U16 = 2,          //!< Unsigned 16-bit integer.
	NVCV_S16 = 3,          //!< Signed 16-bit integer.
	NVCV_F16 = 4,          //!< 16-bit floating-point.
	NVCV_U32 = 5,          //!< Unsigned 32-bit integer.
	NVCV_S32 = 6,          //!< Signed 32-bit integer.
	NVCV_F32 = 7,          //!< 32-bit floating-point (float).
	NVCV_U64 = 8,          //!< Unsigned 64-bit integer.
	NVCV_S64 = 9,          //!< Signed 64-bit integer.
	NVCV_F64 = 10,         //!< 64-bit floating-point (double).
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
#define NVCV_INTERLEAVED 0 //!< All components of pixel(x,y) are adjacent (same as chunky) (default for non-YUV).
#define NVCV_CHUNKY 0      //!< All components of pixel(x,y) are adjacent (same as interleaved).
#define NVCV_PLANAR 1      //!< The same component of all pixels are adjacent.
#define NVCV_UYVY 2        //!< [UYVY]    Chunky 4:2:2 (default for 4:2:2)
#define NVCV_VYUY 4        //!< [VYUY]    Chunky 4:2:2
#define NVCV_YUYV 6        //!< [YUYV]    Chunky 4:2:2
#define NVCV_YVYU 8        //!< [YVYU]    Chunky 4:2:2
#define NVCV_CYUV 10       //!< [YUV]     Chunky 4:4:4
#define NVCV_CYVU 12       //!< [YVU]     Chunky 4:4:4
#define NVCV_YUV 3         //!< [Y][U][V] Planar 4:2:2 or 4:2:0 or 4:4:4
#define NVCV_YVU 5         //!< [Y][V][U] Planar 4:2:2 or 4:2:0 or 4:4:4
#define NVCV_YCUV 7        //!< [Y][UV]   Semi-planar 4:2:2 or 4:2:0 (default for 4:2:0)
#define NVCV_YCVU 9        //!< [Y][VU]   Semi-planar 4:2:2 or 4:2:0

//! The following are FOURCC aliases for specific layouts. Note that it is still required to specify the format as well
//! as the layout, e.g. NVCV_YUV420 and NVCV_NV12, even though the NV12 layout is only associated with YUV420 sampling.
#define NVCV_I420 NVCV_YUV  //!< [Y][U][V] Planar 4:2:0
#define NVCV_IYUV NVCV_YUV  //!< [Y][U][V] Planar 4:2:0
#define NVCV_YV12 NVCV_YVU  //!< [Y][V][U] Planar 4:2:0
#define NVCV_NV12 NVCV_YCUV //!< [Y][UV]   Semi-planar 4:2:0 (default for 4:2:0)
#define NVCV_NV21 NVCV_YCVU //!< [Y][VU]   Semi-planar 4:2:0
#define NVCV_YUY2 NVCV_YUYV //!< [YUYV]    Chunky 4:2:2
#define NVCV_I444 NVCV_YUV  //!< [Y][U][V] Planar 4:4:4
#define NVCV_YM24 NVCV_YUV  //!< [Y][U][V] Planar 4:4:4
#define NVCV_YM42 NVCV_YVU  //!< [Y][V][U] Planar 4:4:4
#define NVCV_NV24 NVCV_YCUV //!< [Y][UV]   Semi-planar 4:4:4
#define NVCV_NV42 NVCV_YCVU //!< [Y][VU]   Semi-planar 4:4:4

//! The following are ORed together for the colorspace field for YUV.
//! NVCV_601 and NVCV_709 describe the color axes of YUV.
//! NVCV_VIDEO_RANGE and NVCV_VIDEO_RANGE describe the range, [16, 235] or [0, 255], respectively.
//! NVCV_CHROMA_COSITED and NVCV_CHROMA_INTSTITIAL describe the location of the chroma samples.
#define NVCV_601 0x00               //!< The Rec.601  YUV colorspace, typically used for SD.
#define NVCV_709 0x01               //!< The Rec.709  YUV colorspace, typically used for HD.
#define NVCV_2020 0x02              //!< The Rec.2020 YUV colorspace.
#define NVCV_VIDEO_RANGE 0x00       //!< The video range is [16, 235].
#define NVCV_FULL_RANGE 0x04        //!< The video range is [ 0, 255].
#define NVCV_CHROMA_COSITED 0x00    //!< The chroma is sampled at the same location as the luma samples horizontally.
#define NVCV_CHROMA_INTSTITIAL 0x08 //!< The chroma is sampled between luma samples horizontally.
#define NVCV_CHROMA_TOPLEFT \
	0x10 //!< The chroma is sampled at the same location as the luma samples horizontally and vertically.
#define NVCV_CHROMA_MPEG2 NVCV_CHROMA_COSITED //!< As is most video.
#define NVCV_CHROMA_MPEG1 NVCV_CHROMA_INTSTITIAL
#define NVCV_CHROMA_JPEG NVCV_CHROMA_INTSTITIAL
#define NVCV_CHROMA_H261 NVCV_CHROMA_INTSTITIAL
#define NVCV_CHROMA_INTERSTITIAL NVCV_CHROMA_INTSTITIAL //!< Correct spelling

//! This is the value for the gpuMem field or the memSpace argument.
#define NVCV_CPU 0        //!< The buffer is stored in CPU memory.
#define NVCV_GPU 1        //!< The buffer is stored in CUDA memory.
#define NVCV_CUDA 1       //!< The buffer is stored in CUDA memory.
#define NVCV_CPU_PINNED 2 //!< The buffer is stored in pinned CPU memory.
#define NVCV_CUDA_ARRAY 3 //!< A CUDA array is used for storage.

/** Logging support **/
enum {
	NVCV_LOG_FATAL,   //!< Message to be printed right before aborting due to an unrecoverable error.
	NVCV_LOG_ERROR,   //!< An operation has failed, but it is not fatal.
	NVCV_LOG_WARNING, //!< Something was not quite right, but we fixed it up, perhaps at a loss in performance.
	NVCV_LOG_INFO     //!< Nothing is wrong, but this information might be of interest.
};

//! Image descriptor.
typedef struct
#ifdef _MSC_VER
	__declspec(dllexport)
#endif // _MSC_VER
	NvCVImage {
	unsigned int width;                    //!< The number of pixels horizontally in the image.
	unsigned int height;                   //!< The number of pixels  vertically  in the image.
	signed int pitch;                      //!< The byte stride between pixels vertically.
	NvCVImage_PixelFormat pixelFormat;     //!< The format of the pixels in the image.
	NvCVImage_ComponentType componentType; //!< The data type used to represent each component of the image.
	unsigned char pixelBytes;              //!< The number of bytes in a chunky pixel.
	unsigned char componentBytes;          //!< The number of bytes in each pixel component.
	unsigned char numComponents;           //!< The number of components in each pixel.
	unsigned char planar;                  //!< NVCV_CHUNKY, NVCV_PLANAR, NVCV_UYVY, ....
	unsigned char gpuMem;                  //!< NVCV_CPU, NVCV_CPU_PINNED, NVCV_CUDA, NVCV_GPU
	unsigned char colorspace;              //!< An OR of colorspace, range and chroma phase.
	unsigned char reserved[2];             //!< For structure padding and future expansion. Set to 0.
	void *pixels;                          //!< Pointer to pixel(0,0) in the image.
	void *deletePtr;                       //!< Buffer memory to be deleted (can be NULL).
	void (*deleteProc)(void *p);           //!< Delete procedure to call rather than free().
	unsigned long long bufferBytes;        //!< The maximum amount of memory available through pixels.
} NvCVImage;

//! Integer rectangle.
typedef struct NvCVRect2i {
	int x;      //!< The left edge of the rectangle.
	int y;      //!< The top  edge of the rectangle.
	int width;  //!< The width  of the rectangle.
	int height; //!< The height of the rectangle.
} NvCVRect2i;

//! Integer point.
typedef struct NvCVPoint2i {
	int x; //!< The horizontal coordinate.
	int y; //!< The vertical coordinate
} NvCVPoint2i;

typedef struct CUstream_st *CUstream;
typedef const char *NvVFX_EffectSelector;
typedef const char *NvVFX_ParameterSelector;
typedef void *NvVFX_Handle;
/* requires sdk version >= 0.7.0 */
typedef void *NvVFX_StateObjectHandle;

/* nvvfx functions */
typedef NvCV_Status NvVFX_API (*NvVFX_GetVersion_t)(unsigned int *version);
typedef NvCV_Status NvVFX_API (*NvVFX_CreateEffect_t)(NvVFX_EffectSelector code, NvVFX_Handle *effect);
typedef NvCV_Status NvVFX_API (*NvVFX_DestroyEffect_t)(NvVFX_Handle effect);
typedef NvCV_Status NvVFX_API (*NvVFX_SetU32_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name,
						unsigned int val);
typedef NvCV_Status NvVFX_API (*NvVFX_SetS32_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name, int val);
typedef NvCV_Status NvVFX_API (*NvVFX_SetF32_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name, float val);
typedef NvCV_Status NvVFX_API (*NvVFX_SetF64_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name, double val);
typedef NvCV_Status NvVFX_API (*NvVFX_SetU64_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name,
						unsigned long long val);
typedef NvCV_Status NvVFX_API (*NvVFX_SetObject_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name, void *ptr);
/* requires sdk version >= 0.7.0 */
typedef NvCV_Status NvVFX_API (*NvVFX_SetStateObjectHandleArray_t)(NvVFX_Handle effect,
								   NvVFX_ParameterSelector param_name,
								   NvVFX_StateObjectHandle *handle);
/* ----------------------------- */
typedef NvCV_Status NvVFX_API (*NvVFX_SetCudaStream_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name,
						       CUstream stream);
typedef NvCV_Status NvVFX_API (*NvVFX_SetImage_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name,
						  NvCVImage *im);
typedef NvCV_Status NvVFX_API (*NvVFX_SetString_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name,
						   const char *str);
typedef NvCV_Status NvVFX_API (*NvVFX_GetU32_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name,
						unsigned int *val);
typedef NvCV_Status NvVFX_API (*NvVFX_GetS32_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name, int *val);
typedef NvCV_Status NvVFX_API (*NvVFX_GetF32_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name, float *val);
typedef NvCV_Status NvVFX_API (*NvVFX_GetF64_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name, double *val);
typedef NvCV_Status NvVFX_API (*NvVFX_GetU64_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name,
						unsigned long long *val);
typedef NvCV_Status NvVFX_API (*NvVFX_GetObject_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name, void **ptr);
typedef NvCV_Status NvVFX_API (*NvVFX_GetCudaStream_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name,
						       CUstream *stream);
typedef NvCV_Status NvVFX_API (*NvVFX_GetImage_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name,
						  NvCVImage *im);
typedef NvCV_Status NvVFX_API (*NvVFX_GetString_t)(NvVFX_Handle effect, NvVFX_ParameterSelector param_name,
						   const char **str);
typedef NvCV_Status NvVFX_API (*NvVFX_Run_t)(NvVFX_Handle effect, int async);
typedef NvCV_Status NvVFX_API (*NvVFX_Load_t)(NvVFX_Handle effect);
typedef NvCV_Status NvVFX_API (*NvVFX_CudaStreamCreate_t)(CUstream *stream);
typedef NvCV_Status NvVFX_API (*NvVFX_CudaStreamDestroy_t)(CUstream stream);

/* requires SDK version >= 0.7.0 */
typedef NvCV_Status NvVFX_API (*NvVFX_AllocateState_t)(NvVFX_Handle effect, NvVFX_StateObjectHandle *handle);
typedef NvCV_Status NvVFX_API (*NvVFX_DeallocateState_t)(NvVFX_Handle effect, NvVFX_StateObjectHandle handle);
typedef NvCV_Status NvVFX_API (*NvVFX_ResetState_t)(NvVFX_Handle effect, NvVFX_StateObjectHandle handle);

/* requires SDK version >= 0.7.5 */
typedef NvCV_Status NvVFX_API (*NvVFX_ConfigureLogger_t)(int verbosity, const char *file,
							 void (*cb)(void *, const char *), void *cb_data);
/* NvCVImage functions */
typedef NvCV_Status NvCV_API (*NvCVImage_Init_t)(NvCVImage *im, unsigned width, unsigned height, int pitch,
						 void *pixels, NvCVImage_PixelFormat format,
						 NvCVImage_ComponentType type, unsigned layout, unsigned memSpace);
typedef NvCV_Status NvCV_API (*NvCVImage_InitView_t)(NvCVImage *subImg, NvCVImage *fullImg, int x, int y,
						     unsigned width, unsigned height);
typedef NvCV_Status NvCV_API (*NvCVImage_Alloc_t)(NvCVImage *im, unsigned width, unsigned height,
						  NvCVImage_PixelFormat format, NvCVImage_ComponentType type,
						  unsigned layout, unsigned memSpace, unsigned alignment);
typedef NvCV_Status NvCV_API (*NvCVImage_Realloc_t)(NvCVImage *im, unsigned width, unsigned height,
						    NvCVImage_PixelFormat format, NvCVImage_ComponentType type,
						    unsigned layout, unsigned memSpace, unsigned alignment);
typedef NvCV_Status NvCV_API (*NvCVImage_Dealloc_t)(NvCVImage *im);
typedef NvCV_Status NvCV_API (*NvCVImage_Create_t)(unsigned width, unsigned height, NvCVImage_PixelFormat format,
						   NvCVImage_ComponentType type, unsigned layout, unsigned memSpace,
						   unsigned alignment, NvCVImage **out);
typedef NvCV_Status NvCV_API (*NvCVImage_Destroy_t)(NvCVImage *im);
typedef NvCV_Status NvCV_API (*NvCVImage_ComponentOffsets_t)(NvCVImage_PixelFormat format, int *rOff, int *gOff,
							     int *bOff, int *aOff, int *yOff);
typedef NvCV_Status NvCV_API (*NvCVImage_Transfer_t)(const NvCVImage *src, NvCVImage *dst, float scale,
						     struct CUstream_st *stream, NvCVImage *tmp);
typedef NvCV_Status NvCV_API (*NvCVImage_TransferRect_t)(const NvCVImage *src, const NvCVRect2i *srcRect,
							 NvCVImage *dst, const NvCVPoint2i *dstPt, float scale,
							 struct CUstream_st *stream, NvCVImage *tmp);
typedef NvCV_Status NvCV_API (*NvCVImage_TransferFromYUV_t)(
	const void *y, int yPixBytes, int yPitch, const void *u, const void *v, int uvPixBytes, int uvPitch,
	NvCVImage_PixelFormat yuvFormat, NvCVImage_ComponentType yuvType, unsigned yuvColorSpace, unsigned yuvMemSpace,
	NvCVImage *dst, const NvCVRect2i *dstRect, float scale, struct CUstream_st *stream, NvCVImage *tmp);
typedef NvCV_Status NvCV_API (*NvCVImage_TransferToYUV_t)(
	const NvCVImage *src, const NvCVRect2i *srcRect, const void *y, int yPixBytes, int yPitch, const void *u,
	const void *v, int uvPixBytes, int uvPitch, NvCVImage_PixelFormat yuvFormat, NvCVImage_ComponentType yuvType,
	unsigned yuvColorSpace, unsigned yuvMemSpace, float scale, struct CUstream_st *stream, NvCVImage *tmp);
typedef NvCV_Status NvCV_API (*NvCVImage_MapResource_t)(NvCVImage *im, struct CUstream_st *stream);

typedef NvCV_Status NvCV_API (*NvCVImage_UnmapResource_t)(NvCVImage *im, struct CUstream_st *stream);
typedef NvCV_Status NvCV_API (*NvCVImage_Composite_t)(const NvCVImage *fg, const NvCVImage *bg, const NvCVImage *mat,
						      NvCVImage *dst, struct CUstream_st *stream);
typedef NvCV_Status NvCV_API (*NvCVImage_CompositeRect_t)(const NvCVImage *fg, const NvCVPoint2i *fgOrg,
							  const NvCVImage *bg, const NvCVPoint2i *bgOrg,
							  const NvCVImage *mat, unsigned mode, NvCVImage *dst,
							  const NvCVPoint2i *dstOrg, struct CUstream_st *stream);
typedef NvCV_Status NvCV_API (*NvCVImage_CompositeOverConstant_t)(const NvCVImage *src, const NvCVImage *mat,
								  const void *bgColor, NvCVImage *dst,
								  struct CUstream_st *stream);
typedef NvCV_Status NvCV_API (*NvCVImage_FlipY_t)(const NvCVImage *src, NvCVImage *dst);
typedef NvCV_Status NvCV_API (*NvCVImage_GetYUVPointers_t)(NvCVImage *im, unsigned char **y, unsigned char **u,
							   unsigned char **v, int *yPixBytes, int *cPixBytes,
							   int *yRowBytes, int *cRowBytes);

typedef const char *(*NvCV_GetErrorStringFromCode_t)(NvCV_Status code);

/* beware: this is experimental : D3D functions */
typedef NvCV_Status NvCV_API (*NvCVImage_ToD3DFormat_t)(NvCVImage_PixelFormat format, NvCVImage_ComponentType type,
							unsigned layout, DXGI_FORMAT *d3dFormat);
typedef NvCV_Status NvCV_API (*NvCVImage_FromD3DFormat_t)(DXGI_FORMAT d3dFormat, NvCVImage_PixelFormat *format,
							  NvCVImage_ComponentType *type, unsigned char *layout);
typedef NvCV_Status NvCV_API (*NvCVImage_ToD3DColorSpace_t)(unsigned char nvcvColorSpace,
							    DXGI_COLOR_SPACE_TYPE *pD3dColorSpace);
typedef NvCV_Status NvCV_API (*NvCVImage_FromD3DColorSpace_t)(DXGI_COLOR_SPACE_TYPE d3dColorSpace,
							      unsigned char *pNvcvColorSpace);
typedef NvCV_Status NvCV_API (*NvCVImage_InitFromD3D11Texture_t)(NvCVImage *im, struct ID3D11Texture2D *tx);
typedef NvCV_Status NvCV_API (*NvCVImage_InitFromD3DTexture_t)(NvCVImage *im, struct ID3D11Texture2D *tx);

/* cuda runtime */
typedef enum cudaError {
	cudaSuccess = 0,
	cudaErrorInvalidValue = 1,
	cudaErrorMemoryAllocation = 2,
	cudaErrorInitializationError = 3,
	cudaErrorCudartUnloading = 4,
	cudaErrorProfilerDisabled = 5,
	cudaErrorProfilerNotInitialized = 6,
	cudaErrorProfilerAlreadyStarted = 7,
	cudaErrorProfilerAlreadyStopped = 8,
	cudaErrorInvalidConfiguration = 9,
	cudaErrorInvalidPitchValue = 12,
	cudaErrorInvalidSymbol = 13,
	cudaErrorInvalidHostPointer = 16,
	cudaErrorInvalidDevicePointer = 17,
	cudaErrorInvalidTexture = 18,
	cudaErrorInvalidTextureBinding = 19,
	cudaErrorInvalidChannelDescriptor = 20,
	cudaErrorInvalidMemcpyDirection = 21,
	cudaErrorAddressOfConstant = 22,
	cudaErrorTextureFetchFailed = 23,
	cudaErrorTextureNotBound = 24,
	cudaErrorSynchronizationError = 25,
	cudaErrorInvalidFilterSetting = 26,
	cudaErrorInvalidNormSetting = 27,
	cudaErrorMixedDeviceExecution = 28,
	cudaErrorNotYetImplemented = 31,
	cudaErrorMemoryValueTooLarge = 32,
	cudaErrorStubLibrary = 34,
	cudaErrorInsufficientDriver = 35,
	cudaErrorCallRequiresNewerDriver = 36,
	cudaErrorInvalidSurface = 37,
	cudaErrorDuplicateVariableName = 43,
	cudaErrorDuplicateTextureName = 44,
	cudaErrorDuplicateSurfaceName = 45,
	cudaErrorDevicesUnavailable = 46,
	cudaErrorIncompatibleDriverContext = 49,
	cudaErrorMissingConfiguration = 52,
	cudaErrorPriorLaunchFailure = 53,
	cudaErrorLaunchMaxDepthExceeded = 65,
	cudaErrorLaunchFileScopedTex = 66,
	cudaErrorLaunchFileScopedSurf = 67,
	cudaErrorSyncDepthExceeded = 68,
	cudaErrorLaunchPendingCountExceeded = 69,
	cudaErrorInvalidDeviceFunction = 98,
	cudaErrorNoDevice = 100,
	cudaErrorInvalidDevice = 101,
	cudaErrorDeviceNotLicensed = 102,
	cudaErrorSoftwareValidityNotEstablished = 103,
	cudaErrorStartupFailure = 127,
	cudaErrorInvalidKernelImage = 200,
	cudaErrorDeviceUninitialized = 201,
	cudaErrorMapBufferObjectFailed = 205,
	cudaErrorUnmapBufferObjectFailed = 206,
	cudaErrorArrayIsMapped = 207,
	cudaErrorAlreadyMapped = 208,
	cudaErrorNoKernelImageForDevice = 209,
	cudaErrorAlreadyAcquired = 210,
	cudaErrorNotMapped = 211,
	cudaErrorNotMappedAsArray = 212,
	cudaErrorNotMappedAsPointer = 213,
	cudaErrorECCUncorrectable = 214,
	cudaErrorUnsupportedLimit = 215,
	cudaErrorDeviceAlreadyInUse = 216,
	cudaErrorPeerAccessUnsupported = 217,
	cudaErrorInvalidPtx = 218,
	cudaErrorInvalidGraphicsContext = 219,
	cudaErrorNvlinkUncorrectable = 220,
	cudaErrorJitCompilerNotFound = 221,
	cudaErrorUnsupportedPtxVersion = 222,
	cudaErrorJitCompilationDisabled = 223,
	cudaErrorInvalidSource = 300,
	cudaErrorFileNotFound = 301,
	cudaErrorSharedObjectSymbolNotFound = 302,
	cudaErrorSharedObjectInitFailed = 303,
	cudaErrorOperatingSystem = 304,
	cudaErrorInvalidResourceHandle = 400,
	cudaErrorIllegalState = 401,
	cudaErrorSymbolNotFound = 500,
	cudaErrorNotReady = 600,
	cudaErrorIllegalAddress = 700,
	cudaErrorLaunchOutOfResources = 701,
	cudaErrorLaunchTimeout = 702,
	cudaErrorLaunchIncompatibleTexturing = 703,
	cudaErrorPeerAccessAlreadyEnabled = 704,
	cudaErrorPeerAccessNotEnabled = 705,
	cudaErrorSetOnActiveProcess = 708,
	cudaErrorContextIsDestroyed = 709,
	cudaErrorAssert = 710,
	cudaErrorTooManyPeers = 711,
	cudaErrorHostMemoryAlreadyRegistered = 712,
	cudaErrorHostMemoryNotRegistered = 713,
	cudaErrorHardwareStackError = 714,
	cudaErrorIllegalInstruction = 715,
	cudaErrorMisalignedAddress = 716,
	cudaErrorInvalidAddressSpace = 717,
	cudaErrorInvalidPc = 718,
	cudaErrorLaunchFailure = 719,
	cudaErrorCooperativeLaunchTooLarge = 720,
	cudaErrorNotPermitted = 800,
	cudaErrorNotSupported = 801,
	cudaErrorSystemNotReady = 802,
	cudaErrorSystemDriverMismatch = 803,
	cudaErrorCompatNotSupportedOnDevice = 804,
	cudaErrorStreamCaptureUnsupported = 900,
	cudaErrorStreamCaptureInvalidated = 901,
	cudaErrorStreamCaptureMerge = 902,
	cudaErrorStreamCaptureUnmatched = 903,
	cudaErrorStreamCaptureUnjoined = 904,
	cudaErrorStreamCaptureIsolation = 905,
	cudaErrorStreamCaptureImplicit = 906,
	cudaErrorCapturedEvent = 907,
	cudaErrorStreamCaptureWrongThread = 908,
	cudaErrorTimeout = 909,
	cudaErrorGraphExecUpdateFailure = 910,
	cudaErrorUnknown = 999,
	cudaErrorApiFailureBase = 10000
} cudaError;

typedef enum cudaMemcpyKind {
	cudaMemcpyHostToHost = 0,     /**< Host   -> Host */
	cudaMemcpyHostToDevice = 1,   /**< Host   -> Device */
	cudaMemcpyDeviceToHost = 2,   /**< Device -> Host */
	cudaMemcpyDeviceToDevice = 3, /**< Device -> Device */
	cudaMemcpyDefault = 4
} cudaMemcpyKind;

typedef enum cudaError cudaError_t;

typedef cudaError_t CUDARTAPI (*cudaMalloc_t)(void **devPtr, size_t size);
typedef cudaError_t CUDARTAPI (*cudaStreamSynchronize_t)(CUstream stream);
typedef cudaError_t CUDARTAPI (*cudaFree_t)(void *devPtr);
typedef cudaError_t CUDARTAPI (*cudaMemsetAsync_t)(void *devPtr, int value, size_t count, CUstream stream);
typedef cudaError_t CUDARTAPI (*cudaMemcpy_t)(void *dst, const void *src, size_t count, enum cudaMemcpyKind kind);

/* NVVFX */
static NvVFX_GetVersion_t NvVFX_GetVersion = NULL;
static NvVFX_CreateEffect_t NvVFX_CreateEffect = NULL;
static NvVFX_DestroyEffect_t NvVFX_DestroyEffect = NULL;
static NvVFX_SetU32_t NvVFX_SetU32 = NULL;
static NvVFX_SetS32_t NvVFX_SetS32 = NULL;
static NvVFX_SetF32_t NvVFX_SetF32 = NULL;
static NvVFX_SetF64_t NvVFX_SetF64 = NULL;
static NvVFX_SetU64_t NvVFX_SetU64 = NULL;
static NvVFX_SetObject_t NvVFX_SetObject = NULL;
static NvVFX_SetCudaStream_t NvVFX_SetCudaStream = NULL;
static NvVFX_SetImage_t NvVFX_SetImage = NULL;
static NvVFX_SetString_t NvVFX_SetString = NULL;
static NvVFX_GetU32_t NvVFX_GetU32 = NULL;
static NvVFX_GetS32_t NvVFX_GetS32 = NULL;
static NvVFX_GetF32_t NvVFX_GetF32 = NULL;
static NvVFX_GetF64_t NvVFX_GetF64 = NULL;
static NvVFX_GetU64_t NvVFX_GetU64 = NULL;
static NvVFX_GetObject_t NvVFX_GetObject = NULL;
static NvVFX_GetCudaStream_t NvVFX_GetCudaStream = NULL;
static NvVFX_GetImage_t NvVFX_GetImage = NULL;
static NvVFX_GetString_t NvVFX_GetString = NULL;
static NvVFX_Run_t NvVFX_Run = NULL;
static NvVFX_Load_t NvVFX_Load = NULL;
static NvVFX_CudaStreamCreate_t NvVFX_CudaStreamCreate = NULL;
static NvVFX_CudaStreamDestroy_t NvVFX_CudaStreamDestroy = NULL;

/* NVVFX SDK >= 0.7.0 */
static NvVFX_SetStateObjectHandleArray_t NvVFX_SetStateObjectHandleArray = NULL;
static NvVFX_AllocateState_t NvVFX_AllocateState = NULL;
static NvVFX_DeallocateState_t NvVFX_DeallocateState = NULL;
static NvVFX_ResetState_t NvVFX_ResetState = NULL;

/* NVVFX SDK >= 0.7.5 */
static NvVFX_ConfigureLogger_t NvVFX_ConfigureLogger = NULL;

/*nvcvimage */
static NvCVImage_Init_t NvCVImage_Init = NULL;
static NvCVImage_InitView_t NvCVImage_InitView = NULL;
static NvCVImage_Alloc_t NvCVImage_Alloc = NULL;
static NvCVImage_Realloc_t NvCVImage_Realloc = NULL;
static NvCVImage_Dealloc_t NvCVImage_Dealloc = NULL;
static NvCVImage_Create_t NvCVImage_Create = NULL;
static NvCVImage_Destroy_t NvCVImage_Destroy = NULL;
static NvCVImage_ComponentOffsets_t NvCVImage_ComponentOffsets = NULL;
static NvCVImage_Transfer_t NvCVImage_Transfer = NULL;
static NvCVImage_TransferRect_t NvCVImage_TransferRect = NULL;
static NvCVImage_TransferFromYUV_t NvCVImage_TransferFromYUV = NULL;
static NvCVImage_TransferToYUV_t NvCVImage_TransferToYUV = NULL;
static NvCVImage_MapResource_t NvCVImage_MapResource = NULL;
static NvCVImage_UnmapResource_t NvCVImage_UnmapResource = NULL;
static NvCVImage_Composite_t NvCVImage_Composite = NULL;
static NvCVImage_CompositeRect_t NvCVImage_CompositeRect = NULL;
static NvCVImage_CompositeOverConstant_t NvCVImage_CompositeOverConstant = NULL;
static NvCVImage_FlipY_t NvCVImage_FlipY = NULL;
static NvCVImage_GetYUVPointers_t NvCVImage_GetYUVPointers = NULL;
/* nvcvimage  D3D*/
static NvCVImage_ToD3DFormat_t NvCVImage_ToD3DFormat = NULL;
static NvCVImage_FromD3DFormat_t NvCVImage_FromD3DFormat = NULL;
static NvCVImage_ToD3DColorSpace_t NvCVImage_ToD3DColorSpace = NULL;
static NvCVImage_FromD3DColorSpace_t NvCVImage_FromD3DColorSpace = NULL;
static NvCVImage_InitFromD3D11Texture_t NvCVImage_InitFromD3D11Texture = NULL;
/* error codes */
static NvCV_GetErrorStringFromCode_t NvCV_GetErrorStringFromCode = NULL;

/* cuda runtime */
static cudaMalloc_t cudaMalloc = NULL;
static cudaStreamSynchronize_t cudaStreamSynchronize = NULL;
static cudaFree_t cudaFree = NULL;
static cudaMemcpy_t cudaMemcpy = NULL;
static cudaMemsetAsync_t cudaMemsetAsync = NULL;

static inline void release_nv_vfx()
{
	NvVFX_CreateEffect = NULL;
	NvVFX_CudaStreamCreate = NULL;
	NvVFX_CudaStreamDestroy = NULL;
	NvVFX_DestroyEffect = NULL;
	NvVFX_GetCudaStream = NULL;
	NvCV_GetErrorStringFromCode = NULL;
	NvVFX_GetF32 = NULL;
	NvVFX_GetF64 = NULL;
	NvVFX_GetImage = NULL;
	NvVFX_GetObject = NULL;
	NvVFX_GetS32 = NULL;
	NvVFX_GetString = NULL;
	NvVFX_GetU32 = NULL;
	NvVFX_GetU64 = NULL;
	NvVFX_GetVersion = NULL;
	NvVFX_Load = NULL;
	NvVFX_Run = NULL;
	NvVFX_SetCudaStream = NULL;
	NvVFX_SetF32 = NULL;
	NvVFX_SetF64 = NULL;
	NvVFX_SetImage = NULL;
	NvVFX_SetObject = NULL;
	NvVFX_SetS32 = NULL;
	NvVFX_SetString = NULL;
	NvVFX_SetU32 = NULL;
	NvVFX_SetU64 = NULL;
	NvVFX_SetStateObjectHandleArray = NULL;
	NvVFX_AllocateState = NULL;
	NvVFX_DeallocateState = NULL;
	NvVFX_ResetState = NULL;
	NvVFX_ConfigureLogger = NULL;
	if (nv_videofx) {
		FreeLibrary(nv_videofx);
		nv_videofx = NULL;
	}
	NvCVImage_Alloc = NULL;
	NvCVImage_ComponentOffsets = NULL;
	NvCVImage_Composite = NULL;
	NvCVImage_CompositeRect = NULL;
	NvCVImage_CompositeOverConstant = NULL;
	NvCVImage_Create = NULL;
	NvCVImage_Dealloc = NULL;
	NvCVImage_Destroy = NULL;
	NvCVImage_Init = NULL;
	NvCVImage_InitView = NULL;
	NvCVImage_Realloc = NULL;
	NvCVImage_Transfer = NULL;
	NvCVImage_TransferRect = NULL;
	NvCVImage_TransferFromYUV = NULL;
	NvCVImage_TransferToYUV = NULL;
	NvCVImage_MapResource = NULL;
	NvCVImage_UnmapResource = NULL;
	NvCVImage_InitFromD3D11Texture = NULL;
	NvCVImage_FlipY = NULL;
	NvCVImage_GetYUVPointers = NULL;
	NvCVImage_ToD3DFormat = NULL;
	NvCVImage_FromD3DFormat = NULL;
	NvCVImage_ToD3DColorSpace = NULL;
	NvCVImage_FromD3DColorSpace = NULL;
	if (nv_cvimage) {
		FreeLibrary(nv_cvimage);
		nv_cvimage = NULL;
	}
}

static inline void nvvfx_get_sdk_path(char *buffer, const size_t len)
{
	DWORD ret = GetEnvironmentVariableA("NV_VIDEO_EFFECTS_PATH", buffer, (DWORD)len);

	if (!ret || ret >= len - 1) {
		char path[MAX_PATH];
		GetEnvironmentVariableA("ProgramFiles", path, MAX_PATH);

		size_t max_len = sizeof(path) / sizeof(char);
		snprintf(buffer, max_len, "%s\\NVIDIA Corporation\\NVIDIA Video Effects", path);
	}
}

static inline bool load_nv_vfx_libs()
{
	char fullPath[MAX_PATH];
	nvvfx_get_sdk_path(fullPath, MAX_PATH);
	SetDllDirectoryA(fullPath);

	nv_videofx = LoadLibrary(L"NVVideoEffects.dll");
	nv_cvimage = LoadLibrary(L"NVCVImage.dll");
	SetDllDirectoryA(NULL);
	return !!nv_videofx && !!nv_cvimage;
}

static unsigned int get_lib_version(void)
{
	static unsigned int version = 0;
	static bool version_checked = false;

	if (version_checked)
		return version;

	version_checked = true;

	char path[MAX_PATH];
	nvvfx_get_sdk_path(path, sizeof(path));

	SetDllDirectoryA(path);

	struct win_version_info nto_ver = {0};
	if (get_dll_ver(L"NVVideoEffects.dll", &nto_ver))
		version = nto_ver.major << 24 | nto_ver.minor << 16 | nto_ver.build << 8 | nto_ver.revis << 0;

	SetDllDirectoryA(NULL);
	return version;
}
#endif

#ifdef __cplusplus
}
#endif // __cplusplus
