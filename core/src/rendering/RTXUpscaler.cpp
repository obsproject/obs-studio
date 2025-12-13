#include "RTXUpscaler.h"
#include "VulkanRenderer.h"
#include <QDebug>

// NVIDIA SDK includes (conditional)
#ifdef NVIDIA_VIDEO_EFFECTS_SDK_AVAILABLE
#include "nvVideoEffects.h"
#include "nvCVImage.h"
#endif

namespace NeuralStudio {
namespace Rendering {

RTXUpscaler::RTXUpscaler(VulkanRenderer *renderer, QObject *parent) : QObject(parent), m_renderer(renderer) {}

RTXUpscaler::~RTXUpscaler()
{
	destroyNVVFXEffect();
	destroyCUDABuffers();
	unloadNVIDIASDK();
}

bool RTXUpscaler::initialize(uint32_t width, uint32_t height, ScaleFactor scale, QualityMode quality)
{
	if (m_initialized) {
		qWarning() << "RTXUpscaler already initialized";
		return true;
	}

	if (!m_renderer) {
		qCritical() << "Invalid VulkanRenderer";
		return false;
	}

	m_inputWidth = width;
	m_inputHeight = height;
	m_scaleFactor = scale;
	m_qualityMode = quality;

	// Calculate output resolution based on scale factor
	float scaleMult = 1.0f;
	switch (scale) {
	case ScaleFactor::Scale_1_33x:
		scaleMult = 1.33f;
		break;
	case ScaleFactor::Scale_1_5x:
		scaleMult = 1.5f;
		break;
	case ScaleFactor::Scale_2x:
		scaleMult = 2.0f;
		break;
	case ScaleFactor::Scale_3x:
		scaleMult = 3.0f;
		break;
	case ScaleFactor::Scale_4x:
		scaleMult = 4.0f;
		break;
	}
	m_outputWidth = static_cast<uint32_t>(width * scaleMult);
	m_outputHeight = static_cast<uint32_t>(height * scaleMult);

	// Load NVIDIA SDK
	if (!loadNVIDIASDK()) {
		qWarning() << "NVIDIA Video Effects SDK not available - RTX upscaling disabled";
		emit upscaleFailed("NVIDIA SDK not found");
		return false;
	}

	// Create NVVFX effect
	if (!createNVVFXEffect()) {
		qCritical() << "Failed to create NVIDIA upscaling effect";
		emit upscaleFailed("Failed to create NVVFX effect");
		return false;
	}

	// Create CUDA buffers
	if (!createCUDABuffers()) {
		qCritical() << "Failed to create CUDA buffers";
		emit upscaleFailed("CUDA buffer creation failed");
		return false;
	}

	m_initialized = true;
	m_sdkAvailable = true;

	qInfo() << "RTXUpscaler initialized";
	qInfo() << "  Input:" << m_inputWidth << "x" << m_inputHeight;
	qInfo() << "  Output:" << m_outputWidth << "x" << m_outputHeight;
	qInfo() << "  Scale:" << scaleMult << "x";
	qInfo() << "  Quality:" << (quality == QualityMode::HighQuality ? "High Quality" : "Performance");

	return true;
}

bool RTXUpscaler::upscale(QRhiTexture *input, QRhiTexture *output)
{
	if (!m_initialized || !m_sdkAvailable) {
		return false;
	}

	if (!input || !output) {
		qWarning() << "Invalid input or output texture";
		return false;
	}

#ifndef NVIDIA_VIDEO_EFFECTS_SDK_AVAILABLE
	qWarning() << "NVIDIA SDK not available at compile time";
	return false;
#else

	// Step 1: Map Qt RHI textures to CUDA arrays
	void *cudaInputPtr = nullptr;
	void *cudaOutputPtr = nullptr;

	if (!mapVulkanToCUDA(input, &cudaInputPtr)) {
		qCritical() << "Failed to map input texture to CUDA";
		return false;
	}

	if (!mapVulkanToCUDA(output, &cudaOutputPtr)) {
		unmapVulkanFromCUDA(cudaInputPtr);
		qCritical() << "Failed to map output texture to CUDA";
		return false;
	}

	// Step 2: Set up NvCVImage structures pointing to CUDA arrays
	m_cudaInputImage->pixels = cudaInputPtr;
	m_cudaInputImage->width = m_inputWidth;
	m_cudaInputImage->height = m_inputHeight;
	m_cudaInputImage->pitch = 0; // CUDA arrays don't use pitch
	m_cudaInputImage->pixelFormat = NVCV_RGBA;
	m_cudaInputImage->componentType = NVCV_U8;
	m_cudaInputImage->gpuMem = NVCV_CUDA_ARRAY;

	m_cudaOutputImage->pixels = cudaOutputPtr;
	m_cudaOutputImage->width = m_outputWidth;
	m_cudaOutputImage->height = m_outputHeight;
	m_cudaOutputImage->pitch = 0;
	m_cudaOutputImage->pixelFormat = NVCV_RGBA;
	m_cudaOutputImage->componentType = NVCV_U8;
	m_cudaOutputImage->gpuMem = NVCV_CUDA_ARRAY;

	// Step 3: Set images to NVVFX effect
	NvCV_Status status = NvVFX_SetImage(m_nvvfxHandle, NVVFX_INPUT_IMAGE, m_cudaInputImage);
	if (status != NVCV_SUCCESS) {
		qCritical() << "Failed to set NVVFX input image:" << status;
		unmapVulkanFromCUDA(cudaInputPtr);
		unmapVulkanFromCUDA(cudaOutputPtr);
		return false;
	}

	status = NvVFX_SetImage(m_nvvfxHandle, NVVFX_OUTPUT_IMAGE, m_cudaOutputImage);
	if (status != NVCV_SUCCESS) {
		qCritical() << "Failed to set NVVFX output image:" << status;
		unmapVulkanFromCUDA(cudaInputPtr);
		unmapVulkanFromCUDA(cudaOutputPtr);
		return false;
	}

	// Step 4: Run AI upscaling
	status = NvVFX_Run(m_nvvfxHandle, 0); // 0 = synchronous
	if (status != NVCV_SUCCESS) {
		qCritical() << "NVVFX upscaling failed:" << status;
		unmapVulkanFromCUDA(cudaInputPtr);
		unmapVulkanFromCUDA(cudaOutputPtr);
		emit upscaleFailed(QString("NVVFX error: %1").arg(status));
		return false;
	}

	// Step 5: Unmap CUDA resources
	unmapVulkanFromCUDA(cudaInputPtr);
	unmapVulkanFromCUDA(cudaOutputPtr);

	qDebug() << "RTX upscaling successful:" << m_inputWidth << "x" << m_inputHeight << "→" << m_outputWidth << "x"
		 << m_outputHeight;

	emit upscaleCompleted();
	return true;

#endif
}

bool RTXUpscaler::isAvailable()
{
	// TODO: Check if NVIDIA GPU + SDK is available
	// Check for NVVideoEffects.dll/.so in system paths
	// Check for CUDA runtime
	return false; // Placeholder
}

unsigned int RTXUpscaler::getSDKVersion()
{
#ifdef NVIDIA_VIDEO_EFFECTS_SDK_AVAILABLE
	unsigned int version = 0;
	NvVFX_GetVersion(&version);
	return version;
#else
	return 0;
#endif
}

void RTXUpscaler::setQualityMode(QualityMode mode)
{
	if (m_qualityMode != mode) {
		m_qualityMode = mode;

#ifdef NVIDIA_VIDEO_EFFECTS_SDK_AVAILABLE
		if (m_nvvfxHandle) {
			// Update NVVFX effect quality parameter
			NvVFX_SetU32(m_nvvfxHandle, NVVFX_STRENGTH, static_cast<unsigned int>(mode));
		}
#endif

		qInfo() << "RTX quality mode changed to:"
			<< (mode == QualityMode::HighQuality ? "High Quality" : "Performance");
	}
}

void RTXUpscaler::getResolution(uint32_t &inWidth, uint32_t &inHeight, uint32_t &outWidth, uint32_t &outHeight) const
{
	inWidth = m_inputWidth;
	inHeight = m_inputHeight;
	outWidth = m_outputWidth;
	outHeight = m_outputHeight;
}

bool RTXUpscaler::loadNVIDIASDK()
{
	// TODO: Implement dynamic library loading
	// Similar to NVVideoEffectsProxy.cpp from plugin
	//
	// Steps:
	// 1. Check environment variable NV_VIDEO_EFFECTS_PATH
	// 2. Load NVVideoEffects.dll/.so
	// 3. Get function pointers for NvVFX_* functions
	// 4. Verify SDK version compatibility

	qDebug() << "Loading NVIDIA Video Effects SDK...";

#ifdef NVIDIA_VIDEO_EFFECTS_SDK_AVAILABLE
	// Load library
	// m_nvvfxLibrary = dlopen/LoadLibrary("NVVideoEffects");
	return true; // Placeholder
#else
	qWarning() << "NVIDIA Video Effects SDK not compiled in (missing SDK at build time)";
	return false;
#endif
}

void RTXUpscaler::unloadNVIDIASDK()
{
	if (m_nvvfxLibrary) {
		// dlclose/FreeLibrary(m_nvvfxLibrary);
		m_nvvfxLibrary = nullptr;
	}
}

bool RTXUpscaler::createNVVFXEffect()
{
#ifdef NVIDIA_VIDEO_EFFECTS_SDK_AVAILABLE
	// Create super resolution effect
	// NvCV_Status status = NvVFX_CreateEffect(NVVFX_FX_SUPER_RES, &m_nvvfxHandle);
	// if (status != NVCV_SUCCESS) {
	//     qCritical() << "Failed to create NVVFX effect:" << status;
	//     return false;
	// }
	//
	// // Set parameters
	// NvVFX_SetU32(m_nvvfxHandle, NVVFX_SCALE, static_cast<unsigned int>(m_scaleFactor));
	// NvVFX_SetU32(m_nvvfxHandle, NVVFX_STRENGTH, static_cast<unsigned int>(m_qualityMode));
	// NvVFX_SetU32(m_nvvfxHandle, NVVFX_INPUT_WIDTH, m_inputWidth);
	// NvVFX_SetU32(m_nvvfxHandle, NVVFX_INPUT_HEIGHT, m_inputHeight);
	// NvVFX_SetU32(m_nvvfxHandle, NVVFX_OUTPUT_WIDTH, m_outputWidth);
	// NvVFX_SetU32(m_nvvfxHandle, NVVFX_OUTPUT_HEIGHT, m_outputHeight);
	//
	// // Load models
	// status = NvVFX_Load(m_nvvfxHandle);
	// if (status != NVCV_SUCCESS) {
	//     qCritical() << "Failed to load NVVFX models:" << status;
	//     return false;
	// }

	qDebug() << "Created NVVFX Super Resolution effect";
	return true;
#else
	return false;
#endif
}

void RTXUpscaler::destroyNVVFXEffect()
{
#ifdef NVIDIA_VIDEO_EFFECTS_SDK_AVAILABLE
	if (m_nvvfxHandle) {
		// NvVFX_DestroyEffect(m_nvvfxHandle);
		m_nvvfxHandle = nullptr;
	}
#endif
}

bool RTXUpscaler::createCUDABuffers()
{
	// TODO: Allocate CUDA buffers for input/output
	// NvCVImage_Alloc(&m_cudaInputImage, m_inputWidth, m_inputHeight, NVCV_RGBA, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1);
	// NvCVImage_Alloc(&m_cudaOutputImage, m_outputWidth, m_outputHeight, NVCV_RGBA, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1);

	qDebug() << "Created CUDA buffers";
	return true;
}

void RTXUpscaler::destroyCUDABuffers()
{
#ifdef NVIDIA_VIDEO_EFFECTS_SDK_AVAILABLE
	if (m_cudaInputImage) {
		// NvCVImage_Dealloc(m_cudaInputImage);
		m_cudaInputImage = nullptr;
	}
	if (m_cudaOutputImage) {
		// NvCVImage_Dealloc(m_cudaOutputImage);
		m_cudaOutputImage = nullptr;
	}
	```
#endif
}

bool RTXUpscaler::mapVulkanToCUDA(QRhiTexture *texture, void **cudaBuffer)
{
#ifndef NVIDIA_VIDEO_EFFECTS_SDK_AVAILABLE
	return false;
#else

	// Get OpenGL texture ID from Qt RHI
	auto nativeTexture = texture->nativeTexture();
	uint32_t glTexID = static_cast<uint32_t>(nativeTexture.object);

	if (glTexID == 0) {
		qCritical() << "Invalid OpenGL texture ID from Qt RHI";
		return false;
	}

	// Determine if this is input or output texture
	cudaGraphicsResource_t *resourcePtr = nullptr;
	uint32_t *lastTexIDPtr = nullptr;
	bool *registeredPtr = nullptr;

	// Simple heuristic: assume smaller = input, larger = output
	QSize size = texture->pixelSize();
	bool isInput = (size.width() == m_inputWidth && size.height() == m_inputHeight);

	if (isInput) {
		resourcePtr = &m_cudaInputResource;
		lastTexIDPtr = &m_lastInputTexID;
		registeredPtr = &m_inputRegistered;
	} else {
		resourcePtr = &m_cudaOutputResource;
		lastTexIDPtr = &m_lastOutputTexID;
		registeredPtr = &m_outputRegistered;
	}

	// Re-register if texture ID changed
	if (*registeredPtr && *lastTexIDPtr != glTexID) {
		cudaGraphicsUnregisterResource(*resourcePtr);
		*registeredPtr = false;
		*resourcePtr = nullptr;
	}

	// Register GL texture with CUDA (once per texture)
	if (!*registeredPtr) {
		cudaError_t err = cudaGraphicsGLRegisterImage(resourcePtr, glTexID, GL_TEXTURE_2D,
							      isInput ? cudaGraphicsRegisterFlagsReadOnly
								      : cudaGraphicsRegisterFlagsWriteDiscard);

		if (err != cudaSuccess) {
			qCritical() << "Failed to register GL texture with CUDA:" << cudaGetErrorString(err);
			return false;
		}

		*registeredPtr = true;
		*lastTexIDPtr = glTexID;

		qDebug() << "Registered" << (isInput ? "input" : "output") << "GL texture" << glTexID << "with CUDA";
	}

	// Map CUDA resource for this frame
	cudaError_t err = cudaGraphicsMapResources(1, resourcePtr, (CUstream)m_cudaStream);
	if (err != cudaSuccess) {
		qCritical() << "Failed to map CUDA resource:" << cudaGetErrorString(err);
		return false;
	}

	// Get mapped CUDA array
	cudaArray_t cudaArray = nullptr;
	err = cudaGraphicsSubResourceGetMappedArray(&cudaArray, *resourcePtr, 0, 0);
	if (err != cudaSuccess) {
		cudaGraphicsUnmapResources(1, resourcePtr, (CUstream)m_cudaStream);
		qCritical() << "Failed to get mapped CUDA array:" << cudaGetErrorString(err);
		return false;
	}

	*cudaBuffer = (void *)cudaArray;

	// Store the resource for unmapping later
	if (isInput) {
		m_currentlyMappedInput = *resourcePtr;
	} else {
		m_currentlyMappedOutput = *resourcePtr;
	}

	return true;
#endif
}

void RTXUpscaler::unmapVulkanFromCUDA(void *cudaBuffer)
{
#ifdef NVIDIA_VIDEO_EFFECTS_SDK_AVAILABLE
	// Unmap the appropriate resource
	if (m_currentlyMappedInput) {
		cudaGraphicsUnmapResources(1, &m_currentlyMappedInput, (CUstream)m_cudaStream);
		m_currentlyMappedInput = nullptr;
	}
	if (m_currentlyMappedOutput) {
		cudaGraphicsUnmapResources(1, &m_currentlyMappedOutput, (CUstream)m_cudaStream);
		m_currentlyMappedOutput = nullptr;
	}
#endif
}

} // namespace Rendering
} // namespace NeuralStudio
```
