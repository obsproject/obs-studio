#ifndef NV_CV_IMAGE_H
#define NV_CV_IMAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// NvCV_Status codes
typedef enum {
    NVCV_SUCCESS = 0,
    NVCV_ERR_GENERAL = -1,
    NVCV_ERR_UNIMPLEMENTED = -2,
    NVCV_ERR_MEMORY = -3,
    NVCV_ERR_EFFECT = -4,
    NVCV_ERR_SELECTOR = -5,
    NVCV_ERR_BUFFER = -6,
    NVCV_ERR_PARAMETER = -7,
    NVCV_ERR_MISMATCH = -8,
    NVCV_ERR_PIXELFORMAT = -9,
    NVCV_ERR_MODEL = -10,
    NVCV_ERR_LIBRARY = -11,
    NVCV_ERR_INITIALIZATION = -12,
    NVCV_ERR_FILE = -13,
    NVCV_ERR_FEATURENOTFOUND = -14,
    NVCV_ERR_MISSINGINPUT = -15,
    NVCV_ERR_RESOLUTION = -16,
    NVCV_ERR_UNSUPPORTEDGPU = -17,
    NVCV_ERR_CUDA = -18,
    NVCV_ERR_OPENGL = -19,
    NVCV_ERR_DIRECT3D = -20,
} NvCV_Status;

// Pixel Format
typedef enum {
    NVCV_FORMAT_UNKNOWN = 0,
    NVCV_Y       = 1,
    NVCV_A       = 2,
    NVCV_YA      = 3,
    NVCV_RGB     = 4,
    NVCV_BGR     = 5,
    NVCV_RGBA    = 6,
    NVCV_BGRA    = 7,
    NVCV_ARGB    = 8,
    NVCV_ABGR    = 9,
    // ... others
} NvCVImage_PixelFormat;

typedef enum {
    NVCV_U8  = 0,
    NVCV_U16 = 1,
    NVCV_S16 = 2,
    NVCV_F16 = 3,
    NVCV_U32 = 4,
    NVCV_S32 = 5,
    NVCV_F32 = 6,
    NVCV_U64 = 7,
    NVCV_S64 = 8,
    NVCV_F64 = 9,
} NvCVImage_ComponentType;

typedef struct {
    unsigned int width;
    unsigned int height;
    int pitch;
    NvCVImage_PixelFormat pixelFormat;
    NvCVImage_ComponentType componentType;
    unsigned int bufferSize;
    void* pixels;
    void* deletePtr;
    void (*deleteFunc)(void*);
    // ...
    // Simplified stub
} NvCVImage;

// Helpers
void NvCVImage_Init(NvCVImage* im, unsigned int width, unsigned int height, int pitch, void* pixels, NvCVImage_PixelFormat format, NvCVImage_ComponentType type);
void NvCVImage_Destroy(NvCVImage* im);
const char* NvCV_GetErrorStringFromCode(NvCV_Status status);

#ifdef __cplusplus
}
#endif

#endif // NV_CV_IMAGE_H
